#include "../lib/std_types.h"
#include "../Rcc/Rcc.h"
#include "../GPIO/Gpio.h"
#include "../Adc/Adc.h"
#include "../Lcd/Lcd.h"
#include "../Pwm/Pwm.h"
#include "../Timer/Timer.h"
#include "../App/StateMachine.h"
#include "../App/Logger.h"

/* --- Hardware Mapping Definitions --- */
#define SENSOR_PORT    GPIO_A
#define FAN_PWM_PORT   GPIO_B
#define FAN_PWM_PIN    3U

/* --- Logging Step Configuration --- */
#define LOG_INTERVAL_SECONDS  2
#define LOOPS_PER_SECOND      20
#define LOG_TICK_THRESHOLD    (LOG_INTERVAL_SECONDS * LOOPS_PER_SECOND)

/* --- Global Variables for Async ADC Array --- */
volatile uint16 currentRawAdcArray[NUM_SENSORS] = {0};
uint8 sensorChannels[NUM_SENSORS] = {ADC_CHANNEL_0, ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_6};

/* FIX 1: Callback matches the 2-parameter signature expected by AdcMultiChannelCallback */
void Adc_GroupConversionComplete_Callback(uint16* rawValues, uint8 numChannels) {
    /* The ADC ISR already wrote the data directly into currentRawAdcArray. */
    /* We just re-trigger the group scan so it constantly runs in the background. */

    /* FIX 2: Pass the Results buffer, not the sensorChannels! */
    Adc_ScanChannelGroupAsync((uint16*)currentRawAdcArray, NUM_SENSORS, Adc_GroupConversionComplete_Callback);
}

int main(void) {
    char tempString[10];
    uint32 tickCounter = 0;
    uint32 secondsElapsed = 0;

    /* 1. Init Clocks */
    Rcc_Init();
    Rcc_Enable(RCC_GPIOA);
    Rcc_Enable(RCC_GPIOB);
    Rcc_Enable(RCC_GPIOD);
    Rcc_Enable(RCC_ADC1);
    Rcc_Enable(RCC_TIM2);
    Rcc_Enable(RCC_USART2);

    /* 2. Init LCD & GPIO */
    Lcd_Init();
    Lcd_SetCursor(0, 0);

    /* Initialize all 4 sensor pins as Analog */
    Gpio_Init(SENSOR_PORT, 0U, GPIO_ANALOG, GPIO_PUSH_PULL);
    Gpio_Init(SENSOR_PORT, 4U, GPIO_ANALOG, GPIO_PUSH_PULL);
    Gpio_Init(SENSOR_PORT, 5U, GPIO_ANALOG, GPIO_PUSH_PULL);
    Gpio_Init(SENSOR_PORT, 6U, GPIO_ANALOG, GPIO_PUSH_PULL);

    Gpio_Init(FAN_PWM_PORT, FAN_PWM_PIN, GPIO_AF, GPIO_PUSH_PULL);
    Gpio_SetAF(FAN_PWM_PORT, FAN_PWM_PIN, GPIO_AF1);

    /* 3. Init App Layer Modules */
    StateMachine_Init();
    Logger_Init();

    /* 4. Init Peripherals */
    Adc_Init(ADC_RES_12BIT);

    /* FIX 3: Configure the ADC multiplexer sequence BEFORE starting the scan */
    Adc_ConfigScanGroup_Continuous(sensorChannels, NUM_SENSORS);

    Pwm_Init(TIMER_2, PWM_CHANNEL_2, 15, 999);
    Pwm_Start(TIMER_2, PWM_CHANNEL_2);

    for(volatile int i = 0; i < 5000; i++) { __asm("NOP"); }

    /* 5. Start Autonomous ADC Group Sampling */
    Adc_ScanChannelGroupAsync((uint16*)currentRawAdcArray, NUM_SENSORS, Adc_GroupConversionComplete_Callback);

    /* 6. Main Super-Loop */
    while(1) {

        /* Step A: Find the Maximum Temperature to drive the Fan State Machine */
        uint16 maxRawAdc = 0;
        for(uint8 i = 0; i < NUM_SENSORS; i++) {
            if(currentRawAdcArray[i] > maxRawAdc) {
                maxRawAdc = currentRawAdcArray[i];
            }
        }

        /* Convert the hottest raw value to Millivolts, then to Celsius */
        uint32 maxVoltage_mV = (((uint32)maxRawAdc * 5000UL) ) / 4096UL - 1;
        uint32 maxTemp = maxVoltage_mV / 10;

        /* Update the fan state based strictly on the hottest refrigerator */
        StateMachine_Update(maxTemp);

        /* Step B: LCD Sequential Display Logic */
        /* Changes which sensor is displayed every 2 seconds */
        uint8 displayIndex = (secondsElapsed / 2) % NUM_SENSORS;

        StateMachine_FormatTempString(currentRawAdcArray[displayIndex], tempString);
        Lcd_SetCursor(0, 0);
        Lcd_SendString("Zone ");
        Lcd_SendChar(displayIndex + '1');
        Lcd_SendString(": ");
        Lcd_SendString(tempString);
        Lcd_SendString("  "); /* Padding to clear leftover characters */

        /* PACING DELAY (~50ms) */
        for (volatile uint32 d = 0; d < 20000; d++) { __asm("NOP"); }

        /* LOGGER SOFTWARE TIMER */
        tickCounter++;
        if (tickCounter >= LOG_TICK_THRESHOLD) {
            tickCounter = 0;
            secondsElapsed += LOG_INTERVAL_SECONDS;

            /* Send the entire array to the UART logger */
            Logger_LogData_HMS(secondsElapsed, (uint16*)currentRawAdcArray);
        }
    }

    return 0;
}