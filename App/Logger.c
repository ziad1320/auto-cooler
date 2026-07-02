#include "Logger.h"
#include "StateMachine.h"
#include "../Usart/Usart.h"

/* Private helper for standard numbers (e.g. "125") */
static void IntToString(uint32 num, char* str) {
    int i = 0;
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    char temp[12];
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

/* Private helper to force a 2-digit format for clocks (e.g. "05" instead of "5") */
static void IntTo2DigitString(uint8 num, char* str) {
    str[0] = (num / 10) + '0';
    str[1] = (num % 10) + '0';
    str[2] = '\0';
}

void Logger_Init(void) {
    Usart2_Init();
    Usart2_TransmitString("\r\n--- MULTI-ZONE SYSTEM BOOT ---\r\n");
}

/* The original Seconds logger adapted for arrays */
void Logger_LogData_Seconds(uint32 timeSec, uint16* rawAdcArray) {
    char timeStr[12];
    char tempStr[10];

    IntToString(timeSec, timeStr);

    Usart2_TransmitString("[");
    Usart2_TransmitString(timeStr);
    Usart2_TransmitString("s] ");

    for (uint8 i = 0; i < NUM_SENSORS; i++) {
        Usart2_TransmitString("S");
        char sNum[2] = {i + '1', '\0'};
        Usart2_TransmitString(sNum);
        Usart2_TransmitString(":");

        StateMachine_FormatTempString(rawAdcArray[i], tempStr);
        Usart2_TransmitString(tempStr);
        Usart2_TransmitString(" | ");
    }

    const char* fanState = StateMachine_GetStateString();
    Usart2_TransmitString("Fan: ");
    Usart2_TransmitString(fanState);
    Usart2_TransmitString("\r\n");
}

/* The HH:MM:SS logger adapted for arrays */
void Logger_LogData_HMS(uint32 timeSec, uint16* rawAdcArray) {
    char hhStr[3], mmStr[3], ssStr[3];
    char tempStr[10];

    /* Calculate Hours, Minutes, Seconds from total elapsed seconds */
    uint8 hours   = (timeSec / 3600) % 24;
    uint8 minutes = (timeSec / 60) % 60;
    uint8 seconds = timeSec % 60;

    /* Convert to padded strings */
    IntTo2DigitString(hours, hhStr);
    IntTo2DigitString(minutes, mmStr);
    IntTo2DigitString(seconds, ssStr);

    /* Send Timestamp over UART */
    Usart2_TransmitString("[");
    Usart2_TransmitString(hhStr);
    Usart2_TransmitString(":");
    Usart2_TransmitString(mmStr);
    Usart2_TransmitString(":");
    Usart2_TransmitString(ssStr);
    Usart2_TransmitString("] ");

    /* Loop through all sensors and format them sequentially */
    for (uint8 i = 0; i < NUM_SENSORS; i++) {
        Usart2_TransmitString("S");
        char sNum[2] = {i + '1', '\0'}; /* Converts index 0 to '1', etc. */
        Usart2_TransmitString(sNum);
        Usart2_TransmitString(":");

        StateMachine_FormatTempString(rawAdcArray[i], tempStr);
        Usart2_TransmitString(tempStr);
        Usart2_TransmitString(" | ");
    }

    /* Get string info from the State Machine */
    const char* fanState = StateMachine_GetStateString();
    Usart2_TransmitString("Fan: ");
    Usart2_TransmitString(fanState);
    Usart2_TransmitString("\r\n");
}