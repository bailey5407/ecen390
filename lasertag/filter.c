#include "filter.h"
#include <stdint.h>

#define FIR_FILTER_LENGTH 81
#define BANDPASS_FILTERS_COUNT 10
#define BANDPASS_A_COEFFICIENT_AMOUNT 11
#define BANDPASS_B_COEFFICIENT_AMOUNT 11

#define XQUEUE_SIZE 81
#define YQUEUE_SIZE 11
#define ZQUEUE_SIZE 10
#define OUTPUTQUEUE_SIZE FILTER_INPUT_PULSE_WIDTH
#define QUEUE_INIT_VALUE 0

#define POWER_INIT_VAL 0
#define POWER_SIZE 10

#define NAME_1 "zQueue_1"
#define NAME_2 "zQueue_2"
#define NAME_3 "zQueue_3"
#define NAME_4 "zQueue_4"
#define NAME_5 "zQueue_5"
#define NAME_6 "zQueue_6"
#define NAME_7 "zQueue_7"
#define NAME_8 "zQueue_8"
#define NAME_9 "zQueue_9"
#define NAME_0 "zQueue_0"

#define ONAME_1 "outputQueue_1"
#define ONAME_2 "outputQueue_2"
#define ONAME_3 "outputQueue_3"
#define ONAME_4 "outputQueue_4"
#define ONAME_5 "outputQueue_5"
#define ONAME_6 "outputQueue_6"
#define ONAME_7 "outputQueue_7"
#define ONAME_8 "outputQueue_8"
#define ONAME_9 "outputQueue_9"
#define ONAME_0 "outputQueue_0"

// static array to store the power output of each bandpass filter
static double currentPowerValue[POWER_SIZE];

// xQueue to store input from receiver board
static queue_t xQueue;

// yQueue to store output from FIR filter
static queue_t yQueue;

// zQueues to store outputs of bandpass filters for history
static queue_t zQueues[BANDPASS_FILTERS_COUNT];

// outputQueueus to store output of bandpass filters for power calculation
static queue_t outputQueues[BANDPASS_FILTERS_COUNT];

// Array to store the FIR coefficients for the anti aliasing filter
const static double FIR_Coefficients[FIR_FILTER_LENGTH] = {4.3579622275120866e-04,   2.7155425450406482e-04,   6.3039002645022389e-05,  
-1.9349227837935689e-04,  -4.9526428865281219e-04,  -8.2651441681321381e-04,  -1.1538970332472540e-03,  -1.4254746936265955e-03, 
-1.5744703111426981e-03,  -1.5281041447445794e-03,  -1.2208092333090719e-03,  -6.1008312441271589e-04,   3.0761698758506020e-04,
1.4840192333212628e-03,   2.8123077568332064e-03,   4.1290616416556000e-03,  5.2263464670258821e-03,   5.8739882867061598e-03,   
5.8504032099208096e-03,   4.9787419333799775e-03,   3.1637974805960069e-03,   4.2435139609132765e-04,  -3.0844289197247210e-03,
-7.0632027332701800e-03, -1.1078458037608587e-02,  -1.4591395057493114e-02,  -1.7004337345765962e-02,  -1.7720830774014484e-02,
-1.6213409845727566e-02, -1.2091458677988302e-02,  -5.1609257765542595e-03,   4.5319860006883522e-03,   1.6679627700682677e-02,
3.0718365411587255e-02,   4.5861875593064996e-02,   6.1160185621895728e-02,   7.5579213982547147e-02,   8.8092930943210607e-02,
9.7778502396672365e-02,   1.0390414346016495e-01,  1.0600000000000000e-01,   1.0390414346016495e-01,   9.7778502396672365e-02,
8.8092930943210607e-02,   7.5579213982547147e-02,   6.1160185621895728e-02,   4.5861875593064996e-02,   3.0718365411587255e-02,
1.6679627700682677e-02,   4.5319860006883522e-03,  -5.1609257765542595e-03,  -1.2091458677988302e-02,  -1.6213409845727566e-02,
-1.7720830774014484e-02,  -1.7004337345765962e-02,  -1.4591395057493114e-02,  -1.1078458037608587e-02,  -7.0632027332701800e-03,
-3.0844289197247210e-03,  4.2435139609132765e-04,   3.1637974805960069e-03,   4.9787419333799775e-03,   5.8504032099208096e-03,
5.8739882867061598e-03,   5.2263464670258821e-03,   4.1290616416556000e-03,   2.8123077568332064e-03,   1.4840192333212628e-03,
3.0761698758506020e-04,  -6.1008312441271589e-04,  -1.2208092333090719e-03,  -1.5281041447445794e-03,  -1.5744703111426981e-03,
-1.4254746936265955e-03, -1.1538970332472540e-03,  -8.2651441681321381e-04,  -4.9526428865281219e-04,  -1.9349227837935689e-04,
6.3039002645022389e-05,   2.7155425450406482e-04,   4.3579622275120866e-04};

