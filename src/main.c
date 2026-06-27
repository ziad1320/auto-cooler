#include "../Rcc/Rcc.h"
#include "../GPIO/GPIO.h"
#include "../LCD/Lcd.h"
#include "../Adc/Adc.h"
#include "../lib/std_types.h"

// Simple blocking delay
void delay_ms(volatile uint32 count) {
    count *= 1000;
    while (count--) {
        __asm("NOP");
    }
}

// Bare-metal helper to convert a number to a string for the LCD
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

int main(void) {
    /* 1. CLOCK INITIALIZATION */
    Rcc_Init();
    Rcc_Enable(RCC_GPIOA);
    Rcc_Enable(RCC_GPIOD);

    Rcc_Enable(RCC_ADC1);

    /* 2. LCD INITIALIZATION */
    Lcd_Init();
    Lcd_SetCursor(0, 0);
    Lcd_SendString("ADC Ch0 Value:");

    /* 3. GPIO INITIALIZATION */
    Gpio_Init(GPIO_A, 0, 3, 0);

    /* 4. ADC INITIALIZATION */
    // 12-bit resolution means values will range from 0 (0V) to 4095 (Max Voltage)
    Adc_Init(ADC_RES_12BIT);
    Adc_ConfigSingleChannel_OneShot(ADC_CHANNEL_0);

    uint16 adc_value = 0;
    char display_str[16];

    /* 5. SUPER LOOP */
    while(1) {
        // Trigger the hardware to take a voltage reading
        Adc_StartConversion();

        // Wait for the reading to finish and grab the result
        adc_value = Adc_ReadSingleChannel();

        // Move cursor to the second line of the LCD
        Lcd_SetCursor(1, 0);
        Lcd_SendString("Raw: ");

        // Convert the 12-bit number to text and print it
        IntToString(adc_value, display_str);
        Lcd_SendString(display_str);

        // Add extra spaces to clear out leftover digits
        Lcd_SendString("    ");

        // Update the screen 10 times a second
        delay_ms(100);
    }

    return 0;
}