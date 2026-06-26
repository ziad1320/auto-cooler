#ifndef LCD_H
#define LCD_H

#include "../lib/std_types.h"

/**
 * @brief Initialise GPIO pins and send the HD44780 4-bit init sequence.
 *        Must be called once after Rcc_Enable(RCC_GPIOD).
 */
void Lcd_Init(void);

/**
 * @brief Clear the display and return cursor to home.
 */
void Lcd_Clear(void);

/**
 * @brief Move the cursor to the given row and column.
 * @param Row  0 = top line, 1 = bottom line
 * @param Col  0-15
 */
void Lcd_SetCursor(uint8 Row, uint8 Col);

/**
 * @brief Write a single ASCII character at the current cursor position.
 */
void Lcd_SendChar(char Ch);

/**
 * @brief Write a null-terminated string starting at the current cursor.
 */
void Lcd_SendString(const char *Str);

/**
 * @brief Send a raw HD44780 command byte.
 */
void Lcd_SendCommand(uint8 Cmd);

#endif
