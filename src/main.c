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

#define LED_PORT       GPIO_B
uint8 led_pins[NUM_SENSORS] = {4U, 5U, 6U, 7U}; /* PB4, PB5, PB6, PB7 */

/* --- Logging Step Configuration --- */
#define LOG_INTERVAL_SECONDS  2
#define LOOPS_PER_SECOND      10  /* 10 loops of 100ms hardware delay = 1 second */
#define LOG_TICK_THRESHOLD    (LOG_INTERVAL_SECONDS * LOOPS_PER_SECOND)

/* --- Global Variables for Async ADC Array --- */
volatile uint16 currentRawAdcArray[NUM_SENSORS] = {0};
uint8 sensorChannels[NUM_SENSORS] = {ADC_CHANNEL_0, ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_6};
volatile uint8 currentSensorIndex = 0;

/* The standard ADC ISR for Round-Robin Polling */
void Adc_ConversionComplete_Callback(uint16 rawValue) {
    currentRawAdcArray[currentSensorIndex] = rawValue;

    currentSensorIndex++;
    if(currentSensorIndex >= NUM_SENSORS) {
        currentSensorIndex = 0;
    }

    Adc_ConfigSingleChannel_OneShot(sensorChannels[currentSensorIndex]);
    Adc_StartConversion();
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
    Rcc_Enable(RCC_TIM2);   /* For PWM */
    Rcc_Enable(RCC_TIM3);   /* For Hardware Delay */
    Rcc_Enable(RCC_USART2);

    /* 2. Init LCD & GPIO */
    Lcd_Init();
    Lcd_SetCursor(0, 0);

    /* Initialize all 4 sensor pins as Analog */
    Gpio_Init(SENSOR_PORT, 0U, GPIO_ANALOG, GPIO_PUSH_PULL);
    Gpio_Init(SENSOR_PORT, 4U, GPIO_ANALOG, GPIO_PUSH_PULL);
    Gpio_Init(SENSOR_PORT, 5U, GPIO_ANALOG, GPIO_PUSH_PULL);
    Gpio_Init(SENSOR_PORT, 6U, GPIO_ANALOG, GPIO_PUSH_PULL);

    /* Initialize all 4 LED pins as Output */
    for(uint8 i = 0; i < NUM_SENSORS; i++) {
        Gpio_Init(LED_PORT, led_pins[i], GPIO_OUTPUT, GPIO_PUSH_PULL);
    }

    Gpio_Init(FAN_PWM_PORT, FAN_PWM_PIN, GPIO_AF, GPIO_PUSH_PULL);
    Gpio_SetAF(FAN_PWM_PORT, FAN_PWM_PIN, GPIO_AF1);

    /* 3. Init App Layer Modules */
    StateMachine_Init();
    Logger_Init();

    /* 4. Init Peripherals */
    Adc_Init(ADC_RES_12BIT);

    Pwm_Init(TIMER_2, PWM_CHANNEL_2, 15, 999);
    Pwm_Start(TIMER_2, PWM_CHANNEL_2);

    /* Initial boot delay using Hardware Timer instead of NOPs */
    Timer_DelayMs(TIMER_3, 200);

    /* 5. Start Autonomous ADC Round-Robin Sampling */
    currentSensorIndex = 0;
    Adc_ConfigSingleChannel_OneShot(sensorChannels[currentSensorIndex]);
    Adc_ReadSingleChannelAsync(Adc_ConversionComplete_Callback);
    Adc_StartConversion();

    /* 6. Main Super-Loop */
    while(1) {

        /* Step A: Find the Maximum Temperature & Update LEDs */
        uint16 maxRawAdc = 0;
        for(uint8 i = 0; i < NUM_SENSORS; i++) {

            /* Calculate Celsius for the specific sensor */
            uint32 voltage_mV = (((uint32)currentRawAdcArray[i] * 5000UL) ) / 4096UL - 1;
            uint32 tempC = voltage_mV / 10;

            /* Warning LED Logic (> 40C) */
            if(tempC > 40) {
                Gpio_WritePin(LED_PORT, led_pins[i], HIGH);
            } else {
                Gpio_WritePin(LED_PORT, led_pins[i], LOW);
            }

            /* Find maximum temperature */
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
        uint8 displayIndex = (secondsElapsed / 2) % NUM_SENSORS;

        StateMachine_FormatTempString(currentRawAdcArray[displayIndex], tempString);
        Lcd_SetCursor(0, 0);
        Lcd_SendString("Zone ");
        Lcd_SendChar(displayIndex + '1');
        Lcd_SendString(": ");
        Lcd_SendString(tempString);
        Lcd_SendString("  "); /* Padding */

        /* PACING DELAY (Exactly 100ms hardware block, bypassing Proteus speed issues) */
        Timer_DelayMs(TIMER_3, 100);

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