// Array to store the IIR A coefficients for the IIR bandpass filters
const static double IIR_A_Coefficients[BANDPASS_FILTERS_COUNT][BANDPASS_A_COEFFICIENT_AMOUNT] = {
   {1.0000000000000000e+00,  -5.9637727070164033e+00,   1.9125339333078259e+01,  -4.0341474540744201e+01,   6.1537466875368885e+01,
     -7.0019717951472273e+01,   6.0298814235238943e+01,  -3.8733792862566347e+01,   1.7993533279581083e+01,  -5.4979061224867731e+00,   9.0332828533799669e-01},
   {1.0000000000000000e+00,  -4.6377947119071452e+00,   1.3502215749461570e+01,  -2.6155952405269748e+01,   3.8589668330738313e+01,
     -4.3038990303252582e+01,   3.7812927599537062e+01,  -2.5113598088113733e+01,   1.2703182701888057e+01,  -4.2755083391143360e+00,   9.0332828533799858e-01},
   {1.0000000000000000e+00,  -3.0591317915750946e+00,   8.6417489609637563e+00,  -1.4278790253808850e+01,   2.1302268283304322e+01,
     -2.2193853972079253e+01,   2.0873499791105466e+01,  -1.3709764520609413e+01,   8.1303553577931815e+00,  -2.8201643879900566e+00,   9.0332828533800247e-01},
   {1.0000000000000000e+00,  -1.4071749185996751e+00,   5.6904141470697454e+00,  -5.7374718273676208e+00,   1.1958028362868866e+01,
     -8.5435280598354346e+00,   1.1717345583835906e+01,  -5.5088290876998345e+00,   5.3536787286077301e+00,  -1.2972519209655502e+00,   9.0332828533799225e-01},
   {1.0000000000000000e+00,   8.2010906117760340e-01,   5.1673756579268613e+00,   3.2580350909220939e+00,   1.0392903763919197e+01,
      4.8101776408669110e+00,   1.0183724507092514e+01,   3.1282000712126781e+00,   4.8615933365572035e+00,   7.5604535083144997e-01,   9.0332828533800102e-01},
   {1.0000000000000000e+00,   2.7080869856154464e+00,   7.8319071217995475e+00,   1.2201607990980694e+01,   1.8651500443681531e+01,
      1.8758157568004435e+01,   1.8276088095998901e+01,   1.1715361303018808e+01,   7.3684394621252913e+00,   2.4965418284511678e+00,   9.0332828533799581e-01},
   {1.0000000000000000e+00,   4.9479835250075874e+00,   1.4691607003177591e+01,   2.9082414772101028e+01,   4.3179839108869274e+01,
      4.8440791644688801e+01,   4.2310703962394257e+01,   2.7923434247706368e+01,   1.3822186510470974e+01,   4.5614664160654215e+00,   9.0332828533799658e-01},
   {1.0000000000000000e+00,   6.1701893352279864e+00,   2.0127225876810343e+01,   4.2974193398071705e+01,   6.5958045321253508e+01,
      7.5230437667866667e+01,   6.4630411355739938e+01,   4.1261591079244184e+01,   1.8936128791950566e+01,   5.6881982915180398e+00,   9.0332828533799969e-01},
   {1.0000000000000000e+00,   7.4092912870072354e+00,   2.6857944460290113e+01,   6.1578787811202183e+01,   9.8258255839887198e+01,
      1.1359460153696280e+02,   9.6280452143025911e+01,   5.9124742025776264e+01,   2.5268527576524143e+01,   6.8305064480742885e+00,   9.0332828533799747e-01},
   {1.0000000000000000e+00,   8.5743055776347692e+00,   3.4306584753117896e+01,   8.4035290411037096e+01,   1.3928510844056828e+02,
      1.6305115418161637e+02,   1.3648147221895800e+02,   8.0686288623299845e+01,   3.2276361903872157e+01,   7.9045143816244847e+00,   9.0332828533799858e-01}
};

