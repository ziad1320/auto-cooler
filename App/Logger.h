#ifndef LOGGER_H
#define LOGGER_H

#include "../lib/std_types.h"

void Logger_Init(void);

/* Original Format: [5s] Temp: 25.0°C | Fan: 33% COOLING */
void Logger_LogData_Seconds(uint32 timeSec, uint16 rawAdc);

/* Real-Time Format: [00:00:05] Temp: 25.0°C | Fan: 33% COOLING */
void Logger_LogData_HMS(uint32 timeSec, uint16 rawAdc);

#endif