#ifndef _PROCESS_H
#define _PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "next_exp.h"

typedef struct {
    char *id;
    unsigned int (*bursts)[2]; // Array of pairs (CPU time, IO time)
    int type; // 1 for I/O, 0 for CPU
    unsigned int arrivalTime;
    size_t burstIndex;
    unsigned int currentBurstElapsed;
    unsigned int readyQueueAddTime;
    unsigned int burstStartTime;
    unsigned int tau;
    unsigned int totalPreempted;
    int remainingTime;
} Process;

// Constructor equivalent in C
void createProcess(Process *p, const char *id, int type, unsigned int arrivalTime, unsigned int numBursts, double lambda, double ceiling);

// Print method
void printProcess(const Process *p, int verbose);

// Burst time accessor functions
void getBurstTime(const Process *p, unsigned int *cpuTime, unsigned int *ioTime);
unsigned int getNumBursts(const Process *p);
unsigned int getCurrentBurstTotalCPU(const Process *p);
unsigned int getCPUTimeRemaining(const Process *p);
unsigned int getIOTime(const Process *p);

// Comparator function for SJF and SRT algorithms
int sjfSrtProcSort(const void *p1, const void *p2);

#endif
