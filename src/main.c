#include "../Rcc/Rcc.h"
#include "../GPIO/GPIO.h"
#include "../lib/std_types.h"

void delay_ms(volatile uint32 count) {
    count *= 1000; 
    while (count--) {
        __asm("NOP"); // No operation
    }
}

int main(void) {
    Rcc_Init();
    
    Rcc_Enable(RCC_GPIOA);
    Rcc_Enable(RCC_GPIOC);
     
    // Initialize LED: Port A, Pin 5, Output Mode, Push-Pull
    Gpio_Init(GPIO_A, 5, GPIO_OUTPUT, GPIO_PUSH_PULL); 
    Gpio_Init(GPIO_A, 4, GPIO_OUTPUT, GPIO_PUSH_PULL);

    // Initialize Button: Port C, Pin 13, Input Mode, Pull-Up
    Gpio_Init(GPIO_C, 13, GPIO_INPUT, GPIO_PULL_UP);

    while (1) {
        uint8 button_state = Gpio_ReadPin(GPIO_C, 13);

        Gpio_WritePin(GPIO_A, 4, HIGH);

        if (button_state == LOW) {
            // Button is pressed: Turn LED ON
            Gpio_WritePin(GPIO_A, 5, HIGH);
            
        } else {
            // Button is released: Turn LED OFF
            Gpio_WritePin(GPIO_A, 5, LOW);
            
        }
        // Small delay to debounce the button and prevent rapid toggling
        delay_ms(10);
    }

    return 0;
}