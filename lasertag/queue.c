#include "queue.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


// Limit the size of the statically-allocated queue name.
#define QUEUE_MAX_NAME_SIZE 50

// Return this when queue_pop(), queue_readElementAt() needs to return something
// during an error condition.
#define QUEUE_RETURN_ERROR_VALUE ((queue_data_t)0)

#define OVERFLOW_ERROR_MSG "ERROR: OVERFLOW\n"
#define UNDERFLOW_ERROR_MSG "ERROR: UNDERFLOW\n"
#define INVALID_INDEX_ERROR_MSG "Error: INVALID INDEX\n"

// Allocates memory for the queue (the data* pointer) and initializes all
// parts of the data structure. Prints out an error message if malloc() fails
// and calls assert(false) to print-out line-number information and die.
// The queue is empty after initialization. To fill the queue with known
// values (e.g. zeros), call queue_overwritePush() up to queue_size() times.
void queue_init(queue_t *q, queue_size_t size, const char *name){
    //set the data pointer to an address of allocated memory
    q->data = malloc(size * sizeof(queue_data_t));

    //set the indices to the same initial value of zero
    //(indices are used as address offsets)
    q->indexIn = q->indexOut = 0;

    //initialize element count to zero
    q->elementCount = 0;

    //initiliaze size to the size determined by the function argument
    q->size = size;

    //set the flags to false by default
    q->underflowFlag = false;
    q->overflowFlag = false;

    //set the name of the queue determined by the function argument
    for(uint8_t i = 0; i < QUEUE_MAX_NAME_SIZE; i++){
        q->name[i] = name[i];
    } 
}

// Get the user-assigned name for the queue.
const char *queue_name(queue_t *q){
    return q->name;
}

// Returns the capacity of the queue.
queue_size_t queue_size(queue_t *q){
    return q->size;
}

// Returns true if the queue is full.
bool queue_full(queue_t *q){
    if(q->elementCount == q->size){
        return true;
    }
    else return false;
}

// Returns true if the queue is empty.
bool queue_empty(queue_t *q){
    if(q->elementCount == 0){
        return true;
    }
    else return false;
}

// If the queue is not full, pushes a new element into the queue and clears the
// underflowFlag. IF the queue is full, set the overflowFlag, print an error
// message and DO NOT change the queue.
void queue_push(queue_t *q, queue_data_t value){
    //guard clause for overflow. Set flag and print error message
    if(queue_full(q)){
        q->overflowFlag = true;
        printf(OVERFLOW_ERROR_MSG);
        return;
    }
    //increment the number of elements in our queue
    q->elementCount++;
    //set the data at our indexIn to the value passed by the function argument
    *(q->data + q->indexIn) = value;
    //increment our index variable. If the end of our index is reached, reset
    //the index variable to zero
    q->indexIn++;
    if(q->indexIn >= q->size){
        q->indexIn = 0;
    }
    
    //set underflow flag to false
    q->underflowFlag = false;

    return;
}

// If the queue is not empty, remove and return the oldest element in the queue.
// If the queue is empty, set the underflowFlag, print an error message, and DO
// NOT change the queue.
queue_data_t queue_pop(queue_t *q){
    //guard clause for underflow. Set flag and print error message
    if(queue_empty(q)){
        q->underflowFlag = true;
        printf(UNDERFLOW_ERROR_MSG);
        return QUEUE_RETURN_ERROR_VALUE;
    }

    queue_data_t data;

    //decrement the number of elements in our queue
    q->elementCount--;
    //get the data stored at the indexOut of our queue
    data = *(q->data + q->indexOut);
    //increment our index variable, resetting to zero if we reach the end
    //of our index
    q->indexOut++;
    if(q->indexOut >= q->size){
        q->indexOut = 0;
    }

    //set overflow flag to false
    q->overflowFlag = false;

    return data;
}

// If the queue is full, call queue_pop() and then call queue_push().
// If the queue is not full, just call queue_push().
void queue_overwritePush(queue_t *q, queue_data_t value){
    //if queue is full, pop queue
    if(queue_full(q)){
        queue_pop(q);
    }
    //push to queue
    queue_push(q, value);
    return;
}

// Provides random-access read capability to the queue.
// Low-valued indexes access older queue elements while higher-value indexes
// access newer elements (according to the order that they were added). Print a
// meaningful error message if an error condition is detected.
queue_data_t queue_readElementAt(queue_t *q, queue_index_t index){
    //Invalid Index Error
    if(index >= q->elementCount){
        printf(INVALID_INDEX_ERROR_MSG);
        return QUEUE_RETURN_ERROR_VALUE;
    }
    return q->data[(q->indexOut + index) % q->size];
}

// Returns a count of the elements currently contained in the queue.
queue_size_t queue_elementCount(queue_t *q){
    return q->elementCount;
}

// Returns true if an underflow has occurred (queue_pop() called on an empty
// queue).
bool queue_underflow(queue_t *q){
    return q->underflowFlag;
}

// Returns true if an overflow has occurred (queue_push() called on a full
// queue).
bool queue_overflow(queue_t *q){
    return q->overflowFlag;
}

// Frees the storage that you malloc'd before.
void queue_garbageCollect(queue_t *q){
    free(q->data);
}
