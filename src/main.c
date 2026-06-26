#include "../Rcc/Rcc.h"
#include "../GPIO/GPIO.h"
#include "../Timer/Timer.h"
#include "../lib/std_types.h"

volatile uint8 async_led_state = LOW;

/* *
 * THE CALLBACK FUNCTION
 * The NVIC will jump here automatically when TIM2 finishes counting.
 */
void Blink_Callback(void) {
    // 1. Toggle LED 1 (PA5)
    async_led_state = (async_led_state == LOW) ? HIGH : LOW;
    Gpio_WritePin(GPIO_A, 5, async_led_state);

    // 2. Re-trigger the async timer!
    // Because your driver uses One-Pulse Mode (CR1_OPM), it stops after one run.
    // We start it again here so it blinks continuously in the background.
    Timer_DelayMsAsync(TIMER_2, 500, Blink_Callback);
}

int main(void) {
    /* 1. CLOCK INITIALIZATION */
    Rcc_Init();
    Rcc_Enable(RCC_GPIOA);
    Rcc_Enable(RCC_TIM2); // Used for Async NVIC test
    Rcc_Enable(RCC_TIM3); // Used for Sync Blocking test

    /* 2. GPIO INITIALIZATION */
    Gpio_Init(GPIO_A, 5, GPIO_OUTPUT, GPIO_PUSH_PULL);
    Gpio_Init(GPIO_A, 6, GPIO_OUTPUT, GPIO_PUSH_PULL);
    Gpio_WritePin(GPIO_A, 5, LOW);
    Gpio_WritePin(GPIO_A, 6, LOW);

    /* 3. START THE ASYNCHRONOUS TIMER (NVIC Test) */
    // This tells TIM2 to count to 500ms in the background and then call our function.
    Timer_DelayMsAsync(TIMER_2, 500, Blink_Callback);

    uint8 sync_led_state = LOW;

    /* 4. THE SUPER LOOP */
    while (1) {
        // Toggle LED 2 (PA6)
        sync_led_state = (sync_led_state == LOW) ? HIGH : LOW;
        Gpio_WritePin(GPIO_A, 6, sync_led_state);

        // Block the CPU for 1000ms (1 second)
        // You will see PA5 (LED 1) continue to blink 2 times while the CPU
        // is "stuck" here waiting for PA6 (LED 2) to delay!
        Timer_DelayMs(TIMER_3, 1000);
    }

    return 0;
}