// Array to store the IIR B coefficients for the IIR bandpass filters
const static double IIR_B_Coefficients[BANDPASS_FILTERS_COUNT][BANDPASS_B_COEFFICIENT_AMOUNT] = {
    {9.0928661148193053e-10,   0.0000000000000000e+00,  -4.5464330574096529e-09,   0.0000000000000000e+00,   9.0928661148193057e-09,
       0.0000000000000000e+00,  -9.0928661148193057e-09,   0.0000000000000000e+00,   4.5464330574096529e-09,   0.0000000000000000e+00,  -9.0928661148193053e-10},
    {9.0928661148191792e-10,   0.0000000000000000e+00,  -4.5464330574095892e-09,   0.0000000000000000e+00,   9.0928661148191783e-09,
       0.0000000000000000e+00,  -9.0928661148191783e-09,   0.0000000000000000e+00,   4.5464330574095892e-09,   0.0000000000000000e+00,  -9.0928661148191792e-10},
    {9.0928661148189351e-10,   0.0000000000000000e+00,  -4.5464330574094676e-09,   0.0000000000000000e+00,   9.0928661148189351e-09,
       0.0000000000000000e+00,  -9.0928661148189351e-09,   0.0000000000000000e+00,   4.5464330574094676e-09,   0.0000000000000000e+00,  -9.0928661148189351e-10},
    {9.0928661148202731e-10,   0.0000000000000000e+00,  -4.5464330574101368e-09,   0.0000000000000000e+00,   9.0928661148202735e-09,
       0.0000000000000000e+00,  -9.0928661148202735e-09,   0.0000000000000000e+00,   4.5464330574101368e-09,   0.0000000000000000e+00,  -9.0928661148202731e-10},
    {9.0928661148198792e-10,   0.0000000000000000e+00,  -4.5464330574099399e-09,   0.0000000000000000e+00,   9.0928661148198798e-09,
       0.0000000000000000e+00,  -9.0928661148198798e-09,   0.0000000000000000e+00,   4.5464330574099399e-09,   0.0000000000000000e+00,  -9.0928661148198792e-10},
    {9.0928661148205316e-10,   0.0000000000000000e+00,  -4.5464330574102658e-09,   0.0000000000000000e+00,   9.0928661148205316e-09,
       0.0000000000000000e+00,  -9.0928661148205316e-09,   0.0000000000000000e+00,   4.5464330574102658e-09,   0.0000000000000000e+00,  -9.0928661148205316e-10},
    {9.0928661148200353e-10,   0.0000000000000000e+00,  -4.5464330574100176e-09,   0.0000000000000000e+00,   9.0928661148200353e-09,
       0.0000000000000000e+00,  -9.0928661148200353e-09,   0.0000000000000000e+00,   4.5464330574100176e-09,   0.0000000000000000e+00,  -9.0928661148200353e-10},
    {9.0928661148183447e-10,   0.0000000000000000e+00,  -4.5464330574091723e-09,   0.0000000000000000e+00,   9.0928661148183445e-09,
       0.0000000000000000e+00,  -9.0928661148183445e-09,   0.0000000000000000e+00,   4.5464330574091723e-09,   0.0000000000000000e+00,  -9.0928661148183447e-10},
    {9.0928661148201087e-10,   0.0000000000000000e+00,  -4.5464330574100540e-09,   0.0000000000000000e+00,   9.0928661148201081e-09,
       0.0000000000000000e+00,  -9.0928661148201081e-09,   0.0000000000000000e+00,   4.5464330574100540e-09,   0.0000000000000000e+00,  -9.0928661148201087e-10},
    {9.0928661148200384e-10,   0.0000000000000000e+00,  -4.5464330574100193e-09,   0.0000000000000000e+00,   9.0928661148200386e-09,
       0.0000000000000000e+00,  -9.0928661148200386e-09,   0.0000000000000000e+00,   4.5464330574100193e-09,   0.0000000000000000e+00,  -9.0928661148200384e-10}
};

