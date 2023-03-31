#include "transmitter.h"
#include "filter.h"
#include "mio.h"
#include "buttons.h"
#include "switches.h"
#include "utils.h"
#include <stdio.h>

#define DEFAULT_PLAYER_FREQUENCY 0

//given a rate of 100kHz, the duration of each tick in seconds is defined accordingly
#define TICK_PERIOD_SECONDS 1.0E-5
//given the tick period, the amount of ticks in 200ms is 20,000. In test mode, its 200 ticks
#define BURST_TICK_COUNT 20000
#define BURST_TICK_COUNT_TEST 200

#define DUTY_CYCLE 0.5

#define TRANSMITTER_OUTPUT_PIN 13
#define TRANSMITTER_HIGH_VALUE 1
#define TRANSMITTER_LOW_VALUE 0

#define TEST_DELAY 500
#define BOUNCE_DELAY 5
#define TRANSMITTER_DELAY 400

#define DEBUG 1

//state machine states
typedef enum{
    low_state,
    transmit_state
} tx_st_t;

//keep track of current state
volatile static tx_st_t current_state;

//keep track of whether we're transmitting or not
volatile static bool transmitting;

//keep track of continuous mode
volatile static bool cmFlag;

//keep track of elapsed ticks
volatile static uint16_t elapsed_ticks;

//keep track of frequency we're transmitting in ticks
volatile static uint16_t tx_frequency_ticks;

//keep track of when transmitter is in test mode
volatile static bool test_mode;

//this variable keeps track of whether the pin is high or low.
//the value is switched during the state actions of the transmitter state machine
volatile static bool pin_high = false;

// The transmitter state machine generates a square wave output at the chosen
// frequency as set by transmitter_setFrequencyNumber(). The step counts for the
// frequencies are provided in filter.h

// Standard init function.
void transmitter_init(){
    //mio stuff for the pins:
    mio_init(false);
    mio_setPinAsOutput(TRANSMITTER_OUTPUT_PIN);

    //initialize to low state
    current_state = low_state;

    //set transmitting flag to false
    transmitting = false;

    //continuous mode off by default
    cmFlag = false;

    //set player transmit frequency
    transmitter_setFrequencyNumber(DEFAULT_PLAYER_FREQUENCY);

    //set elapsed ticks
    elapsed_ticks = 0;

    //default test_mode to false, switch to true if running transmitter_runTest()
    test_mode = false;
}

// Standard tick function.
void transmitter_tick(){
    //this variable determines the length of the pulse. Normally it's 20000 ticks, although in test mode, it's 200
    uint16_t tick_cnt = BURST_TICK_COUNT;
    if (test_mode) 
        tick_cnt = BURST_TICK_COUNT_TEST;
    
    //keeps track of ticks for frequency
    static uint16_t frequency_ticks = 0;
    static uint16_t frequency_tick_count = 0;

    //state transitions
    switch(current_state){
        case low_state:
            //start transmitting if fired or if continuous mode flag is enabled
            if(transmitting || cmFlag){
                //reset elapsed ticks for good measure
                elapsed_ticks = 0;
                frequency_ticks = 0;
                //set state to transmit state
                current_state = transmit_state;
                //get frequency of transmission
                frequency_tick_count = tx_frequency_ticks;
                //start transmitting low
                mio_writePin(TRANSMITTER_OUTPUT_PIN, TRANSMITTER_LOW_VALUE);
                pin_high = false;
            }
            break;
        case transmit_state:
            //only transition if the tick count is exceeds the tick count per burst
            if(elapsed_ticks >= tick_cnt){
                //set transmitting flag to false
                transmitting = false;
                //reset elapsed ticks
                elapsed_ticks = 0;
                frequency_ticks = 0;
                //set state to low state
                current_state = low_state;
                //finish transmitting low for good meausure
                mio_writePin(TRANSMITTER_OUTPUT_PIN, TRANSMITTER_LOW_VALUE);
                pin_high = false;
            }
            break;
    }
    //state actions
    switch(current_state){
        case low_state:
            //no actions
            break;
        case transmit_state:
            //increment elapsed ticks and frequency ticks
            elapsed_ticks++;
            frequency_ticks++;
            //once we reach the amount of ticks for 50% duty cycle signal, switch the output pin value
            if(frequency_ticks >= frequency_tick_count){
                frequency_ticks = 0;
                //if output is high, set it low and vice versa
                if(pin_high){
                    pin_high = false;
                    mio_writePin(TRANSMITTER_OUTPUT_PIN, TRANSMITTER_LOW_VALUE);
                }
                else{
                    pin_high = true;
                    mio_writePin(TRANSMITTER_OUTPUT_PIN, TRANSMITTER_HIGH_VALUE);
                }
            }
            break;
    }
}

// Activate the transmitter.
void transmitter_run(){
    transmitting = true;
}

