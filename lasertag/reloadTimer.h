#ifndef RELOADTIMER_H_
#define RELOADTIMER_H_

#include <stdbool.h>

// The lockoutTimer is active for 1/2 second once it is started.
// It is used to lock-out the detector once a hit has been detected.
// This ensures that only one hit is detected per 1/2-second interval.

// Perform any necessary inits for the lockout timer.
void reloadTimer_init();

// Standard tick function.
void reloadTimer_tick();

// Calling this starts the timer.
void reloadTimer_start();

// Returns true if the timer is running.
bool reloadTimer_running();

// Test function assumes interrupts have been completely enabled and
// lockoutTimer_tick() function is invoked by isr_function().
// Prints out pass/fail status and other info to console.
// Returns true if passes, false otherwise.
// This test uses the interval timer to determine correct delay for
// the interval timer.
bool lockoutTimer_runTest();

#endif
