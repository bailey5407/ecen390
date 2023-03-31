#include "detector.h"
#include "buffer.h"
#include "interrupts.h"
#include "filter.h"
#include "lockoutTimer.h"
#include "hitLedTimer.h"
#include "transmitter.h"
#include <stdlib.h>
#include <stdio.h>
#include "invincibilityTimer.h"

#define ADC_MAX 4095
#define ADC_NORM_CONST 2
#define ADC_NORM_MIN 1

#define FILTER_REQUIRED_ELEMENTS 10
#define FILTER_COUNT 10
#define MEDIAN_VALUE 4

#define POWER_VAL_TEST 10
#define DISABLE_INTERRUPTS false
#define DEFAULT_FUDGE_FACTOR 750
#define DEBUG_SORT FALSE


/***************************** Constants *************************************/


static buffer_data_t rawAdcValue; // popped adc buffer value
static double scaledAdcValue; // scaled adc buffer value
static uint16_t elementCount; // amount of buffer elements
static uint8_t add_input_cnt; // filter add new input count 

// invocation count to keep track of times detector is invoked
static uint32_t invocation_cnt;

//keeps track of hits per frequency
static volatile detector_hitCount_t detector_hitArray[FILTER_COUNT];

//keep track of whether a hit is detected or not
static volatile bool hit_detected;
 
//frequency number of last valid hit
static volatile uint16_t freqLastValidHit;

//ignored frequencies array
static bool ignoredFreqArray[FILTER_COUNT];

//ignored frequencies array that never changes value after init
static bool savedFreqArray[FILTER_COUNT];

//keeps track of fudge factors for frequencies
static uint32_t fudgeFactors[FILTER_COUNT];

//array storing power values associated with each filter
static double currentPowerValues[FILTER_COUNT];

//array storing sorted playernumbers corresponding to power values (player with lowest power is index 0)
static uint8_t sortedPlayerPower[FILTER_COUNT];

//for ignoreFrequencies function. Keeps track of first run so that arrays are set correctly

/************************** Helper Functions **************************************/


//initialize fudge factors to default
static void fudgeFactors_init(){
    for(uint8_t i = 0; i < FILTER_COUNT; i++){
        fudgeFactors[i] = DEFAULT_FUDGE_FACTOR;
    }
    return;
}

//sets playerPower array values to 0-9 initially
static void initPlayerPower(){
    for(uint8_t i = 0; i < FILTER_COUNT; i++){
        sortedPlayerPower[i] = i;
    }
}

//populates currentPowerValues array with current powers from each filter in our bank
static void getPowerValues(){
    filter_getCurrentPowerValues(currentPowerValues);
    //set ignored frequency powers to 0:
    // for(uint16_t i = 0; i < FILTER_COUNT; i++){
    //     if(ignoredFreqArray[i]){
    //         filter_setCurrentPowerValue(i, 0);
    //     }
    // }
    return;
}

//swaps the indices (passed as arguments) in the sorted power array
static void sortedPower_swap(uint8_t index1, uint8_t index2){
    //save whatever is in index2 as temp
    uint8_t temp = sortedPlayerPower[index2];
    //put whatever is in index1 into the index2 spot
    sortedPlayerPower[index2] = sortedPlayerPower[index1];
    //put the saved value from index2 into the index1 spot
    sortedPlayerPower[index1] = temp;
    return;
}

//sorts the player numbers in ascending order in terms of hit power. Modifies sorted power values
//uses insertion sort algorithm
static void sortPlayerPower(){
    for(uint8_t i = 1; i < FILTER_COUNT; i++){
        while(i > 0 && currentPowerValues[sortedPlayerPower[i-1]] > currentPowerValues[sortedPlayerPower[i]]){
            sortedPower_swap(i,i-1);
            i = i - 1;
        }
    }
}