// Init function for xQueue
void xQueue_init(){
    queue_init(&xQueue, XQUEUE_SIZE, "xQueue");
    for(uint8_t i = 0; i < XQUEUE_SIZE; i++){
        queue_overwritePush(&xQueue, QUEUE_INIT_VALUE);
    }
}

// Init function for yQueue
void yQueue_init(){
    queue_init(&yQueue, YQUEUE_SIZE, "yQueue");
    for(uint8_t i = 0; i < YQUEUE_SIZE; i++){
        queue_overwritePush(&yQueue, QUEUE_INIT_VALUE);
    }
}

// Init function for zQueues array
void zQueues_init(){
    //initializing zQueues and filling them with 0s
    for(uint8_t i = 0; i < BANDPASS_FILTERS_COUNT; i++){
        char* name;
        // switch statement for naming scheme
        switch(i){
            case 0: 
                name = NAME_0;
                break;
            case 1:
                name = NAME_1;
                break;
            case 2: 
                name = NAME_2;
                break;
            case 3:
                name = NAME_3;
                break;
            case 4: 
                name = NAME_4;
                break;
            case 5:
                name = NAME_5;
                break;
            case 6: 
                name = NAME_6;
                break;
            case 7:
                name = NAME_7;
                break;
            case 8:
                name = NAME_8;
                break;
            case 9:
                name = NAME_9;
                break;
        }
        queue_init(&zQueues[i], ZQUEUE_SIZE, name);
        for(uint8_t j = 0; j < ZQUEUE_SIZE; j++){
            queue_overwritePush(&zQueues[i], QUEUE_INIT_VALUE);
        }
    }
}

// Init function for outputQueues array
void outputQueues_init(){
    //initializing outputQueues and filling them with 0s
    for(uint8_t i = 0; i < BANDPASS_FILTERS_COUNT; i++){
        char* name;
        // switch statement for naming scheme
        switch(i){
            case 0: 
                name = ONAME_0;
                break;
            case 1:
                name = ONAME_1;
                break;
            case 2: 
                name = ONAME_2;
                break;
            case 3:
                name = ONAME_3;
                break;
            case 4: 
                name = ONAME_4;
                break;
            case 5:
                name = ONAME_5;
                break;
            case 6: 
                name = ONAME_6;
                break;
            case 7:
                name = ONAME_7;
                break;
            case 8:
                name = ONAME_8;
                break;
            case 9:
                name = ONAME_9;
                break;
        }
        queue_init(&outputQueues[i], OUTPUTQUEUE_SIZE, name);
        for(uint8_t j = 0; j < OUTPUTQUEUE_SIZE; j++){
            queue_overwritePush(&outputQueues[i], QUEUE_INIT_VALUE);
        }
    }
}

