#include "../lib/std_types.h"
#include "../Rcc/Rcc.h"
#include "../GPIO/Gpio.h"
#include "../Timer/Timer.h"
#include "../Usart/Usart.h"

/* --- Bare-Metal Register Addresses (Bypassing missing CMSIS headers) --- */
#define GPIOA_MODER  (*(volatile uint32*)(0x40020000 + 0x00))
#define GPIOA_IDR    (*(volatile uint32*)(0x40020000 + 0x10))
#define GPIOA_BSRR   (*(volatile uint32*)(0x40020000 + 0x18))

#define STK_CTRL     (*(volatile uint32*)(0xE000E010))
#define STK_LOAD     (*(volatile uint32*)(0xE000E014))
#define STK_VAL      (*(volatile uint32*)(0xE000E018))

/* --- Microsecond Delay (SysTick) --- */
void Delay_us(uint32 us) {
    STK_LOAD = (16 * us) - 1; /* Assumes 16MHz clock */
    STK_VAL = 0;
    STK_CTRL = 5; /* Enable SysTick, core clock */
    while (!(STK_CTRL & (1 << 16))); /* Wait for countdown flag */
    STK_CTRL = 0; /* Disable */
}

/* --- 1-Wire Bare-Metal Pin Control (Hardcoded to PA0) --- */
void DS18B20_SetInput(void) {
    GPIOA_MODER &= ~(3U << (0 * 2)); /* Clear MODER bits for PA0 */
}

void DS18B20_SetOutput(void) {
    GPIOA_MODER &= ~(3U << (0 * 2));
    GPIOA_MODER |=  (1U << (0 * 2)); /* Set PA0 to Output */
}

void DS18B20_WritePin(uint8 state) {
    if (state) {
        GPIOA_BSRR = (1U << 0);      /* Set PA0 High */
    } else {
        GPIOA_BSRR = (1U << 16);     /* Set PA0 Low */
    }
}

uint8 DS18B20_ReadPin(void) {
    return (GPIOA_IDR & (1U << 0)) ? 1 : 0;
}

/* --- 1-Wire Protocol Logic --- */
uint8 DS18B20_Init(void) {
    uint8 response = 0;
    DS18B20_SetOutput();
    DS18B20_WritePin(0);
    Delay_us(480);

    DS18B20_SetInput();
    Delay_us(80);

    if (!(DS18B20_ReadPin())) {
        response = 1;
    } else {
        response = 0;
    }
    Delay_us(400);
    return response;
}

void DS18B20_WriteByte(uint8 data) {
    for (uint8 i = 0; i < 8; i++) {
        DS18B20_SetOutput();
        DS18B20_WritePin(0);
        if (data & (1 << i)) {
            Delay_us(1);
            DS18B20_SetInput();
            Delay_us(60);
        } else {
            Delay_us(60);
            DS18B20_SetInput();
            Delay_us(1);
        }
    }
}

uint8 DS18B20_ReadByte(void) {
    uint8 data = 0;
    for (uint8 i = 0; i < 8; i++) {
        DS18B20_SetOutput();
        DS18B20_WritePin(0);
        Delay_us(2);
        DS18B20_SetInput();
        Delay_us(10);
        if (DS18B20_ReadPin()) {
            data |= (1 << i);
        }
        Delay_us(50);
    }
    return data;
}

int main(void) {
    /* 1. Init Clocks */
    Rcc_Init();
    Rcc_Enable(RCC_GPIOA);
    Rcc_Enable(RCC_USART2);
    Rcc_Enable(RCC_TIM3);

    /* 2. Init USART */
    Usart2_Init();
    Usart2_TransmitString("\r\n--- PHASE 4: DS18B20 DIGITAL SENSOR BOOT ---\r\n");

    /* 3. Main Loop */
    while(1) {
        if (DS18B20_Init()) {
            /* Ask sensor to calculate temperature */
            DS18B20_WriteByte(0xCC); /* Skip ROM */
            DS18B20_WriteByte(0x44); /* Convert T */

            /* The DS18B20 needs 750ms to do the math */
            Timer_DelayMs(TIMER_3, 800);

            /* Ask sensor to send the data */
            DS18B20_Init();
            DS18B20_WriteByte(0xCC); /* Skip ROM */
            DS18B20_WriteByte(0xBE); /* Read Scratchpad */

            uint8 temp_LSB = DS18B20_ReadByte();
            uint8 temp_MSB = DS18B20_ReadByte();

            /* Calculate Celsius */
            uint32 raw_temp = (temp_MSB << 8) | temp_LSB;
            uint32 temp_x10 = (raw_temp * 10) / 16;
            uint32 whole = temp_x10 / 10;
            uint32 decimal = temp_x10 % 10;

            /* Bare-metal Integer to String Conversion */
            char payload[15];
            int idx = 0;

            if (whole == 0) {
                payload[idx++] = '0';
            } else {
                char rev[10];
                int r = 0;
                while (whole > 0) {
                    rev[r++] = (whole % 10) + '0';
                    whole /= 10;
                }
                while (r > 0) {
                    payload[idx++] = rev[--r];
                }
            }

            payload[idx++] = '.';
            payload[idx++] = decimal + '0';
            payload[idx++] = 'C';
            payload[idx++] = '\n';
            payload[idx] = '\0';

            Usart2_TransmitString(payload);
        } else {
            Usart2_TransmitString("SENSOR NOT FOUND!\r\n");
            Timer_DelayMs(TIMER_3, 1000);
        }
    }

    return 0;
}