//runs detector algorithm and sets hit_detected flag
static void runDetectorAlgorithm(){
    //get player powers
    getPowerValues();    
    //sort player powers
    sortPlayerPower();
    //get the median value (or at least the fifth value)
    double median_power = currentPowerValues[sortedPlayerPower[MEDIAN_VALUE]];
    //compute a threshold with the median power and a fudge factor
    double power_threshold = median_power * DEFAULT_FUDGE_FACTOR;
    //find max power and if it exceeds threshold, we have a possible hit
    //find max valid power
    uint8_t maxValidPower_index = FILTER_COUNT - 1;
    //make sure player number is valid (not in our ignored frequency array)
    while(ignoredFreqArray[sortedPlayerPower[maxValidPower_index]] == true){
        maxValidPower_index--;
        if(maxValidPower_index > 100){
            printf("all frequencies are ignored\n");
            break;
        }
    }
    //get player number of max power
    uint16_t playerNumberMaxPower = sortedPlayerPower[maxValidPower_index];
    //get the power associated with that player number
    double power_val = currentPowerValues[playerNumberMaxPower];
    //if the power exceeds the threshold, set hit detected flag to true and remember player number of hit
    if (power_val >= power_threshold){
        hit_detected = true;
        freqLastValidHit = sortedPlayerPower[maxValidPower_index];
    }
    //else set hit detected flag to false
    else {
        detector_clearHit();
    } 
    return;
}


/****************************** Visible Functions ******************************************/


// Initialize the detector module.
// By default, all frequencies are considered for hits.
// Assumes the filter module is initialized previously.
void detector_init(void) {
    //initialize ignoredFreqArray all to false initially
    for(uint16_t i = 0; i < FILTER_COUNT; i++){
        ignoredFreqArray[i] = false;
    }
    //set ignored frequencies
    detector_setIgnoredFrequencies(ignoredFreqArray);
    //load savedFreqArray
    for(uint8_t i = 0; i < FILTER_COUNT; i++){
        savedFreqArray[i] = ignoredFreqArray[i];
    }
    fudgeFactors_init();
    //init playerPower array
    initPlayerPower();
    //set hit detected flag to false
    hit_detected = false;
    //set invocation count to 0
    invocation_cnt = 0;
    //initialize filter run count to 0
    add_input_cnt = 0;
    //flush buffer
    while(buffer_elements() != 0) buffer_pop();
}

// ignoredFreqArray is indexed by frequency number. If an element is set to true,
// the frequency will be ignored. Multiple frequencies can be ignored.
// Your shot frequency (based on the switches) is a good choice to ignore.
void detector_setIgnoredFrequencies(bool freqArray[]){
    for(uint16_t i = 0; i < FILTER_COUNT; i++){
        ignoredFreqArray[i] = freqArray[i];
        savedFreqArray[i] = freqArray[i];
    }
    return;
}

// Runs the entire detector: decimating FIR-filter, IIR-filters,
// power-computation, hit-detection. If interruptsCurrentlyEnabled = true,
// interrupts are running. If interruptsCurrentlyEnabled = false you can pop
// values from the ADC buffer without disabling interrupts. If
// interruptsCurrentlyEnabled = true, do the following:
// 1. disable interrupts.
// 2. pop the value from the ADC buffer.
// 3. re-enable interrupts.
// Ignore hits on frequencies specified with detector_setIgnoredFrequencies().
// Assumption: draining the ADC buffer occurs faster than it can fill.
void detector(bool interruptsCurrentlyEnabled) {
    //increment detector invocation count (for statistics)
    invocation_cnt++;
    //query the ADC Buffer to see how many elements it contains
    elementCount = buffer_elements();
    //loop through the following element count times
    for (uint16_t i = 0; i < elementCount; i++) {
        //if interrupts are enabled, temporarily disable them to safely pop a value from the buffer
        if (interruptsCurrentlyEnabled == true) {
            // briefly disable interrupts
            interrupts_disableArmInts();
            // pop element from ADC Buffer
            rawAdcValue = buffer_pop();
            // reenable interrupts
            interrupts_enableArmInts();
        }
        //if interrupts are disabled, simply pop a value from the buffer
        else {
            rawAdcValue = buffer_pop();
        }
        //scale ADC value
        scaledAdcValue = rawAdcValue / (0.5 * ADC_MAX) - ADC_NORM_MIN;
        // pass new input to FIR filter
        filter_addNewInput(scaledAdcValue);

        // keeps track of number of times filter_addNewInput has been invoked
        // once it has been invoked 10 times, we have enough data to run
        // our bank of filters
        add_input_cnt++;
        // if add new input has been called 10 times (number of elements required for filters to calculate a value)
        if (add_input_cnt == FILTER_REQUIRED_ELEMENTS) {
            // reset input count
            add_input_cnt = 0;
            // Call FIR Filter to anti-alias data input
            filter_firFilter();
            // run IIR and power calculations for all filters in bandpass bank
            for (uint16_t i = 0; i < FILTER_COUNT; i++) {
                // run IIR for each player number
                filter_iirFilter(i);
                // computer power for that IIR
                filter_computePower(i, false, false);
            }
            // if lockout timer is not running, run hit detection algorithm
            if (!lockoutTimer_running() && !invincibilityTimer_running()){
                //run algorithm
                runDetectorAlgorithm();
                //if the hit was valid
                if(detector_hitDetected()){
                    // start the lockout and hit led timers
                    lockoutTimer_start();
                    hitLedTimer_start();
                    // increment element at index of freq of iir filter output where hit was detected
                    detector_hitArray[freqLastValidHit]++;
                }
            }
        }
    }
}

