#include "trigger.h"
#include "buttons.h"
#include "mio.h"
#include "filter.h"
#include "transmitter.h"
#include "utils.h"
#include "game.h"
#include "sound.h"
#include <stdbool.h>
#include <stdio.h>
#include "lockoutTimer.h"
// #include "reloadTimer.h"
#include "invincibilityTimer.h"


#define DEBUG 0
#define TEST_DELAY 500

#define MIO_TRIGGER_PIN 10
#define TRIGGER_BUTTON BUTTONS_BTN0_MASK
#define GUN_TRIGGER_PRESSED 1
#define RELOAD_TICKS 300000

#define DEBOUNCE_PERIOD_SECONDS 5E-2
//given a sample rate of 100kHz and a determined debounce period, the number of debounce ticks is determined accodingly
#define DEBOUNCE_TICKS DEBOUNCE_PERIOD_SECONDS * FILTER_SAMPLE_FREQUENCY_IN_KHZ * 1000

#define DEFAULT_NUMBER_OF_SHOTS 10

volatile static uint32_t reloadCounter;
volatile static bool reloadEnable;

//define states of trigger state machine and current_state to keep track
typedef enum{
    init_state,
    press_debounce_state,
    release_debounce_state
} trigger_st_t;
volatile static trigger_st_t current_state;

//flag to toggle ignoring gun input, this is because the pin is pull up, and we don't want to accidentally read values if no trigger is connected
volatile static bool ignoreGunInput;

//trigger pressed function returns true if either the trigger is pulled or button 0 is pressed
bool triggerPressed() {
	return ((!ignoreGunInput & (mio_readPin(MIO_TRIGGER_PIN) == GUN_TRIGGER_PRESSED)) || 
                (buttons_read() & BUTTONS_BTN0_MASK));
}

//keep track of instantaneous tigger value
volatile static bool trigger_pressed;

//keep track of elapsed ticks
volatile static uint32_t elapsed_ticks;

//trigger enable boolean
volatile static bool trigger_enabled;

//keep track of remaining shots
volatile static trigger_shotsRemaining_t remaining_shots;

//fires a shot, including decrementing the remaining shots and playing the sound
static void trigger_fireShot(){
    //guard clause: can't shoot if lockout timer is running
    if(lockoutTimer_running()) return;
    //guard clause: can't shoot if reload timer is running
    //if(invincibilityTimer_running()) return;
    
    //if we have ammo, fire a shot and decrement remaining_shots
    if(remaining_shots > 0) {
        transmitter_run();
        sound_playSound(sound_gunFire_e);
        remaining_shots--;
        if(remaining_shots == 0){
            reloadEnable = true;
        }
    }
    //if we don't have ammo, play the click noise for an empty clip
    else {
        sound_playSound(sound_gunClick_e);
    }
    return;
}

//reloads the gun, plays sound and sets remaining shots to default
void trigger_reload(){
    remaining_shots = DEFAULT_NUMBER_OF_SHOTS;
    sound_playSound(sound_gunReload_e);
}


/**********************************************************************************/


// Init trigger data-structures.
// Initializes the mio subsystem.
// Determines whether the trigger switch of the gun is connected
// (see discussion in lab web pages).
void trigger_init(){
    //initialize mio input pin
    ignoreGunInput = false;
    mio_init(false);
    mio_setPinAsInput(MIO_TRIGGER_PIN);
    if (triggerPressed()) {
    ignoreGunInput = true;
    }

    //initialize buttons
    buttons_init();

    //set current state to init state
    current_state = init_state;

    //set elapsed ticks to 0
    reloadEnable = false;
    elapsed_ticks = 0;
    reloadCounter = 0;

    //set remaining shots
    trigger_setRemainingShotCount(DEFAULT_NUMBER_OF_SHOTS);
}

// Standard tick function.
void trigger_tick(){
    if(!trigger_enabled){
        return;
    }
    //state transitions
    switch(current_state){
        case init_state:
            //if trigger is pressed, move into debouncing state
            if(trigger_pressed = triggerPressed()){
                current_state = press_debounce_state;
            }
            break;
        case press_debounce_state:
            //once value has settled, reset ticks and move into state depending on settled value
            if(elapsed_ticks > DEBOUNCE_TICKS){
                elapsed_ticks = 0;
                //if we've settled on a trigger pulled value, fire the gun as a transition action and move into release state
                if(trigger_pressed){
                    trigger_fireShot();
                    current_state = release_debounce_state;

                    // if(remaining_shots == 0){
                    //     reloadTimer_start();
                    // }
                }
                //if we've settled on a released value, move into init state
                else current_state = init_state;
            }
            break;
        case release_debounce_state:
            //if the trigger has been held down for three seconds, reload the gun
            if((elapsed_ticks > RELOAD_TICKS) && trigger_pressed){
                trigger_reload();
                elapsed_ticks = 0;
            }
            //once value has settled to released, reset ticks and move into init state
            if(elapsed_ticks > DEBOUNCE_TICKS && !trigger_pressed){
                elapsed_ticks = 0;
                current_state = init_state;
            }
            break;
    }
    //state actions
    switch(current_state){
        case init_state:
            //do nothing
            break;
        case press_debounce_state:
            elapsed_ticks++;
            //if the stored value does not equal the current value, reset tick count and store curent value
            if(trigger_pressed != triggerPressed()){
                elapsed_ticks = 0;
                trigger_pressed = triggerPressed();
            }
            break;
        case release_debounce_state:
            elapsed_ticks++;
            //if the stored value does not equal the current value, reset tick count and store curent value
            if(trigger_pressed != triggerPressed()){
                elapsed_ticks = 0;
                trigger_pressed = triggerPressed();
            }
            break;
    }
    if(reloadEnable){
        reloadCounter++;
        if(reloadCounter > RELOAD_TICKS) {
            trigger_reload();
            reloadCounter = 0;
            reloadEnable = false;
        }
    }
}

// Enable the trigger state machine. The trigger state-machine is inactive until
// this function is called. This allows you to ignore the trigger when helpful
// (mostly useful for testing).
void trigger_enable(){
    trigger_enabled = true;
}

// Disable the trigger state machine so that trigger presses are ignored.
void trigger_disable(){
    trigger_enabled = false;
}

// Returns the number of remaining shots.
trigger_shotsRemaining_t trigger_getRemainingShotCount(){
    return remaining_shots;
}

// Sets the number of remaining shots.
void trigger_setRemainingShotCount(trigger_shotsRemaining_t count){
    remaining_shots = count;
}

// Runs the test continuously until BTN3 is pressed.
// The test just prints out a 'D' when the trigger or BTN0
// is pressed, and a 'U' when the trigger or BTN0 is released.
// Depends on the interrupt handler to call tick function.
void trigger_runTest(){
    trigger_enable();
    while(!(buttons_read() & BUTTONS_BTN3_MASK));
    trigger_disable();
    utils_msDelay(TEST_DELAY); //delay time between tests
    return;
}