// Must call this prior to using any filter functions.
void filter_init(){
    //initialize each queue and fill it with zeros
    xQueue_init();
    yQueue_init();
    zQueues_init();
    outputQueues_init();
}

// Use this to copy an input into the input queue of the FIR-filter (xQueue).
void filter_addNewInput(double x){
    queue_overwritePush(&xQueue, x);   
}

// Invokes the FIR-filter. Input is contents of xQueue.
// Output is returned and is also pushed on to yQueue.
double filter_firFilter(){
    queue_data_t output = 0;
    //calculate the filter output using all values of the x queue and the FIR coefficients
    for(uint32_t i = 0; i < XQUEUE_SIZE; i++){
        output = output + FIR_Coefficients[i] * queue_readElementAt(&xQueue, i);
    }
    //write the output to the y queue and return it
    queue_overwritePush(&yQueue, output);
    return output;
}

// Use this to invoke a single iir filter. Input comes from yQueue.
// Output is returned and is also pushed onto zQueue[filterNumber].
double filter_iirFilter(uint16_t filterNumber){
    queue_data_t output;
    queue_data_t term_1 = 0;
    queue_data_t term_2 = 0;
    //calculate term 1
    for(uint8_t i = 0; i < YQUEUE_SIZE; i++){
        term_1 = IIR_B_Coefficients[filterNumber][i] * queue_readElementAt(&yQueue, i);
    }
    //calculate term 2
    for(uint8_t i = 0; i < ZQUEUE_SIZE; i++){
        term_2 = IIR_A_Coefficients[filterNumber][i+1] * queue_readElementAt(&zQueues[filterNumber], i);
    }
    //output is difference of term 1 and term 2
    output = term_1 - term_2;
    //push output to the zQueue and outputQueue. Also return it
    queue_overwritePush(&zQueues[filterNumber], output);
    queue_overwritePush(&outputQueues[filterNumber], output);
    return output;
}

// Use this to compute the power for values contained in an outputQueue.
// If force == true, then recompute power by using all values in the
// outputQueue. This option is necessary so that you can correctly compute power
// values the first time. After that, you can incrementally compute power values
// by:
// 1. Keeping track of the power computed in a previous run, call this
// prev-power.
// 2. Keeping track of the oldest outputQueue value used in a previous run, call
// this oldest-value.
// 3. Get the newest value from the output queue, call this newest-value.
// 4. Compute new power as: prev-power - (oldest-value * oldest-value) +
// (newest-value * newest-value). Note that this function will probably need an
// array to keep track of these values for each of the 10 output queues.
double filter_computePower(uint16_t filterNumber, bool forceComputeFromScratch, bool debugPrint) {
    double power = POWER_INIT_VAL;
    // outputqueue of certain filternumber
    queue_t* outputQueue = filter_getIirOutputQueue(filterNumber);

    // if force = true, compute power by summing the square of the voltages in outputqueue
    if (forceComputeFromScratch == true) {
        for (int i=0; i < OUTPUTQUEUE_SIZE; i++) {
            double voltage = queue_readElementAt(outputQueue, i);
            power += voltage*voltage;
        }
        filter_setCurrentPowerValue(filterNumber, power);
        return power;
    }
    
    //1. Keep track of the power computed in previous run
    queue_data_t prev_power = filter_getCurrentPowerValue(filterNumber);

    //2. Keep track of the oldest outputQueue value used in previous run
    queue_data_t oldest_value = queue_readElementAt(outputQueue, outputQueue->indexOut);
    
    //3. Get the newest value from the output queue
    queue_data_t newest_value = queue_readElementAt(outputQueue,outputQueue->indexIn);

    //4. compute new_power
    power = prev_power - (oldest_value * oldest_value) + (newest_value * newest_value);

    // set power value for this filternumber
    filter_setCurrentPowerValue(filterNumber, power);
    return power;
}