// Returns true if a hit was detected
bool detector_hitDetected(void) {
    return hit_detected;
}

// Returns the frequency number that caused the hit.
uint16_t detector_getFrequencyNumberOfLastHit(void){
    return freqLastValidHit;
}

// Clear the detected hit once you have accounted for it.
void detector_clearHit(void){
    hit_detected = false;
}

// Ignore all hits. Used to provide some limited invincibility in some game
// modes. The detector will ignore all hits if the flag is true, otherwise will
// respond to hits normally.
void detector_ignoreAllHits(bool flagValue) {
    if(flagValue) printf("ignoring all hits\n");
    else printf("NOT ignoring all hits\n");
    //if flag true: set ignoredFreqArray values to true (ignore everything)
    if(flagValue) for(uint16_t i = 0; i < FILTER_COUNT; i++) ignoredFreqArray[i] = true;
    //if flag value false, reset ignoredFreqArray values to saved ignored values
    else for(uint16_t i = 0; i < FILTER_COUNT; i++) ignoredFreqArray[i] = savedFreqArray[i];
    return;
}

// Get the current hit counts.
// Copy the current hit counts into the user-provided hitArray
// using a for-loop.
void detector_getHitCounts(detector_hitCount_t hitArray[]) {
    for (uint16_t i = 0; i < FILTER_COUNT; i++) {
        hitArray[i] = detector_hitArray[i];
    }
}

// Allows the fudge-factor index to be set externally from the detector.
// The actual values for fudge-factors is stored in an array found in detector.c
void detector_setFudgeFactorIndex(uint32_t factor){
    for(uint8_t i = 0; i < FILTER_COUNT; i++){
        fudgeFactors[i] = factor;
    }
    return;
}

// Returns the detector invocation count.
// The count is incremented each time detector is called.
// Used for run-time statistics.
uint32_t detector_getInvocationCount(void) {
    return invocation_cnt;
}

/******************************************************
******************** Test Routines ********************
******************************************************/

// Students implement this as part of Milestone 3, Task 3.
// Create two sets of power values and call your hit detection algorithm
// on each set. With the same fudge factor, your hit detect algorithm
// should detect a hit on the first set and not detect a hit on the second.
void detector_runTest(void) {
    //load dummy values into power array
    filter_setCurrentPowerValue(0, 150);
    filter_setCurrentPowerValue(1, 20);
    filter_setCurrentPowerValue(2, 40);
    filter_setCurrentPowerValue(3, 10);
    filter_setCurrentPowerValue(4, 15);
    filter_setCurrentPowerValue(5, 30);
    filter_setCurrentPowerValue(6, 35);
    filter_setCurrentPowerValue(7, 15);
    filter_setCurrentPowerValue(8, 25);
    filter_setCurrentPowerValue(9, 80);
    //run the detector algorithm
    detector(false);
    //print out the player number corresponding to the detected hit
    printf("A hit was detected by player %d\n", freqLastValidHit);
}