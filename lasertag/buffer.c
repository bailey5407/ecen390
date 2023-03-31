#include "buffer.h"

#define BUFFER_SIZE 32768

// This implements a dedicated circular buffer for storing values
// from the ADC until they are read and processed by the detector.
// The function of the buffer is similar to a queue or FIFO.

// Type of elements in the buffer.
typedef uint32_t buffer_data_t;

//struct for buffer elements (we only need one buffer so this works)
typedef struct{
    uint32_t indexIn; // Points to the next open slot.
    uint32_t indexOut; // Points to the next element to be removed.
    uint32_t elementCount; // Number of elements in the buffer.
    uint32_t size;  //maximum size of the buffer
    buffer_data_t data[BUFFER_SIZE]; // Values are stored here.
} buffer_t;

volatile static buffer_t buffer;

// Initialize the buffer to empty.
void buffer_init(void){
    //set element count to 0
    buffer.elementCount = 0;
    //set size to buffer count constant
    buffer.size = BUFFER_SIZE;
    //set indexIn and indexOut to 0 (index of data)
    buffer.indexIn = 0;
    buffer.indexOut = 0;
    //initialize every entry in the array to 0
    for(uint16_t i = 0; i < BUFFER_SIZE; i++){
        buffer.data[i] = 0;
    }
}

// Add a value to the buffer. Overwrite the oldest value if full.
void buffer_pushover(buffer_data_t value){
    //guard clause: array is full. Overwrite oldest value
    if(buffer.elementCount == buffer.size){
        //increment indexOut and rollover if necessary
        if(++buffer.indexOut == buffer.size) buffer.indexOut = 0;
    }
    //otherwise increment element count
    else buffer.elementCount++;
    //add value to array
    buffer.data[buffer.indexIn] = value;
    //increment indexIn and rollover if necessary
    if(++buffer.indexIn == buffer.size) buffer.indexIn = 0;
    return;
}

// Remove a value from the buffer. Return zero if empty.
buffer_data_t buffer_pop(void){
    //guard clause: return 0 if empty
    if(buffer.elementCount == 0) return 0;

    //save value to return at indexOut
    buffer_data_t value = buffer.data[buffer.indexOut];
    //decrement elementCount
    buffer.elementCount--;
    //increment indexOut and rollover if necessary
    if(++buffer.indexOut == buffer.size) buffer.indexOut = 0;
    return value;
}

// Return the number of elements in the buffer.
uint32_t buffer_elements(void){
    return buffer.elementCount;
}

// Return the capacity of the buffer in elements.
uint32_t buffer_size(void){
    return buffer.size;
}