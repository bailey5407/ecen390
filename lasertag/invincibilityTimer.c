
#include <stdbool.h>
#include <stdint.h>
#include "detector.h"
#include "game.h"

#define LOCKOUT_TICKS 500000
#define TICKS_PER_SECOND 100000

//keep track of invincibility time in ticks
static volatile uint32_t invincibility_time_ticks;

//keep track of elapsed ticks
static volatile uint32_t elapsed_ticks;

//this bool starts the timer
volatile static bool timerStartFlag = false;

typedef enum {
    init_state,
    active_state
} invincibility_timer_st_t;

static invincibility_timer_st_t current_state;


// Perform any necessary inits for the invincibility timer.
void invincibilityTimer_init(){
    current_state = init_state;
    invincibility_time_ticks = LOCKOUT_TICKS;
}

// Standard tick function.
void invincibilityTimer_tick(){
     //state transitions
    switch(current_state) {
        case init_state:
            if(timerStartFlag){
                elapsed_ticks = 0;
                current_state = active_state;
                detector_ignoreAllHits(true);
            }
            break;
        case active_state:
            if(elapsed_ticks > invincibility_time_ticks){
                timerStartFlag = false;
                elapsed_ticks = 0;
                current_state = init_state;
                detector_ignoreAllHits(false);
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
void invincibilityTimer_start(uint32_t seconds){
    timerStartFlag = true;
    invincibility_time_ticks = seconds * TICKS_PER_SECOND;
}

// Returns true if the timer is running.
bool invincibilityTimer_running(){
    return timerStartFlag;
}


