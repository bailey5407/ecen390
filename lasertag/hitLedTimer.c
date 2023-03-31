#include "hitLedTimer.h"
#include "utils.h"
#include "buttons.h"
#include "leds.h"
#include "mio.h"
#include <stdio.h>
#include <stdint.h>

#define LEDS_OFF 0b0000
#define LEDS_ON 0b0011
#define TEST_DELAY 500
#define TIMER_DELAY 10
#define MIO_JF3_PIN 11
#define LED_HIGH_VALUE 1
#define LED_LOW_VALUE 0

//the amount of ticks for the LED to be on for 0.5 seconds
#define LED_ON_TICKS 50000

//keep track of timer start
volatile static bool timerStartFlag = false;

//bool to keep track of timer enabled
static bool enableTimer;

//keep track of elapsed ticks
volatile static uint16_t elapsed_ticks;

//keep track of states
typedef enum {
    ledOn_state,
    ledOff_state
} hit_st_t;

volatile static hit_st_t current_state;

// The hitLedTimer is active for 1/2 second once it is started.
// While active, it turns on the LED connected to MIO pin 11
// and also LED LD0 on the ZYBO board.

// init function
void hitLedTimer_init() {
    enableTimer = true;
    //initialize leds
    leds_init(false);
    //initialize mio
    mio_init(false);
    mio_setPinAsOutput(MIO_JF3_PIN);
    //timer off by default
    timerStartFlag = false;
    //set to off state
    current_state = ledOff_state;
}

// Standard tick function.
void hitLedTimer_tick() {
    //state transition
    switch(current_state) {
        case ledOff_state:
            if(timerStartFlag && enableTimer){
                current_state = ledOn_state;
                elapsed_ticks = 0;
                timerStartFlag = false;
            }
            break;
        case ledOn_state:
            if(elapsed_ticks > LED_ON_TICKS){
                current_state = ledOff_state;
                elapsed_ticks = 0;
            }
            break;
    }
    //state actions
    switch(current_state) {
        case ledOff_state:
            hitLedTimer_turnLedOff();
            break;
        case ledOn_state:
            elapsed_ticks++;
            hitLedTimer_turnLedOn();
            break;
    }
}

// Calling this starts the timer.
void hitLedTimer_start() {
    timerStartFlag = true;
}

// Returns true if the timer is currently running.
bool hitLedTimer_running() {
    if (current_state == ledOn_state) {
        return true;
    }
    return false;
}

// Turns the gun's hit-LED on.
void hitLedTimer_turnLedOn() {
    leds_write(LEDS_ON);
    mio_writePin(MIO_JF3_PIN, LED_HIGH_VALUE);
}

// Turns the gun's hit-LED off.
void hitLedTimer_turnLedOff() {
    leds_write(LEDS_OFF);
    mio_writePin(MIO_JF3_PIN, LED_LOW_VALUE);
}

// Enables the LED timer
void hitLedTimer_enable() {
    enableTimer = true;
}

//disables the LED timer
void hitLedTimer_disable(){
    enableTimer = false;
}

// Runs a visual test of the hit LED until BTN3 is pressed.
// The test continuously blinks the hit-led on and off.
// Depends on the interrupt handler to call tick function.
void hitLedTimer_runTest() {
    hitLedTimer_enable();
    while (!(buttons_read() & BUTTONS_BTN3_MASK)) {
       hitLedTimer_start();
        while (hitLedTimer_running());
        utils_msDelay(TEST_DELAY);
    }
    utils_msDelay(TEST_DELAY); //delay time between tests
    return;
}