#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

typedef struct {
    char *id;
    int numBursts;
    int **cpu_io_bursts;
    bool cpu_bound;
    double lambda;
    double ceiling;
    int arrival;
} Process;

typedef struct {
    Process *process;
    int index;
    int rr_blockedio;
} Process_helper;

// Helper functions
int starter_compare(const void *a, const void *b) {
    Process_helper *ph1 = (Process_helper *)a;
    Process_helper *ph2 = (Process_helper *)b;
    if ((*ph1).process->arrival == (*ph2).process->arrival) {
        return strcmp((*ph1).process->id, (*ph2).process->id);
    }
    return (*ph1).process->arrival - (*ph2).process->arrival;
}

void print_queue(Process_helper *queue, int queue_count) {
    if (queue_count == 0) {
        printf("[Q empty]\n");
    } else {
        printf("[Q ");
        for (int n = 0; n < queue_count; n++) {
            Process *p = (*(queue + n)).process;
            printf(" %s", p->id);
        }
        printf("]\n");
    }
}

// RR functions


/* 
Purpose: Adds a process to the back of the ready queue, effectively re-queuing a process that has been preempted or has completed its I/O burst.

Parameters:
    queue: A pointer to the pointer of the array of Process_helper structures (the ready queue).
    queue_size: A pointer to the integer that holds the current size of the queue.
    current_process: The process to be re-queued.

How it works:
    Increases the size of the queue by 1.
    Reallocates memory to accommodate the new process.
    Adds the current_process to the back of the queue.
*/

void requeue_process(Process_helper **queue, int *queue_size, Process_helper *current_process) {
    (*queue_size)++;
    *queue = realloc(*queue, (*queue_size) * sizeof(Process_helper));
    if (*queue == NULL) {
        perror("realloc() failed");
        abort();
    }
    (*queue)[(*queue_size) - 1] = *current_process;
}

/*
Purpose: Dequeues the first process from the ready queue and prepares it to start using the CPU.

Parameters:
    queue: A pointer to the pointer of the array of Process_helper structures (the ready queue).
    queue_size: A pointer to the integer that holds the current size of the queue.
    current_process: A pointer to the pointer of the current process being executed.
    queue_to_cpu: A pointer to the integer tracking the time remaining for context switching.
    tcs: The time for a context switch.
    time: The current simulation time.

How it works:
    If queue_to_cpu reaches zero (indicating that context switching is complete), it dequeues the first process in the queue and assigns it to current_process.
    The size of the queue is decreased, and memory is reallocated to reflect this change.
    Prints the start of the process using the CPU.
*/

void dequeue_process(Process_helper **queue, int *queue_size, Process_helper **current_process, int *queue_to_cpu, int tcs, int time) {
    if (*queue_size > 0) {
        (*queue_to_cpu)--;
        if (*queue_to_cpu == 0) {
            *current_process = malloc(sizeof(Process_helper));
            **current_process = **queue;
            // Shift all elements down one index
            for (int i = 0; i < (*queue_size) - 1; i++) {
                (*queue)[i] = (*queue)[i + 1];
            }
            (*queue_size)--;
            *queue = realloc(*queue, (*queue_size) * sizeof(Process_helper));
            if (*queue == NULL && *queue_size > 0) {
                perror("realloc() failed");
                abort();
            }
            printf("time %dms: Process %s started using the CPU ", time, (*current_process)->process->id);
            print_queue(*queue, *queue_size);
            *queue_to_cpu = tcs + 1;
        }
    }
}

/*
Purpose: Handles the completion of I/O bursts for processes and re-queues them to the ready queue.

Parameters:
    previous_process: A pointer to the pointer of the array of Process_helper structures currently waiting for I/O completion.
    previous_size: A pointer to the integer holding the number of processes waiting for I/O completion.
    queue: A pointer to the pointer of the array of Process_helper structures (the ready queue).
    queue_size: A pointer to the integer holding the current size of the queue.
    time: The current simulation time.

How it works:
    Iterates over all processes waiting for I/O completion.
    If the current time matches the time when a process should complete its I/O burst, the process is re-queued and removed from the previous_process list.
*/

void complete_io(Process_helper **previous_process, int *previous_size, Process_helper **queue, int *queue_size, int time) {
    for (int n = 0; n < *previous_size; n++) {
        if (time == (*previous_process)[n].rr_blockedio) {
            requeue_process(queue, queue_size, &((*previous_process)[n]));
            printf("time %dms: Process %s completed I/O; added to ready queue ", time, (*previous_process)[n].process->id);
            print_queue(*queue, *queue_size);
            for (int j = n; j < *previous_size - 1; j++) {
                (*previous_process)[j] = (*previous_process)[j + 1];
            }
            (*previous_size)--;
            *previous_process = realloc(*previous_process, (*previous_size) * sizeof(Process_helper));
            if (*previous_process == NULL && *previous_size > 0) {
                perror("realloc() failed");
                abort();
            }
            n--;  // Stay at the same index after removal
        }
    }
}

