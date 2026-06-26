/**
* Lcd_Private.h
 *
 *  HD44780-compatible LCD (LM016L / LCD016L) — 4-bit mode
 *  GPIO pin assignments (all on GPIOD, RW tied to GND):
 *
 *    PC0 → RS  (Register Select: 0=Command, 1=Data)
 *    PC1 → E   (Enable pulse)
 *    PC2 → D4
 *    PC3 → D5
 *    PC4 → D6
 *    PC5 → D7
 *    GND → RW  (always write — no MCU pin required)
 */

#ifndef LCD_PRIVATE_H
#define LCD_PRIVATE_H

/* Port used for all LCD signals */
#define LCD_PORT    'D'

/* Control pins */
#define LCD_RS_PIN  0U
#define LCD_EN_PIN  1U

/* 4-bit data bus pins (D4-D7) */
#define LCD_D4_PIN  2U
#define LCD_D5_PIN  3U
#define LCD_D6_PIN  4U
#define LCD_D7_PIN  5U

/* HD44780 command bytes */
#define LCD_CMD_CLEAR        0x01U   /* Clear display (needs 1.52 ms)     */
#define LCD_CMD_HOME         0x02U   /* Return home  (needs 1.52 ms)      */
#define LCD_CMD_ENTRY_MODE   0x06U   /* Increment cursor, no display shift */
#define LCD_CMD_DISPLAY_OFF  0x08U   /* Display off                        */
#define LCD_CMD_DISPLAY_ON   0x0CU   /* Display on, cursor off, blink off  */
#define LCD_CMD_FUNCTION_4BIT 0x28U  /* 4-bit bus, 2 lines, 5x8 font      */
#define LCD_CMD_DDRAM        0x80U   /* Set DDRAM address (OR with address) */

/* DDRAM row base addresses */
#define LCD_ROW0_BASE        0x00U
#define LCD_ROW1_BASE        0x40U

#endif