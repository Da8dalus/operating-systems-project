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
    int time_queuestart;
    int time_tcs;
    int remaining_time;  // Remaining time for current burst
    int time_slice_left;  // Remaining time in the current time slice
} Process_helper;

int starter_compare_rr(const void *a, const void *b) {
    Process_helper *ph1 = (Process_helper *)a;
    Process_helper *ph2 = (Process_helper *)b;
    if ((*ph1).process->arrival == (*ph2).process->arrival) {
        return strcmp((*ph1).process->id, (*ph2).process->id);
    }
    return (*ph1).process->arrival - (*ph2).process->arrival;
}

void print_queue_rr(Process_helper *queue, int queue_count) {
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

void RR(Process *givenProcesses, int n_process, int tcs, int tslice, FILE *output, int n_cpuBound, int n_ioBound) {
    Process_helper *unvisited = calloc(n_process, sizeof(Process_helper));
    for (int n = 0; n < n_process; n++) {
        Process_helper *p = (unvisited + n);
        p->process = (givenProcesses + n);
        p->index = 0;
        p->rr_blockedio = 0;
        p->time_queuestart = 0;
        p->time_tcs = 0;
        p->remaining_time = 0;  // Initially, no remaining time
        p->time_slice_left = tslice;  // Initialize time slice to the maximum time slice value
    }

    // Sort unvisited processes by arrival time
    qsort(unvisited, n_process, sizeof(Process_helper), starter_compare_rr);

    int visited_count = 0;
    int time = 0;
    int process_end_cpu_at = 0;
    int queue_size = 0;
    bool process_preempted = false;

    Process_helper *queue = calloc(0, sizeof(Process_helper));  // Start with an empty queue
    Process_helper *current_process = NULL; // To track the current process using the CPU
    Process_helper *previous_process = calloc(0, sizeof(Process_helper));
    int previous_size = 0;

    printf("\n\ntime 0ms: Simulator started for RR [Q empty]\n");

    while (visited_count < n_process || queue_size > 0 || current_process != NULL || previous_size > 0) {
        // (a) CPU burst completion or time slice expiration
        if (current_process != NULL) {
            if (time == process_end_cpu_at) {
                // If the process has remaining time after the time slice expires, requeue it
                if (current_process->remaining_time > 0) {
                    process_preempted = true;
                } else {
                    // Regular completion logic
                    current_process->index++;

                    if (current_process->index < current_process->process->numBursts) {
                        // Blocking on I/O
                        int **cpu_io_bursts = current_process->process->cpu_io_bursts;
                        if (*(*(cpu_io_bursts + current_process->index - 1) + 1) > 0) {
                            current_process->rr_blockedio = time + *(*(cpu_io_bursts + current_process->index - 1) + 1) + tcs / 2;
                            printf("time %dms: Process %s switching out of CPU; blocking on I/O until time %dms ",
                                   time, current_process->process->id, current_process->rr_blockedio);
                            print_queue_rr(queue, queue_size);

                            // Add current_process to previous_process array
                            previous_size++;
                            previous_process = realloc(previous_process, previous_size * sizeof(Process_helper));
                            if (previous_process == NULL) {
                                perror("previous_process realloc() failed");
                                abort();
                            }
                            *(previous_process + previous_size - 1) = *current_process;
                        }
                    } else {
                        // Termination message
                        printf("time %dms: Process %s terminated ", time, current_process->process->id);
                        print_queue_rr(queue, queue_size);
                    }
                    free(current_process);
                    current_process = NULL;
                }

                if (process_preempted) {
                    // Re-queue the process
                    queue_size++;
                    current_process->time_tcs = tcs / 2;
                    queue = realloc(queue, queue_size * sizeof(Process_helper));
                    *(queue + queue_size - 1) = *current_process;
                    printf("time %dms: Time slice expired; process %s requeued with %dms remaining ",
                           time, current_process->process->id, current_process->remaining_time);
                    print_queue_rr(queue, queue_size);
                    process_preempted = false;
                    free(current_process);
                    current_process = NULL;
                }
            }
        }

        // (b) Process starts using the CPU
        if (current_process == NULL && queue_size > 0) {
            queue->time_tcs--;

            if (queue->time_tcs == 0) {
                current_process = malloc(sizeof(Process_helper));
                *current_process = *queue;

                // Shift all elements down one index
                for (int i = 0; i < queue_size - 1; i++) {
                    *(queue + i) = *(queue + i + 1);
                }

                queue_size--;
                queue = realloc(queue, queue_size * sizeof(Process_helper));

                Process *p = current_process->process;
                int **cpu_io_bursts = p->cpu_io_bursts;

                // Calculate remaining time for the current burst and set up time slice
                if (current_process->remaining_time == 0) {
                    current_process->remaining_time = *(*(cpu_io_bursts + current_process->index) + 0);
                    current_process->time_slice_left = tslice;  // Reset the time slice
                }
                int run_time = (current_process->remaining_time > current_process->time_slice_left) ? current_process->time_slice_left : current_process->remaining_time;
                process_end_cpu_at = time + run_time;
                current_process->remaining_time -= run_time;
                current_process->time_slice_left -= run_time;

                printf("time %dms: Process %s started using the CPU for %dms burst ",
                       time, p->id, run_time);
                print_queue_rr(queue, queue_size);
            }
        }

        // (c) I/O burst completions so add back to queue
        for (int n = 0; n < previous_size; n++) {
            Process_helper *previous = previous_process + n;
            if (time == previous->rr_blockedio) {
                queue_size++;
                previous->time_tcs = tcs / 2;
                queue = realloc(queue, queue_size * sizeof(Process_helper));
                *(queue + queue_size - 1) = *previous;

                printf("time %dms: Process %s completed I/O; added to ready queue ", time, previous->process->id);
                print_queue_rr(queue, queue_size);

                // Remove the process from previous_process array
                for (int j = n; j < previous_size - 1; j++) {
                    *(previous_process + j) = *(previous_process + j + 1);
                }
                previous_size--;

                previous_process = realloc(previous_process, previous_size * sizeof(Process_helper));
                if (previous_process == NULL && previous_size > 0) {
                    perror("realloc() failed");
                    abort();
                }
                n--;  // Stay at the same index after removal
            }
        }

        // (d) New process arrivals
        if (visited_count < n_process) {
            Process_helper *P = (unvisited + visited_count);
            Process *process = P->process;
            if (time == process->arrival) {
                visited_count++;
                P->rr_blockedio = 0;
                P->index = 0;
                P->time_queuestart = time;
                P->time_tcs = tcs / 2;

                queue_size++;
                queue = realloc(queue, queue_size * sizeof(Process_helper));
                *(queue + queue_size - 1) = *P;

                printf("time %dms: Process %s arrived; added to ready queue ", time, process->id);
                print_queue_rr(queue, queue_size);
            }
        }

        if (visited_count < n_process || queue_size > 0 || current_process != NULL || previous_size > 0) {
            time++;
        } else {
            printf("Exiting loop. No more processes to handle.\n");
        }
    }

    printf("time %dms: Simulator ended for RR ", time + tcs / 2);
    print_queue_rr(queue, queue_size);

    // Free allocated memory
    free(queue);
    free(previous_process);
    free(unvisited);
}
