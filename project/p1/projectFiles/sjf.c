// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <stdbool.h>
// #include <math.h>

// typedef struct {
//     char *id;
//     int numBursts;
//     int **cpu_io_bursts;
//     bool cpu_bound;
//     double lambda;
//     double ceiling;
//     int arrival;
// } Process;

// typedef struct {
//     Process *process;
//     int index;
//     int sjf_blockedio;
//     int tau;
// } Process_helper;

// int tau_compare(const void *a, const void *b) {
//     Process_helper *ph1 = (Process_helper *)a;
//     Process_helper *ph2 = (Process_helper *)b;
//     if (ph1->tau == ph2->tau) {
//         return strcmp(ph1->process->id, ph2->process->id);
//     }
//     return ph1->tau - ph2->tau;
// }

// void print_queue(Process_helper *queue, int queue_count) {
//     if (queue_count == 0) {
//         printf("[Q empty]\n");
//     } else {
//         printf("[Q ");
//         for (int n = 0; n < queue_count; n++) {
//             Process *p = (*(queue + n)).process;
//             printf(" %s", p->id);
//         }
//         printf("]\n");
//     }
// }

// void SJF(Process *givenProcesses, int n_process, int tcs, double alpha, FILE *output) {
//     Process_helper *unvisited = calloc(n_process, sizeof(Process_helper));
//     if (unvisited == NULL) {
//         perror("Failed to allocate memory for unvisited processes");
//         abort();
//     }

//     for (int n = 0; n < n_process; n++) {
//         Process_helper *p = (unvisited + n);
//         p->process = (givenProcesses + n);
//         p->index = 0;
//         p->sjf_blockedio = 0;
//         p->tau = ceil(1 / p->process->lambda);
//     }

//     qsort(unvisited, n_process, sizeof(Process_helper), tau_compare);

//     int visited_count = 0;
//     int time = 0;
//     int queue_size = 0;
//     int process_end_cpu_at = 0;

//     Process_helper *queue = NULL;
//     Process_helper *current_process = NULL;

//     printf("time 0ms: Simulator started for SJF [Q empty]\n");

//     while (visited_count < n_process || queue_size > 0 || current_process != NULL) {
//         if (current_process != NULL) {
//             if (time == process_end_cpu_at) {
//                 current_process->index++;

//                 if (current_process->index < current_process->process->numBursts) {
//                     // Placeholder for I/O handling
//                 } else {
//                     printf("time %dms: Process %s terminated\n", time, current_process->process->id);
//                     free(current_process);
//                     current_process = NULL;
//                 }
//             }
//         }

//         if (visited_count < n_process) {
//             Process_helper *P = (unvisited + visited_count);
//             Process *process = P->process;
//             if (time == process->arrival) {
//                 visited_count++;
//                 P->sjf_blockedio = 0;
//                 P->index = 0;

//                 queue_size++;
//                 Process_helper *temp_queue = realloc(queue, queue_size * sizeof(Process_helper));
//                 if (temp_queue == NULL) {
//                     perror("realloc() failed");
//                     abort();
//                 }
//                 queue = temp_queue;

//                 *(queue + queue_size - 1) = *P;
//                 printf("time %dms: Process %s arrived; added to ready queue ", time, process->id);
//                 print_queue(queue, queue_size);
//             }
//         }

//         if (current_process == NULL && queue_size > 0) {
//             printf("Selecting the next process to run...\n");

//             qsort(queue, queue_size, sizeof(Process_helper), tau_compare);

//             current_process = malloc(sizeof(Process_helper));
//             if (current_process == NULL) {
//                 perror("malloc() failed");
//                 abort();
//             }

//             printf("Queue front process ID: %s\n", queue->process->id);

//             if (queue->process->cpu_io_bursts == NULL) {
//                 fprintf(stderr, "Error: cpu_io_bursts is NULL for process %s\n", queue->process->id);
//                 abort();
//             }

//             for (int i = 0; i < queue->process->numBursts; i++) {
//                 if (queue->process->cpu_io_bursts[i] == NULL) {
//                     fprintf(stderr, "Error: cpu_io_bursts[%d] is NULL for process %s\n", i, queue->process->id);
//                     abort();
//                 }
//             }

//             *current_process = *queue;

//             printf("Selected process: %s, index: %d\n", 
//                    current_process->process->id, 
//                    current_process->index);

//             process_end_cpu_at = time + *(*(current_process->process->cpu_io_bursts + current_process->index) + 0);

//             printf("time %dms: Process %s started using the CPU for %dms burst\n",
//                    time, current_process->process->id, *(*(current_process->process->cpu_io_bursts + current_process->index) + 0));