// Returns true if the transmitter is still running.
bool transmitter_running(){
    return transmitting;
}

// Sets the frequency number. If this function is called while the
// transmitter is running, the frequency will not be updated until the
// transmitter stops and transmitter_run() is called again.
void transmitter_setFrequencyNumber(uint16_t frequencyNumber){
    tx_frequency_ticks = filter_frequencyTickTable[frequencyNumber] * DUTY_CYCLE;
}

// Returns the current frequency setting.
uint16_t transmitter_getFrequencyNumber(){
    uint16_t frequencyNumber;
    //loops through the tick table until it finds a matching value to return
    for(uint8_t i = 0; i < sizeof(filter_frequencyTickTable); i++){
        if((tx_frequency_ticks/DUTY_CYCLE) == filter_frequencyTickTable[i]){
            frequencyNumber = i;
            break;
        }
    }
    return frequencyNumber;
}

// Runs the transmitter continuously.
// if continuousModeFlag == true, transmitter runs continuously, otherwise, it
// transmits one burst and stops. To set continuous mode, you must invoke
// this function prior to calling transmitter_run(). If the transmitter is
// currently in continuous mode, it will stop running if this function is
// invoked with continuousModeFlag == false. It can stop immediately or wait
// until a 200 ms burst is complete. NOTE: while running continuously,
// the transmitter will only change frequencies in between 200 ms bursts.
void transmitter_setContinuousMode(bool continuousModeFlag){
    cmFlag = continuousModeFlag;
}

/******************************************************************************
***** Test Functions
******************************************************************************/

void transmitter_enableTestMode() {
    test_mode = true;
}

void transmitter_disableTestMode() {
    test_mode = false;
}
    
// Prints out the clock waveform to stdio. Terminates when BTN1 is pressed.
// Prints out one line of 1s and 0s that represent one period of the clock signal, in terms of ticks.
#define TRANSMITTER_TEST_TICK_PERIOD_IN_MS 10
#define BOUNCE_DELAY 5
void transmitter_runTest() {
  transmitter_init();                                     // init the transmitter.
  transmitter_enableTestMode();                         //enable test mode
  static uint16_t switchValue = 0;
  while (!(buttons_read() & BUTTONS_BTN1_MASK) && switchValue < 10) {         // Run continuously until BTN1 is pressed.
    // uint16_t switchValue = switches_read();               // Compute a safe number from the switches.
    transmitter_setFrequencyNumber(switchValue);          // set the frequency number based upon switch value.
    transmitter_run();                                    // Start the transmitter.
    while (transmitter_running()) {                       // Keep ticking until it is done.
      transmitter_tick();                                 // tick.
      utils_msDelay(TRANSMITTER_TEST_TICK_PERIOD_IN_MS);  // short delay between ticks.
    }
    switchValue++;
  }
  do {utils_msDelay(BOUNCE_DELAY);} while (buttons_read());
}
    

// Tests the transmitter in non-continuous mode.
// The test runs until BTN3 is pressed.
// To perform the test, connect the oscilloscope probe
// to the transmitter and ground probes on the development board
// prior to running this test. You should see about a 300 ms dead
// spot between 200 ms pulses.
// Should change frequency in response to the slide switches.
// Depends on the interrupt handler to call tick function.
void transmitter_runTestNoncontinuous() {
    transmitter_disableTestMode();
    transmitter_setContinuousMode(false);
    //run until button 3 is pressed
    while (!(buttons_read() & BUTTONS_BTN3_MASK)) {
        transmitter_setFrequencyNumber(switches_read() % FILTER_FREQUENCY_COUNT);   //set frequency number from switches
        transmitter_run();      //run transmitter
        //wait for transmitter to return false
        while(transmitter_running());
        utils_msDelay(TRANSMITTER_DELAY);   //also delay so signal shows up on oscillator
    }
    utils_msDelay(TEST_DELAY); //delay time between tests
    return;
}

// Tests the transmitter in continuous mode.
// To perform the test, connect the oscilloscope probe
// to the transmitter and ground probes on the development board
// prior to running this test.
// Transmitter should continuously generate the proper waveform
// at the transmitter-probe pin and change frequencies
// in response to changes in the slide switches.
// Test runs until BTN3 is pressed.
// Depends on the interrupt handler to call tick function.
void transmitter_runTestContinuous() {
    transmitter_disableTestMode();
    transmitter_setContinuousMode(true);
    //run until button 3 is pressed
    while (!(buttons_read() & BUTTONS_BTN3_MASK)) {
        transmitter_setFrequencyNumber(switches_read() % FILTER_FREQUENCY_COUNT);   //set frequency number from switches
        transmitter_run();      //run transmitter
        //wait for transmitter to return false
        while(transmitter_running()); 
    }
    utils_msDelay(TEST_DELAY); //delay time between tests
    return;
}