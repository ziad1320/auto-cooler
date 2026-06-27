#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "../lib/std_types.h"

/* The strictly defined states from the Project PDF */
typedef enum {
    STATE_IDLE = 0,
    STATE_COOLING,
    STATE_OVERHEAT,
    NUM_STATES
} SystemState_t;

/* Initialize the state machine, LCD, and Alarm LED */
void StateMachine_Init(void);

/* Feed the current temperature into the state machine to trigger logic */
void StateMachine_Update(uint32 currentTemp);

#endif