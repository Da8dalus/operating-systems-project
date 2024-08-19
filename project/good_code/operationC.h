#ifndef _OPERATION_H
#define _OPERATION_H

#include "process.h"

// Operation types
typedef enum {
    EXPIRE,
    CONTINUED_BURST,
    BURST,
    ENQUEUE,
    IO_END,
    CPU_UNBUSY,
    ARRIVE,
    DEQUEUE
} OpType;

typedef struct {
    OpType type;
    unsigned int time;
    Process *process;
} Operation;

// Constructor function for Operation
Operation createOperation(OpType type, unsigned int time, Process *process);

// Comparator function for sorting Operations
int operationComparator(const void *op1, const void *op2);

#endif
