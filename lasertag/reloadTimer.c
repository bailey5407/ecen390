#include <stdbool.h>
#include <stdint.h>
#include "game.h"
#include "trigger.h"
#include "sound.h"
// The lockoutTimer is active for 1/2 second once it is started.
// It is used to lock-out the detector once a hit has been detected.
// This ensures that only one hit is detected per 1/2-second interval.

#define MAX_TICKS 300000 // Defined in terms of 100 kHz ticks.
//keep track of elapsed ticks
volatile static uint16_t elapsed_ticks;

//this bool starts the timer
volatile static bool timerStartFlag = false;

typedef enum {
    init_state,
    active_state
} reload_timer_st_t;

volatile static reload_timer_st_t current_state;


// Perform any necessary inits for the lockout timer.
void reloadTimer_init(){
    current_state = init_state;
}

// Standard tick function.
void reloadTimer_tick(){
     //state transitions
    switch(current_state) {
        case init_state:
            if(timerStartFlag){
                elapsed_ticks = 0;
                sound_playSound(sound_gameStart_e);
                current_state = active_state;
            }
            break;
        case active_state:
            if(elapsed_ticks >= MAX_TICKS){
                timerStartFlag = false;
                elapsed_ticks = 0;
                trigger_reload();
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
void reloadTimer_start(){
    timerStartFlag = true;
}

// Returns true if the timer is running.
bool reloadTimer_running(){
    return timerStartFlag;
}
