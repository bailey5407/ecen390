#include "isr.h"
#include "hitLedTimer.h"
#include "lockoutTimer.h"
#include "transmitter.h"
#include "trigger.h"
#include "buffer.h"
#include "interrupts.h"
#include "sound.h"
#include <stdio.h>
#include "invincibilityTimer.h"
#include "reloadTimer.h"

// The interrupt service routine (ISR) is implemented here.
// Add function calls for state machine tick functions and
// other interrupt related modules.

//take the most recent value from the ADC and put it into the buffer at 100KHz rate
void updateBuffer(){
    buffer_pushover(interrupts_getAdcData());
}

// Perform initialization for interrupt and timing related modules.
void isr_init(){
    transmitter_init();
    trigger_init();
    hitLedTimer_init();
    lockoutTimer_init();
    buffer_init();
    sound_init();
    invincibilityTimer_init();
    reloadTimer_init();
}

// This function is invoked by the timer interrupt at 100 kHz.
void isr_function(){
    transmitter_tick();
    trigger_tick();
    hitLedTimer_tick();
    lockoutTimer_tick();
    updateBuffer();
    sound_tick();
    invincibilityTimer_tick();
    reloadTimer_tick();
}