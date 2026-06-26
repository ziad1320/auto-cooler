#include "../Rcc/Rcc.h"
#include "../GPIO/GPIO.h"
#include "../Timer/Timer.h"
#include "../Pwm/Pwm.h"
#include "../lib/std_types.h"

/* Simple blocking delay so we can see the fade effect */
void delay_ms(volatile uint32 count) {
    count *= 1000;
    while (count--) {
        __asm("NOP");
    }
}

int main(void) {
    /* 1. CLOCK INITIALIZATION */
    Rcc_Init();
    Rcc_Enable(RCC_GPIOA);
    Rcc_Enable(RCC_TIM2); // Enable clock for Timer 2

    /* 2. GPIO INITIALIZATION */
    // For PWM to control a pin, it CANNOT be a standard output.
    // It MUST be configured as an Alternate Function (AF).
    // On STM32F401, TIM2_CH1 is mapped to PA0 using Alternate Function 1 (AF1).
    Gpio_Init(GPIO_A, 0, GPIO_AF, GPIO_PUSH_PULL);
    Gpio_SetAF(GPIO_A, 0, GPIO_AF1);

    /* 3. PWM INITIALIZATION */
    // create a 1 kHz PWM signal.
    // Assuming a 16MHz clock:
    // Prescaler = 15  --> Timer clock becomes 1MHz (1 tick = 1 microsecond)
    // AutoReload = 1000 --> 1000 microseconds = 1 millisecond period (1 kHz)
    Pwm_Init(TIMER_2, PWM_CHANNEL_1, 15, 1000);

    // Start the PWM generation on Channel 1
    Pwm_Start(TIMER_2, PWM_CHANNEL_1);

    int8 duty_cycle = 0;
    int8 fade_amount = 5; // How much to change the duty cycle each step

    /* 4. SUPER LOOP */
    while (1) {
        // Update the PWM duty cycle
        Pwm_SetDutyPercent(TIMER_2, PWM_CHANNEL_1, duty_cycle);

        // Change the duty cycle for the next loop iteration
        duty_cycle += fade_amount;

        // Reverse direction at the limits (0% or 100%)
        if (duty_cycle >= 100 || duty_cycle <= 0) {
            fade_amount = -fade_amount;
        }

        // Wait 50ms so the human eye can actually see the fade happening
        delay_ms(50);
    }

    return 0;
}