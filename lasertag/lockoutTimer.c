#include "lockoutTimer.h"
#include "intervalTimer.h"
#include "utils.h"
#include <stdio.h>

#define LOCKOUT_TICKS 50000
#define TEST_DELAY 500

//keep track of elapsed ticks
volatile static uint16_t elapsed_ticks;

//this bool starts the timer
volatile static bool timerStartFlag = false;

typedef enum {
    init_state,
    active_state
} lockout_timer_st_t;

volatile static lockout_timer_st_t current_state;

// Perform any necessary inits for the lockout timer.
void lockoutTimer_init() {
    //initialize state variable
    current_state = init_state;
}

// Standard tick function.
void lockoutTimer_tick() {
    //state transitions
    switch(current_state) {
        case init_state:
            if(timerStartFlag){
                elapsed_ticks = 0;
                current_state = active_state;
            }
            break;
        case active_state:
            if(elapsed_ticks > LOCKOUT_TICKS){
                timerStartFlag = false;
                elapsed_ticks = 0;
                current_state = init_state;
            }
            break;
    }
    //state actions
    switch(current_state) {
        case init_state:
            //no actions
            break;
        case active_state:
            elapsed_ticks++;
            break;
    }
}

// Calling this starts the timer.
void lockoutTimer_start() {
    timerStartFlag = true;
}

// Returns true if the timer is running.
bool lockoutTimer_running() {
    return timerStartFlag;
}

// Test function assumes interrupts have been completely enabled and
// lockoutTimer_tick() function is invoked by isr_function().
// Prints out pass/fail status and other info to console.
// Returns true if passes, false otherwise.
// This test uses the interval timer to determine correct delay for
// the interval timer.
bool lockoutTimer_runTest() {
    intervalTimer_init(INTERVAL_TIMER_TIMER_1);
    intervalTimer_start(INTERVAL_TIMER_TIMER_1);
    lockoutTimer_start();
    while (lockoutTimer_running());
    intervalTimer_stop(INTERVAL_TIMER_TIMER_1);
    printf("%f\n", intervalTimer_getTotalDurationInSeconds(INTERVAL_TIMER_TIMER_1));
    utils_msDelay(TEST_DELAY); //delay time between tests
}