// Returns the last-computed output power value for the IIR filter
// [filterNumber].
double filter_getCurrentPowerValue(uint16_t filterNumber) {
    return currentPowerValue[filterNumber];
}

// Sets a current power value for a specific filter number.
// Useful in testing the detector.
void filter_setCurrentPowerValue(uint16_t filterNumber, double value) {
    currentPowerValue[filterNumber] = value;
}

// Get a copy of the current power values.
// This function copies the already computed values into a previously-declared
// array so that they can be accessed from outside the filter software by the
// detector. Remember that when you pass an array into a C function, changes to
// the array within that function are reflected in the returned array.
void filter_getCurrentPowerValues(double powerValues[]) {
    for(uint8_t i = 0; i < POWER_SIZE; i++){
        powerValues[i] = currentPowerValue[i];
    }
}

// Using the previously-computed power values that are currently stored in
// currentPowerValue[] array, copy these values into the normalizedArray[]
// argument and then normalize them by dividing all of the values in
// normalizedArray by the maximum power value contained in currentPowerValue[].
// The pointer argument indexOfMaxValue is used to return the index of the
// maximum value. If the maximum power is zero, make sure to not divide by zero
// and that *indexOfMaxValue is initialized to a sane value (like zero).
void filter_getNormalizedPowerValues(double normalizedArray[], uint16_t *indexOfMaxValue) {
    double array_max = 0;
    indexOfMaxValue = 0;
    // find max in currentPowerValue[] and copy contents into normalized array
    for (uint16_t i=0; i < BANDPASS_FILTERS_COUNT; i++) {
        normalizedArray[i] = currentPowerValue[i];
        //if the new value is greater than our current max, set current max to value
        if (array_max < currentPowerValue[i]) {
            array_max = currentPowerValue[i];
            indexOfMaxValue = &i;
        }
    }
    // normalize values in normalized array
    for (uint8_t i=0; i < BANDPASS_FILTERS_COUNT; i++) {
        if (array_max != 0) {
            normalizedArray[i] /= array_max;
        }
    }      
}

/******************************************************************************
***** Verification-Assisting Functions
***** External test functions access the internal data structures of filter.c
***** via these functions. They are not used by the main filter functions.
******************************************************************************/

// Returns the array of FIR coefficients.
const double *filter_getFirCoefficientArray(){
    return FIR_Coefficients;
}

// Returns the number of FIR coefficients.
uint32_t filter_getFirCoefficientCount(){
    return FIR_FILTER_LENGTH;
}

// Returns the array of coefficients for a particular filter number.
const double *filter_getIirACoefficientArray(uint16_t filterNumber){
    return IIR_A_Coefficients[filterNumber];
}

// Returns the number of A coefficients.
uint32_t filter_getIirACoefficientCount(){
    return BANDPASS_A_COEFFICIENT_AMOUNT; 
}

// Returns the array of coefficients for a particular filter number.
const double *filter_getIirBCoefficientArray(uint16_t filterNumber){
    return IIR_B_Coefficients[filterNumber];
}

// Returns the number of B coefficients.
uint32_t filter_getIirBCoefficientCount(){
    return BANDPASS_B_COEFFICIENT_AMOUNT;
}

// Returns the size of the yQueue.
uint32_t filter_getYQueueSize(){
    return YQUEUE_SIZE;
}

// Returns the decimation value.
uint16_t filter_getDecimationValue(){
    return FILTER_FIR_DECIMATION_FACTOR;
}

// Returns the address of xQueue.
queue_t *filter_getXQueue(){
    return &xQueue;
}

// Returns the address of yQueue.
queue_t *filter_getYQueue(){
    return &yQueue;
}

// Returns the address of zQueue for a specific filter number.
queue_t *filter_getZQueue(uint16_t filterNumber){
    return &zQueues[filterNumber];
}

// Returns the address of the IIR output-queue for a specific filter-number.
queue_t *filter_getIirOutputQueue(uint16_t filterNumber){
    return &outputQueues[filterNumber];
}