/*
Purpose: Checks if any new processes have arrived at the current time and adds them to the ready queue.

Parameters:
    unvisited: A pointer to the pointer of the array of Process_helper structures that have not yet started execution.
    visited_count: A pointer to the integer tracking the number of processes that have been visited.
    n_process: The total number of processes.
    queue: A pointer to the pointer of the array of Process_helper structures (the ready queue).
    queue_size: A pointer to the integer holding the current size of the queue.
    time: The current simulation time.

How it works:
    If a process's arrival time matches the current time, it is added to the ready queue.
*/
void check_process_arrivals(Process_helper **unvisited, int *visited_count, int n_process, Process_helper **queue, int *queue_size, int time) {
    if (*visited_count < n_process) {
        Process_helper *P = (*unvisited + *visited_count);
        if (time == P->process->arrival) {
            (*visited_count)++;
            P->rr_blockedio = 0;
            P->index = 0;
            requeue_process(queue, queue_size, P);
            printf("time %dms: Process %s arrived; added to ready queue ", time, P->process->id);
            print_queue(*queue, *queue_size);
        }
    }
}

void start_process(Process_helper *current_process, int tslice, int *process_end_cpu_at, int *time) {
    int **cpu_io_bursts = current_process->process->cpu_io_bursts;
    int current_burst_time = *(*(cpu_io_bursts + current_process->index) + 0);
    int run_time = (current_burst_time > tslice) ? tslice : current_burst_time;
    printf("time %dms: Process %s started using the CPU for %dms burst\n", *time, current_process->process->id, run_time);
    *process_end_cpu_at = *time + run_time;
}

// Main simulation function

void RoundRobin(Process *givenProcesses, int n_process, int tcs, int tslice, FILE *output) {
    // Sort the processes by arrival time
    Process_helper *unvisited = calloc(n_process, sizeof(Process_helper));
    for (int n = 0; n < n_process; n++) {
        Process_helper *p = (unvisited + n);
        p->process = (givenProcesses + n);
        p->index = 0;
        p->rr_blockedio = 0;
    }
    // Sort the processes by arrival time
    qsort(unvisited, n_process, sizeof(Process_helper), starter_compare);

    int visited_count = 0;
    int time = 0;
    int queue_to_cpu = tcs / 2;
    int process_end_cpu_at = 0;
    int queue_size = 0;

    Process_helper *queue = calloc(0, sizeof(Process_helper));
    Process_helper *current_process = NULL;
    Process_helper *previous_process = calloc(0, sizeof(Process_helper));
    int previous_size = 0;

    printf("time 0ms: Simulator started for RR [Q empty]\n");
    // Main simulation loop
    while (visited_count < n_process || queue_size > 0 || current_process != NULL || previous_size > 0) { // While there are still processes to visit or processes in the queue or processes using the CPU
        if (current_process != NULL && time == process_end_cpu_at) {
            current_process->index++;

            if (current_process->index < current_process->process->numBursts) {
                int remaining_burst_time = *(*(current_process->process->cpu_io_bursts + current_process->index - 1) + 0) - tslice;
                if (remaining_burst_time > 0) {
                    *(*(current_process->process->cpu_io_bursts + current_process->index - 1) + 0) = remaining_burst_time;
                    requeue_process(&queue, &queue_size, current_process);
                    printf("time %dms: Time slice expired; process %s requeued with %dms remaining ", time, current_process->process->id, remaining_burst_time);
                    print_queue(queue, queue_size);
                } else {
                    int **cpu_io_bursts = current_process->process->cpu_io_bursts;
                    if (*(*(cpu_io_bursts + current_process->index - 1) + 1) > 0) {
                        current_process->rr_blockedio = time + *(*(cpu_io_bursts + current_process->index - 1) + 1) + tcs / 2;
                        printf("time %dms: Process %s switching out of CPU; blocking on I/O until time %dms ", time, current_process->process->id, current_process->rr_blockedio);
                        print_queue(queue, queue_size);
                        previous_size++;
                        previous_process = realloc(previous_process, previous_size * sizeof(Process_helper));
                        if (previous_process == NULL) {
                            perror("previous_process realloc() failed");
                            abort();
                        }
                        *(previous_process + previous_size - 1) = *current_process;
                    }
                }
            } else {
                printf("time %dms: Process %s terminated ", time, current_process->process->id);
                print_queue(queue, queue_size);
            }

            free(current_process);
            current_process = NULL;
        }

        dequeue_process(&queue, &queue_size, &current_process, &queue_to_cpu, tcs, time);

        complete_io(&previous_process, &previous_size, &queue, &queue_size, time);

        check_process_arrivals(&unvisited, &visited_count, n_process, &queue, &queue_size, time);

        time++;
    }

    printf("time %dms: Simulator ended for RR ", time);
    print_queue(queue, queue_size);

    free(queue);
    free(previous_process);
    free(unvisited);
}
