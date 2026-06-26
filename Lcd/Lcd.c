/**
 * Lcd.c
 *
 *  HD44780-compatible 16x2 LCD driver — 4-bit parallel mode
 *  Target: STM32F401 @ 16 MHz HSI (no PLL)
 *
 *  Pin map (GPIOC, RW tied to GND):
 *    PC0=RS  PC1=E  PC2=D4  PC3=D5  PC4=D6  PC5=D7
 */

#include "Lcd.h"
#include "Lcd_Private.h"
#include "../GPIO/Gpio.h"
#include "../lib/std_types.h"

/* ------------------------------------------------------------------ */
/*  Busy-wait delay                                                   */
/*  At 16 MHz / -O0: one loop iteration ≈ 4–6 cycles ≈ 0.25–0.375 µs */
/*  Using factor 3 gives ≥ 1 µs/count — conservative, always safe.   */
/* ------------------------------------------------------------------ */
static void Lcd_DelayUs(uint32 Us)
{
    volatile uint32 count = Us * 3UL;
    while (count--)
    {
        /* busy wait */
    }
}

static void Lcd_DelayMs(uint32 Ms)
{
    Lcd_DelayUs(Ms * 1000UL);
}

/* ------------------------------------------------------------------ */
/*  Low-level helpers                                                 */
/* ------------------------------------------------------------------ */

/** Produce a high→low enable pulse (min 230 ns high, min 37 µs cycle) */
static void Lcd_Pulse(void)
{
    Gpio_WritePin(LCD_PORT, LCD_EN_PIN, HIGH);
    Lcd_DelayUs(1U);                    /* tPW ≥ 230 ns */
    Gpio_WritePin(LCD_PORT, LCD_EN_PIN, LOW);
    Lcd_DelayUs(50U);                   /* execution time ≥ 37 µs     */
}

/**
 * @brief Write the lower 4 bits of `nibble` to D4-D7 and pulse Enable.
 *        RS must already be set by the caller.
 */
static void Lcd_WriteNibble(uint8 Nibble)
{
    Gpio_WritePin(LCD_PORT, LCD_D4_PIN, (Nibble >> 0U) & 0x1U);
    Gpio_WritePin(LCD_PORT, LCD_D5_PIN, (Nibble >> 1U) & 0x1U);
    Gpio_WritePin(LCD_PORT, LCD_D6_PIN, (Nibble >> 2U) & 0x1U);
    Gpio_WritePin(LCD_PORT, LCD_D7_PIN, (Nibble >> 3U) & 0x1U);
    Lcd_Pulse();
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

void Lcd_SendCommand(uint8 Cmd)
{
    Gpio_WritePin(LCD_PORT, LCD_RS_PIN, LOW);   /* RS = 0 : command */
    Lcd_WriteNibble(Cmd >> 4U);                 /* high nibble first */
    Lcd_WriteNibble(Cmd & 0x0FU);               /* low nibble        */

    /* Clear and Return-Home need extra time (1.52 ms) */
    if ((Cmd == LCD_CMD_CLEAR) || (Cmd == LCD_CMD_HOME))
    {
        Lcd_DelayMs(2U);
    }
    else
    {
        Lcd_DelayUs(50U);
    }
}

void Lcd_SendChar(char Ch)
{
    Gpio_WritePin(LCD_PORT, LCD_RS_PIN, HIGH);  /* RS = 1 : data */
    Lcd_WriteNibble((uint8)Ch >> 4U);           /* high nibble   */
    Lcd_WriteNibble((uint8)Ch & 0x0FU);         /* low nibble    */
    Lcd_DelayUs(50U);
}

void Lcd_SendString(const char *Str)
{
    while (*Str != '\0')
    {
        Lcd_SendChar(*Str);
        Str++;
    }
}

void Lcd_SetCursor(uint8 Row, uint8 Col)
{
    uint8 address = (Row == 0U) ? (LCD_ROW0_BASE + Col)
                                : (LCD_ROW1_BASE + Col);
    Lcd_SendCommand(LCD_CMD_DDRAM | address);
}

void Lcd_Clear(void)
{
    Lcd_SendCommand(LCD_CMD_CLEAR);
}

void Lcd_Init(void)
{
    /* -------- Configure GPIO pins as push-pull outputs -------- */
    Gpio_Init(LCD_PORT, LCD_RS_PIN, GPIO_OUTPUT, GPIO_PUSH_PULL);
    Gpio_Init(LCD_PORT, LCD_EN_PIN, GPIO_OUTPUT, GPIO_PUSH_PULL);
    Gpio_Init(LCD_PORT, LCD_D4_PIN, GPIO_OUTPUT, GPIO_PUSH_PULL);
    Gpio_Init(LCD_PORT, LCD_D5_PIN, GPIO_OUTPUT, GPIO_PUSH_PULL);
    Gpio_Init(LCD_PORT, LCD_D6_PIN, GPIO_OUTPUT, GPIO_PUSH_PULL);
    Gpio_Init(LCD_PORT, LCD_D7_PIN, GPIO_OUTPUT, GPIO_PUSH_PULL);

    /* Drive all pins low before starting */
    Gpio_WritePin(LCD_PORT, LCD_RS_PIN, LOW);
    Gpio_WritePin(LCD_PORT, LCD_EN_PIN, LOW);
    Gpio_WritePin(LCD_PORT, LCD_D4_PIN, LOW);
    Gpio_WritePin(LCD_PORT, LCD_D5_PIN, LOW);
    Gpio_WritePin(LCD_PORT, LCD_D6_PIN, LOW);
    Gpio_WritePin(LCD_PORT, LCD_D7_PIN, LOW);

    /* -------- HD44780 power-on init sequence (4-bit mode) -------- */
    /*  Wait > 15 ms after VDD rises to 4.5 V (or > 40 ms to 2.7 V) */
    Lcd_DelayMs(20U);

    /*  Step 1-3: Send 0x3 three times (forces 8-bit bus reset)      */
    Gpio_WritePin(LCD_PORT, LCD_RS_PIN, LOW);
    Lcd_WriteNibble(0x3U);
    Lcd_DelayMs(5U);            /* must be > 4.1 ms */

    Lcd_WriteNibble(0x3U);
    Lcd_DelayUs(150U);          /* must be > 100 µs */

    Lcd_WriteNibble(0x3U);
    Lcd_DelayUs(150U);

    /*  Step 4: Switch to 4-bit mode                                 */
    Lcd_WriteNibble(0x2U);
    Lcd_DelayUs(50U);

    /*  Step 5-9: Standard configuration commands                    */
    Lcd_SendCommand(LCD_CMD_FUNCTION_4BIT);  /* 4-bit, 2 lines, 5x8  */
    Lcd_SendCommand(LCD_CMD_DISPLAY_OFF);    /* display off           */
    Lcd_SendCommand(LCD_CMD_CLEAR);          /* clear + 2 ms wait     */
    Lcd_SendCommand(LCD_CMD_ENTRY_MODE);     /* cursor right, no shift */
    Lcd_SendCommand(LCD_CMD_DISPLAY_ON);     /* display on, no cursor  */
}
