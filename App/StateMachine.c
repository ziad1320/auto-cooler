#include "StateMachine.h"
#include "../Pwm/Pwm.h"
#include "../Lcd/Lcd.h"
#include "../GPIO/Gpio.h"
#include "../Timer/Timer.h"

/* Hardware Pin definition for the Alarm LED (Using PA1) */
#define ALARM_PORT GPIO_A
#define ALARM_PIN  1

/* The mutually exclusive events based on Temperature Thresholds */
typedef enum {
    EVENT_TEMP_LOW = 0,    /* T < 25 */
    EVENT_TEMP_NORMAL,     /* 25 <= T < 40 */
    EVENT_TEMP_HIGH,       /* T >= 40 */
    NUM_EVENTS
} SystemEvent_t;

/* Private internal state variable */
static SystemState_t CurrentState = STATE_IDLE;

/* Typedef for our transition function pointers */
typedef SystemState_t (*StateHandler_t)(uint32 temp);

/* ========================================================================= */
/*                           PRIVATE HELPER LOGIC                            */
/* ========================================================================= */

/* Generates an Event based on the raw temperature */
static SystemEvent_t GetEvent(uint32 temp) {
    if (temp < 25) return EVENT_TEMP_LOW;
    if (temp >= 40) return EVENT_TEMP_HIGH;
    return EVENT_TEMP_NORMAL;
}

/* Returns the discrete PWM Duty Cycle */
static uint8 CalculateDiscreteDutyCycle(uint32 temp) {
    if (temp < 25) return 0;
    if (temp >= 25 && temp < 30) return 33;
    if (temp >= 30 && temp < 35) return 66;
    return 100; /* T >= 35 */
}

/* LCD UI Helper */
static void UpdateDisplay(uint32 temp, uint8 duty, const char* stateStr) {
    char str[16];
    
    /* Update Line 2: Fan and State */
    uint8 idx = 0;
    str[idx++] = 'F'; str[idx++] = 'a'; str[idx++] = 'n'; str[idx++] = ':';
    
    if (duty >= 100) { str[idx++] = '1'; str[idx++] = '0'; str[idx++] = '0'; }
    else if (duty >= 10) { str[idx++] = (duty / 10) + '0'; str[idx++] = (duty % 10) + '0'; }
    else { str[idx++] = duty + '0'; }
    str[idx++] = '%';
    
    uint8 s = 0;
    while(stateStr[s] != '\0') { str[idx++] = stateStr[s++]; }
    while (idx < 15) { str[idx++] = ' '; } /* Clear trailing chars */
    str[15] = '\0';
    
    Lcd_SetCursor(1, 0);
    Lcd_SendString(str);
}

/* ========================================================================= */
/*                          STATE TRANSITION HANDLERS                        */
/* ========================================================================= */

static SystemState_t Action_GoIdle(uint32 temp) {
    Gpio_WritePin(ALARM_PORT, ALARM_PIN, LOW); /* Alarm OFF */
    Pwm_SetDutyPercent(TIMER_2, PWM_CHANNEL_2, 0);
    UpdateDisplay(temp, 0, "IDLE");
    return STATE_IDLE;
}

static SystemState_t Action_GoCooling(uint32 temp) {
    Gpio_WritePin(ALARM_PORT, ALARM_PIN, LOW); /* Alarm OFF */
    uint8 duty = CalculateDiscreteDutyCycle(temp);
    Pwm_SetDutyPercent(TIMER_2, PWM_CHANNEL_2, duty);
    UpdateDisplay(temp, duty, "COOLING");
    return STATE_COOLING;
}

static SystemState_t Action_GoOverheat(uint32 temp) {
    Gpio_WritePin(ALARM_PORT, ALARM_PIN, HIGH); /* Assert Alarm LED */
    Pwm_SetDutyPercent(TIMER_2, PWM_CHANNEL_2, 100);
    UpdateDisplay(temp, 100, "OVERHEAT");
    return STATE_OVERHEAT;
}

static SystemState_t Action_OverheatToCooling(uint32 temp) {
    return Action_GoCooling(temp); 
}

/* ========================================================================= */
/*                     THE 2D FUNCTION POINTER MATRIX                        */
/* ========================================================================= */

static const StateHandler_t StateMachineMatrix[NUM_STATES][NUM_EVENTS] = {
    /*                   EVENT_TEMP_LOW      EVENT_TEMP_NORMAL     EVENT_TEMP_HIGH */
    /* IDLE     */      {Action_GoIdle,      Action_GoCooling,     Action_GoOverheat},
    /* COOLING  */      {Action_GoIdle,      Action_GoCooling,     Action_GoOverheat},
    /* OVERHEAT */      {Action_GoIdle, Action_OverheatToCooling, Action_GoOverheat}
};

/* ========================================================================= */
/*                               PUBLIC API                                  */
/* ========================================================================= */

void StateMachine_Init(void) {
    /* Initialize the Alarm LED on PA1 */
    Gpio_Init(ALARM_PORT, ALARM_PIN, GPIO_OUTPUT, GPIO_PUSH_PULL);
    Gpio_WritePin(ALARM_PORT, ALARM_PIN, LOW);
    
    CurrentState = STATE_IDLE;
}

void StateMachine_Update(uint32 currentTemp) {
    /* 1. Determine the current Event */
    SystemEvent_t currentEvent = GetEvent(currentTemp);
    
    /* 2. Execute the specific Mealy transition function using the 2D Matrix */
    StateHandler_t transitionFunc = StateMachineMatrix[CurrentState][currentEvent];
    
    /* 3. Update the Current State based on the function's return value */
    CurrentState = transitionFunc(currentTemp);
}