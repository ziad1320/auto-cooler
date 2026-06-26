#include "../Rcc/Rcc.h"
#include "../GPIO/GPIO.h"
#include "../LCD/Lcd.h"
#include "../lib/std_types.h"

/* Simple blocking delay so we can visually see the test sequence */
void delay_ms(volatile uint32 count) {
    count *= 1000;
    while (count--) {
        __asm("NOP");
    }
}

int main(void) {
    Rcc_Init();
    Rcc_Enable(RCC_GPIOD);

    Lcd_Init();

    while (1) {
        // Tests: Lcd_SendString()
        // Prints starting at Row 0, Col 0 (default after Init/Clear)
        Lcd_SendString("Testing LCD...");
        delay_ms(1500);

        // Tests: Lcd_SetCursor()
        // Move to the beginning of the second line (Row 1, Col 0)
        Lcd_SetCursor(1, 0);

        // Tests: Lcd_SendChar()
        // Print "OK!" one character at a time with a small delay
        Lcd_SendChar('O');
        delay_ms(300);
        Lcd_SendChar('K');
        delay_ms(300);
        Lcd_SendChar('!');
        delay_ms(1500);

        // Tests: Lcd_Clear()
        // Wipes the screen and moves the hidden cursor back to 0,0
        Lcd_Clear();
        delay_ms(500);

        // Tests: Lcd_SendCommand()
        // We will send command 0x0F: Display ON, Cursor ON, Blink ON
        // This proves the raw command function works by showing a blinking block
        Lcd_SendCommand(0x0F);
        Lcd_SendString("Blinking Cursor!");
        delay_ms(2000);

        // Tests: Lcd_SendCommand() again
        // Send command 0x0C: Display ON, Cursor OFF, Blink OFF (Back to normal)
        Lcd_SendCommand(0x0C);
        Lcd_Clear();
        delay_ms(500);
    }

    return 0;
}