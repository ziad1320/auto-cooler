#include "../lib/std_types.h"
#include "../Rcc/Rcc.h"
#include "../GPIO/Gpio.h"
#include "../Adc/Adc.h"
#include "../Lcd/Lcd.h"
#include "../Pwm/Pwm.h"
#include "../Timer/Timer.h"
#include "../Usart/Usart.h"
#include "../App/StateMachine.h"

/* --- Hardware Mapping Definitions --- */
#define LM35_CHANNEL   ADC_CHANNEL_0
#define SENSOR_PORT    GPIO_A
#define SENSOR_PIN     0U

#define FAN_PWM_PORT   GPIO_B
#define FAN_PWM_PIN    3U

/* --- Global Variables for Async ADC --- */
volatile uint32 currentTemp = 0;
volatile uint16 currentRawAdc = 0;

/* --- Callback for Interrupt-Driven ADC --- */
void Adc_ConversionComplete_Callback(uint16 rawValue) {
    /* Exact integer math calibrated for 5V hardware simulation */
    uint32 voltage_mV = (((uint32)rawValue * 5000UL) ) / 4096UL -1;

    currentTemp = voltage_mV / 10;
    currentRawAdc = rawValue;

    /* Trigger the next ADC conversion so it loops continuously (No CPU Polling) */
    Adc_StartConversion();
}

/* --- Helper: Format exact Temperature String with Decimal --- */
void IntToTempString(uint16 rawAdc, char* str) {
    uint32 voltage_mV = (((uint32)rawAdc * 5000UL) ) / 4096UL -1;
    uint32 tempInteger = voltage_mV / 10;
    uint32 tempDecimal = voltage_mV % 10;

    /* Remove leading zero */
    if (tempInteger < 100) {
        str[0] = ' ';
    } else {
        str[0] = (tempInteger / 100) + '0';
    }

    str[1] = ((tempInteger / 10) % 10) + '0';
    str[2] = (tempInteger % 10) + '0';
    str[3] = '.';
    str[4] = tempDecimal + '0';
    str[5] = 0xDF; /* Degree Symbol */
    str[6] = 'C';
    str[7] = '\0';
}

/* Helper to convert raw integer to string for UART logging safely */
void IntToString(uint16 num, char* str) {
    int i = 0;
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    char temp[6];
    int temp_idx = 0;
    while (num > 0) {
        temp[temp_idx++] = (num % 10) + '0';
        num /= 10;
    }
    while (temp_idx > 0) {
        str[i++] = temp[--temp_idx];
    }
    str[i] = '\0';
}

/* helper to format and send a CSV log over USB */
void LogDataToPC(uint16 rawAdc, const char* tempString) {
    char rawStr[6];

    /* Safely format just the raw integer without overflowing the buffer */
    IntToString(rawAdc, rawStr);

    // Print in CSV format: [Raw_ADC], [Formatted_Temp]
    Usart2_TransmitString(rawStr);
    Usart2_TransmitString(", ");
    Usart2_TransmitString(tempString);
    Usart2_TransmitString("\r\n"); // Carriage Return + Line Feed
}

int main(void) {
    char tempString[10];
    uint16 lastDisplayedRaw = 0xFFFF; /* Track RAW value to catch every 0.1 decimal change */

    /* 1. Init Clocks */
    Rcc_Init();
    Rcc_Enable(RCC_GPIOA);
    Rcc_Enable(RCC_GPIOB);
    Rcc_Enable(RCC_GPIOD); /* LCD Port */
    Rcc_Enable(RCC_ADC1);
    Rcc_Enable(RCC_TIM2);  /* PWM Port */

    /* ADDED: Enable USART2 Clock */
    Rcc_Enable(RCC_USART2);

    /* 2. Init LCD */
    Lcd_Init();
    Lcd_SetCursor(0, 0);
    Lcd_SendString("Temp: ");

    /* 3. Setup Analog Pin (ADC) for Temperature Sensor */
    Gpio_Init(SENSOR_PORT, SENSOR_PIN, GPIO_ANALOG, GPIO_PUSH_PULL);

    /* 4. Setup Alternate Function Pin (PWM) for Fan Control */
    Gpio_Init(FAN_PWM_PORT, FAN_PWM_PIN, GPIO_AF, GPIO_PUSH_PULL);
    Gpio_SetAF(FAN_PWM_PORT, FAN_PWM_PIN, GPIO_AF1); /* AF1 connects to TIM2 */

    /* 5. Init State Machine & Alarm LED (PA1) */
    StateMachine_Init();

    /* 6. Init ADC */
    Adc_Init(ADC_RES_12BIT);
    Adc_ConfigSingleChannel_OneShot(LM35_CHANNEL);

    /* 7. Init PWM */
    Pwm_Init(TIMER_2, PWM_CHANNEL_2, 15, 999);
    Pwm_Start(TIMER_2, PWM_CHANNEL_2);

    /* ADDED: Init USART2 and Print the CSV Header */
    Usart2_Init();
    Usart2_TransmitString("Raw_ADC, Temperature\r\n");

    /* Allow ADC analog circuits to stabilize */
    for(volatile int i = 0; i < 5000; i++) { __asm("NOP"); }

    /* 8. Start Autonomous Interrupt-Driven ADC Sampling */
    Adc_ReadSingleChannelAsync(Adc_ConversionComplete_Callback);
    Adc_StartConversion();

    /* 9. Main Super-Loop (Non-Blocking) */
    while(1) {

        /* Capture volatile variables safely */
        uint32 displayTemp = currentTemp;
        uint16 displayRaw = currentRawAdc;

        /* Only execute updates if the exact decimal temperature changed */
        if (displayRaw != lastDisplayedRaw) {

            /* Update Top Row of LCD precisely (Main.c handles this to keep decimals) */
            IntToTempString(displayRaw, tempString);
            Lcd_SetCursor(0, 6);
            Lcd_SendString(tempString);

            /* * Update Fan, PWM, Alarm LED, and Bottom Row of LCD */
            StateMachine_Update(displayTemp);

            /* ADDED: Log the data to the PC whenever a change happens */
            LogDataToPC(displayRaw, tempString);

            lastDisplayedRaw = displayRaw;
        }
    }

    return 0;
}