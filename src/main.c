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
#define LM35_CHANNEL   ADC_CHANNEL_0
#define SENSOR_PORT    GPIO_A
#define SENSOR_PIN     0U

#define FAN_PWM_PORT   GPIO_B
#define FAN_PWM_PIN    3U

/* --- Logging Step Configuration --- */
#define LOG_INTERVAL_SECONDS  2
#define LOOPS_PER_SECOND      20  /* With a 50ms loop delay, 20 loops = 1 second */
#define LOG_TICK_THRESHOLD    (LOG_INTERVAL_SECONDS * LOOPS_PER_SECOND)

/* --- Global Variables for Async ADC --- */
volatile uint32 currentTemp = 0;
volatile uint16 currentRawAdc = 0;

void Adc_ConversionComplete_Callback(uint16 rawValue) {
    uint32 voltage_mV = (((uint32)rawValue * 5000UL) ) / 4096UL -1;
    currentTemp = voltage_mV / 10;
    currentRawAdc = rawValue;
    Adc_StartConversion();
}

int main(void) {
    char tempString[10];
    uint16 lastDisplayedRaw = 0xFFFF;
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
    Lcd_SendString("Temp: ");

    Gpio_Init(SENSOR_PORT, SENSOR_PIN, GPIO_ANALOG, GPIO_PUSH_PULL);
    Gpio_Init(FAN_PWM_PORT, FAN_PWM_PIN, GPIO_AF, GPIO_PUSH_PULL);
    Gpio_SetAF(FAN_PWM_PORT, FAN_PWM_PIN, GPIO_AF1);

    /* 3. Init App Layer Modules */
    StateMachine_Init();
    Logger_Init();

    /* 4. Init Peripherals */
    Adc_Init(ADC_RES_12BIT);
    Adc_ConfigSingleChannel_OneShot(LM35_CHANNEL);
    Pwm_Init(TIMER_2, PWM_CHANNEL_2, 15, 999);
    Pwm_Start(TIMER_2, PWM_CHANNEL_2);

    for(volatile int i = 0; i < 5000; i++) { __asm("NOP"); }

    /* 5. Start Autonomous ADC Sampling */
    Adc_ReadSingleChannelAsync(Adc_ConversionComplete_Callback);
    Adc_StartConversion();

    /* 6. Main Super-Loop */
    while(1) {
        uint32 displayTemp = currentTemp;
        uint16 displayRaw = currentRawAdc;

        /* LCD & State Machine Update */
        if (displayRaw != lastDisplayedRaw) {

            StateMachine_FormatTempString(displayRaw, tempString);
            Lcd_SetCursor(0, 6);
            Lcd_SendString(tempString);

            StateMachine_Update(displayTemp);
            lastDisplayedRaw = displayRaw;
        }

        /* PACING DELAY (~50ms) */
        for (volatile uint32 d = 0; d < 20000; d++) { __asm("NOP"); }

        /* LOGGER SOFTWARE TIMER */
        tickCounter++;
        if (tickCounter >= LOG_TICK_THRESHOLD) {
            tickCounter = 0;
            secondsElapsed += LOG_INTERVAL_SECONDS;

            // Logger_LogData_Seconds(secondsElapsed, displayRaw);
            Logger_LogData_HMS(secondsElapsed, displayRaw);
        }

        /* Trigger Next Reading */
        Adc_StartConversion();
    }

    return 0;
}