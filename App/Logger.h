#ifndef LOGGER_H
#define LOGGER_H

#include "../lib/std_types.h"

#define NUM_SENSORS 4

void Logger_Init(void);

/* Original Format */
void Logger_LogData_Seconds(uint32 timeSec, uint16* rawAdcArray);

/* Real-Time Format: [00:00:05] S1:25C | S2:26C | S3:24C | S4:41C | Fan: 100% OVERHEAT */
void Logger_LogData_HMS(uint32 timeSec, uint16* rawAdcArray);

#endif