//             // Shift queue elements down
//             for (int i = 0; i < queue_size - 1; i++) {
//                 *(queue + i) = *(queue + i + 1);
//             }

//             queue_size--;
//             Process_helper *temp_queue = realloc(queue, queue_size * sizeof(Process_helper));
//             if (temp_queue == NULL && queue_size > 0) {
//                 perror("realloc() failed");
//                 abort();
//             }
//             queue = temp_queue;

//             // Update queue display after removing the selected process
//             print_queue(queue, queue_size); 
//         }

//         time++;
//     }

//     printf("time %dms: Simulator ended for SJF [Q empty]\n", time);

//     free(queue);
//     free(unvisited);
// }

// int main() {
//     // mock processes
//     Process p1, p2, p3;

//     p1.id = "A1";
//     p1.numBursts = 2;
//     p1.arrival = 0;
//     int p1_bursts[2][2] = {{10, 5}, {7, 0}};
//     p1.cpu_io_bursts = (int **)calloc(p1.numBursts, sizeof(int *));
//     for (int i = 0; i < p1.numBursts; i++) {
//         p1.cpu_io_bursts[i] = (int *)calloc(2, sizeof(int));
//         p1.cpu_io_bursts[i][0] = p1_bursts[i][0];
//         p1.cpu_io_bursts[i][1] = p1_bursts[i][1];
//     }

//     p2.id = "B1";
//     p2.numBursts = 2;
//     p2.arrival = 3;
//     int p2_bursts[2][2] = {{8, 4}, {6, 0}};
//     p2.cpu_io_bursts = (int **)calloc(p2.numBursts, sizeof(int *));
//     for (int i = 0; i < p2.numBursts; i++) {
//         p2.cpu_io_bursts[i] = (int *)calloc(2, sizeof(int));
//         p2.cpu_io_bursts[i][0] = p2_bursts[i][0];
//         p2.cpu_io_bursts[i][1] = p2_bursts[i][1];
//     }

//     p3.id = "C1";
//     p3.numBursts = 2;
//     p3.arrival = 5;
//     int p3_bursts[2][2] = {{6, 2}, {5, 0}};
//     p3.cpu_io_bursts = (int **)calloc(p3.numBursts, sizeof(int *));
//     for (int i = 0; i < p3.numBursts; i++) {
//         p3.cpu_io_bursts[i] = (int *)calloc(2, sizeof(int));
//         p3.cpu_io_bursts[i][0] = p3_bursts[i][0];
//         p3.cpu_io_bursts[i][1] = p3_bursts[i][1];
//     }

//     Process processes[] = {p1, p2, p3};
//     int n_processes = 3;
//     int tcs = 2;
//     double alpha = 0.5;

//     FILE *output = stdout;

//     SJF(processes, n_processes, tcs, alpha, output);

//     // free memory
//     for (int i = 0; i < n_processes; i++) {
//         for (int j = 0; j < processes[i].numBursts; j++) {
//             free(processes[i].cpu_io_bursts[j]);
//         }
//         free(processes[i].cpu_io_bursts);
//     }

//     return 0;
// }

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
    int sjf_blockedio;
    int tau;
    int time_tcs;
} Process_helper;

int recalculate_tau(int tau_old, int actual_burst, double alpha) {
    return (int)ceil(alpha * actual_burst + (1 - alpha) * tau_old);
}

int tau_compare(const void *a, const void *b) {
    Process_helper *ph1 = (Process_helper *)a;
    Process_helper *ph2 = (Process_helper *)b;
    if (ph1->tau == ph2->tau) {
        return strcmp(ph1->process->id, ph2->process->id);
    }
    return ph1->tau - ph2->tau;
}

void printQueue(Process_helper *queue, int queue_count) {
    if (queue_count == 0) {
        printf("[Q empty]");
    } else {
        printf("[Q");
        for (int n = 0; n < queue_count; n++) {
            printf(" %s", queue[n].process->id);
        }
        printf("]");
    }
}


void SJF(Process *givenProcesses, int n_process, int tcs, double alpha, FILE *output) {
    Process_helper *unvisited = calloc(n_process, sizeof(Process_helper));
    if (unvisited == NULL) {
        perror("Failed to allocate memory for unvisited processes");
        abort();
    }

    for (int n = 0; n < n_process; n++) {
        Process_helper *p = (unvisited + n);
        p->process = (givenProcesses + n);
        p->index = 0;
        p->sjf_blockedio = 0;
        p->tau = 1000; // Initial tau is given as 1000ms
    }

    qsort(unvisited, n_process, sizeof(Process_helper), tau_compare);

    int visited_count = 0;
    int time = 0;
    int queue_size = 0;
    int process_end_cpu_at = 0;

    Process_helper *queue = NULL;
    Process_helper *current_process = NULL;

    printf("time 0ms: Simulator started for SJF ");
    printQueue(queue, queue_size);
    printf("\n");

    while (visited_count < n_process || queue_size > 0 || current_process != NULL) {
        if (current_process != NULL) {
            if (time == process_end_cpu_at) {
                int actual_burst = *(*(current_process->process->cpu_io_bursts + current_process->index) + 0);
                printf("time %dms: Process %s (tau %dms) completed a CPU burst; %d bursts to go ", 
                        time, current_process->process->id, current_process->tau, 
                        current_process->process->numBursts - current_process->index - 1);
                printQueue(queue, queue_size);
                printf("\n");
                
                int old_tau = current_process->tau;
                current_process->tau = recalculate_tau(old_tau, actual_burst, alpha);
                printf("time %dms: Recalculated tau for process %s: old tau %dms ==> new tau %dms ", 
                        time, current_process->process->id, old_tau, current_process->tau);
                printQueue(queue, queue_size);
                printf("\n");

                current_process->index++;

                if (current_process->index < current_process->process->numBursts) {
                    int io_burst_time = *(*(current_process->process->cpu_io_bursts + current_process->index - 1) + 1);
                    printf("time %dms: Process %s switching out of CPU; blocking on I/O until time %dms ", 
                            time, current_process->process->id, time + io_burst_time);
                    printQueue(queue, queue_size);
                    printf("\n");

                    current_process->sjf_blockedio = time + io_burst_time;
                    
                    Process_helper *temp_queue = realloc(queue, (queue_size + 1) * sizeof(Process_helper));
                    if (temp_queue == NULL) {
                        perror("realloc() failed");
                        abort();
                    }
                    queue = temp_queue;
                    queue[queue_size] = *current_process;
                    queue_size++;
                    
                    free(current_process);
                    current_process = NULL;
                } else {
                    printf("time %dms: Process %s terminated ", time, current_process->process->id);
                    printQueue(queue, queue_size);
                    printf("\n");
                    free(current_process);
                    current_process = NULL;
                }
            }
        }

        if (visited_count < n_process) {
            Process_helper *P = (unvisited + visited_count);
            Process *process = P->process;
            if (time == process->arrival) {
                visited_count++;
                P->sjf_blockedio = 0;
                P->index = 0;

                queue_size++;
                Process_helper *temp_queue = realloc(queue, queue_size * sizeof(Process_helper));
                if (temp_queue == NULL) {
                    perror("realloc() failed");
                    abort();
                }
                queue = temp_queue;

                *(queue + queue_size - 1) = *P;
                printf("time %dms: Process %s (tau %dms) arrived; added to ready queue ", time, process->id, P->tau);
                printQueue(queue, queue_size);
                printf("\n");
            }
        }

        if (current_process == NULL && queue_size > 0) {

            qsort(queue, queue_size, sizeof(Process_helper), tau_compare);

            current_process = malloc(sizeof(Process_helper));
            if (current_process == NULL) {
                perror("malloc() failed");
                abort();
            }

            *current_process = *queue;

            process_end_cpu_at = time + *(*(current_process->process->cpu_io_bursts + current_process->index) + 0);

            printf("time %dms: Process %s (tau %dms) started using the CPU for %dms burst ", 
                   time, current_process->process->id, current_process->tau, 
                   *(*(current_process->process->cpu_io_bursts + current_process->index) + 0));
            printQueue(queue, queue_size);
            printf("\n");

            for (int i = 0; i < queue_size - 1; i++) {
                *(queue + i) = *(queue + i + 1);
            }

            queue_size--;
            Process_helper *temp_queue = realloc(queue, queue_size * sizeof(Process_helper));
            if (temp_queue == NULL && queue_size > 0) {
                perror("realloc() failed");
                abort();
            }
            queue = temp_queue;

            printQueue(queue, queue_size); 
        }

        time++;

        for (int i = 0; i < queue_size; i++) {
            if (queue[i].sjf_blockedio == time) {
                queue[i].sjf_blockedio = 0;
                printf("time %dms: Process %s (tau %dms) completed I/O; added to ready queue ", 
                        time, queue[i].process->id, queue[i].tau);
                printQueue(queue, queue_size);
                printf("\n");
            }
        }
    }

    printf("time %dms: Simulator ended for SJF [Q empty]\n", time);

    free(queue);
    free(unvisited);
}