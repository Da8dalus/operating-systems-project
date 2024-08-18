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
//     int rr_blockedio;
//     int time_queuestart;
//     int time_tcs;
//     int remaining_time;  // Remaining time for current burst
//     int time_slice_left;  // Remaining time in the current time slice
// } Process_helper;

// int starter_compare_rr(const void *a, const void *b) {
//     Process_helper *ph1 = (Process_helper *)a;
//     Process_helper *ph2 = (Process_helper *)b;
//     if ((*ph1).process->arrival == (*ph2).process->arrival) {
//         return strcmp((*ph1).process->id, (*ph2).process->id);
//     }
//     return (*ph1).process->arrival - (*ph2).process->arrival;
// }

// void print_queue_rr(Process_helper *queue, int queue_count) {
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

// void RR(Process *givenProcesses, int n_process, int tcs, int tslice, FILE *output, int n_cpuBound, int n_ioBound) {
//     Process_helper *unvisited = calloc(n_process, sizeof(Process_helper));
//     for (int n = 0; n < n_process; n++) {
//         Process_helper *p = (unvisited + n);
//         p->process = (givenProcesses + n);
//         p->index = 0;
//         p->rr_blockedio = 0;
//         p->time_queuestart = 0;
//         p->time_tcs = 0;
//         p->remaining_time = 0;  // Initially, no remaining time
//         p->time_slice_left = tslice;  // Initialize time slice to the maximum time slice value
//     }

//     // Sort unvisited processes by arrival time
//     qsort(unvisited, n_process, sizeof(Process_helper), starter_compare_rr);

//     int visited_count = 0;
//     int time = 0;
//     int process_end_cpu_at = 0;
//     int queue_size = 0;
//     bool process_preempted = false;

//      //stat counters
//     // double cpu_activetime = 0;
    
//     // double cpuBound_waittime = 0;
//     // double cpu_waittime = 0;
//     // double io_waittime = 0;
//     // double ioBound_waittime = 0;

//     // double cpuTurnaround = 0;
//     // double ioTurnaround = 0;
//     // double cpuTurn_Count = 0;
//     // double ioTurn_Count = 0;

//     // int cpuContext = 0;
//     // int ioContext = 0;

//     Process_helper *queue = calloc(0, sizeof(Process_helper));  // Start with an empty queue
//     Process_helper *current_process = NULL; // To track the current process using the CPU
//     Process_helper *previous_process = calloc(0, sizeof(Process_helper));
//     int previous_size = 0;

//     printf("\n\ntime 0ms: Simulator started for RR [Q empty]\n");

//     while (visited_count < n_process || queue_size > 0 || current_process != NULL || previous_size > 0) {
//         // (a) CPU burst completion or time slice expiration
//         if (current_process != NULL) {
//             if (time == process_end_cpu_at) {
//                 // If the process has remaining time after the time slice expires, requeue it
//                 if (current_process->remaining_time > 0) {
//                     process_preempted = true;
//                 } else {
//                     // Regular completion logic
//                     current_process->index++;

//                     if (current_process->index < current_process->process->numBursts) {
//                         // Blocking on I/O
//                         int **cpu_io_bursts = current_process->process->cpu_io_bursts;
//                         if (*(*(cpu_io_bursts + current_process->index - 1) + 1) > 0) {
//                             current_process->rr_blockedio = time + *(*(cpu_io_bursts + current_process->index - 1) + 1) + tcs / 2;
//                             printf("time %dms: Process %s switching out of CPU; blocking on I/O until time %dms ",
//                                    time, current_process->process->id, current_process->rr_blockedio);
//                             print_queue_rr(queue, queue_size);

//                             // Add current_process to previous_process array
//                             previous_size++;
//                             previous_process = realloc(previous_process, previous_size * sizeof(Process_helper));
//                             if (previous_process == NULL) {
//                                 perror("previous_process realloc() failed");
//                                 abort();
//                             }
//                             *(previous_process + previous_size - 1) = *current_process;
//                         }
//                     } else {
//                         // Termination message
//                         printf("time %dms: Process %s terminated ", time, current_process->process->id);
//                         print_queue_rr(queue, queue_size);
//                     }
//                     free(current_process);
//                     current_process = NULL;
//                 }

//                 if (process_preempted) {
//                     // Re-queue the process
//                     queue_size++;
//                     current_process->time_tcs = tcs / 2;
//                     queue = realloc(queue, queue_size * sizeof(Process_helper));
//                     *(queue + queue_size - 1) = *current_process;
//                     printf("time %dms: Time slice expired; process %s requeued with %dms remaining ",
//                            time, current_process->process->id, current_process->remaining_time);
//                     print_queue_rr(queue, queue_size);
//                     process_preempted = false;
                    
//                     free(current_process);
//                     current_process = NULL;
//                 }
//             }
//         }

//         // (b) Process starts using the CPU
//         if (current_process == NULL && queue_size > 0) {
//             queue->time_tcs--;

//             if (queue->time_tcs == 0) {
//                 current_process = malloc(sizeof(Process_helper));
//                 *current_process = *queue;

//                 // Shift all elements down one index
//                 for (int i = 0; i < queue_size - 1; i++) {
//                     *(queue + i) = *(queue + i + 1);
//                 }

//                 queue_size--;
//                 queue = realloc(queue, queue_size * sizeof(Process_helper));

//                 Process *p = current_process->process;
//                 int **cpu_io_bursts = p->cpu_io_bursts;

//                 // Calculate remaining time for the current burst and set up time slice
//                 if (current_process->remaining_time == 0) {
//                     current_process->remaining_time = *(*(cpu_io_bursts + current_process->index) + 0);
//                     current_process->time_slice_left = tslice;  // Reset the time slice
//                 }
//                 int run_time = (current_process->remaining_time > current_process->time_slice_left) ? current_process->time_slice_left : current_process->remaining_time;
//                 process_end_cpu_at = time + run_time;
//                 current_process->remaining_time -= run_time;
//                 current_process->time_slice_left -= run_time;

//                 printf("time %dms: Process %s started using the CPU for %dms burst ",
//                        time, p->id, run_time);
//                 print_queue_rr(queue, queue_size);
//             }
//         }

//         // (c) I/O burst completions so add back to queue
//         for (int n = 0; n < previous_size; n++) {
//             Process_helper *previous = previous_process + n;
//             if (time == previous->rr_blockedio) {
//                 queue_size++;
//                 previous->time_tcs = tcs / 2;
//                 queue = realloc(queue, queue_size * sizeof(Process_helper));
//                 *(queue + queue_size - 1) = *previous;

//                 printf("time %dms: Process %s completed I/O; added to ready queue ", time, previous->process->id);
//                 print_queue_rr(queue, queue_size);

//                 // Remove the process from previous_process array
//                 for (int j = n; j < previous_size - 1; j++) {
//                     *(previous_process + j) = *(previous_process + j + 1);
//                 }
//                 previous_size--;

//                 previous_process = realloc(previous_process, previous_size * sizeof(Process_helper));
//                 if (previous_process == NULL && previous_size > 0) {
//                     perror("realloc() failed");
//                     abort();
//                 }
//                 n--;  // Stay at the same index after removal
//             }
//         }

//         // (d) New process arrivals
//         if (visited_count < n_process) {
//             Process_helper *P = (unvisited + visited_count);
//             Process *process = P->process;
//             if (time == process->arrival) {
//                 visited_count++;
//                 P->rr_blockedio = 0;
//                 P->index = 0;
//                 P->time_queuestart = time;
//                 P->time_tcs = tcs / 2;

//                 queue_size++;
//                 queue = realloc(queue, queue_size * sizeof(Process_helper));
//                 *(queue + queue_size - 1) = *P;

//                 printf("time %dms: Process %s arrived; added to ready queue ", time, process->id);
//                 print_queue_rr(queue, queue_size);
//             }
//         }

//         if (visited_count < n_process || queue_size > 0 || current_process != NULL || previous_size > 0) {
//             time++;
//         } else {
//             printf("Exiting loop. No more processes to handle.\n");
//         }
//     }

//     printf("time %dms: Simulator ended for RR ", time + tcs / 2);
//     print_queue_rr(queue, queue_size);

//     // Free allocated memory
//     free(queue);
//     free(previous_process);
//     free(unvisited);
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
    int fcfs_blockedio;
    int time_queuestart;
    int time_tcs;
    int time_slice;
    int remaining_time;
} Process_helper;

int starter_compare(const void *a, const void *b) {
    Process_helper *ph1 = (Process_helper *)a;
    Process_helper *ph2 = (Process_helper *)b;
    if ((*ph1).process->arrival == (*ph2).process->arrival) {
        return strcmp((*ph1).process->id, (*ph2).process->id);
    }
    return (*ph1).process->arrival - (*ph2).process->arrival;
}


int backtoqueue_comparer(const void *a, const void *b) {
    Process_helper *ph1 = (Process_helper *)a;
    Process_helper *ph2 = (Process_helper *)b;
    if((*ph1).fcfs_blockedio == (*ph2).fcfs_blockedio){
        return strcmp((*ph1).process->id, (*ph2).process->id);
    }
    return (*ph1).fcfs_blockedio - (*ph2).fcfs_blockedio;
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

void RR(Process *givenProcesses, int n_process, int tcs, int timeslice, FILE *output, int n_cpuBound, int n_ioBound ){
    Process_helper *unvisited = calloc(n_process, sizeof(Process_helper));
    for (int n = 0; n < n_process; n++) {
        Process_helper *p = (unvisited + n);
        p->process = (givenProcesses + n);
        p->index = 0;
        p->fcfs_blockedio = 0;
        p->time_queuestart = 0;
        p->time_tcs = 0;
        p->time_slice = 0;
        p->remaining_time = 0;
    }

    if(n_cpuBound + n_ioBound != n_process){
        perror("Counted too many processes");
        abort();
    }

    // Sort unvisited processes by arrival time
    qsort(unvisited, n_process, sizeof(Process_helper), starter_compare);

    int visited_count = 0;
    int time = 0;
    // int queue_to_cpu = tcs / 2;
    int process_end_cpu_at = 0;
    int queue_size = 0;



    //stat counters
    double cpu_activetime = 0;
    
    double cpuBound_waittime = 0;
    double cpu_waittime = 0;
    double io_waittime = 0;
    double ioBound_waittime = 0;

    double cpuTurnaround = 0;
    double ioTurnaround = 0;
    double cpuTurn_Count = 0;
    double ioTurn_Count = 0;

    int cpuContext = 0;
    int ioContext = 0;
    

    

    Process_helper *queue = calloc(0, sizeof(Process_helper));  // Start with an empty queue
    Process_helper *current_process = NULL; // To track the current process using the CPU
    Process_helper *previous_process = calloc(0, sizeof(Process_helper));
    int previous_size = 0;

    printf("time 0ms: Simulator started for FCFS [Q empty]\n");

    while (visited_count < n_process || queue_size > 0 || current_process != NULL || previous_size > 0) {
        // (a) CPU burst completion
        if (current_process != NULL) {
            if(time != process_end_cpu_at && current_process->time_slice == 0){
                current_process->remaining_time -= timeslice;
                printf("time %dms: Time slice expired; Process %s requeued with %dms remaining",time, current_process->process->id, current_process->remaining_time);

                
                //move current to end and start anew
                Process_helper helper;
                helper = *current_process;
                // Shift all elements down one index
                for (int i = 0; i < queue_size - 1; i++) {
                    *(queue + i) = *(queue + i + 1);
                }
                queue_size++;

                // Reallocate memory for the queue
                queue = realloc(queue, queue_size * sizeof(Process_helper));
                if (queue == NULL) {
                    perror("realloc() failed");
                    abort();
                }
                if(queue_size > 0){
                    queue->time_tcs += tcs/2 + 1;
                }
                *(queue + queue_size - 1) = helper;
                current_process = NULL;
                    
                


            }else if(time != process_end_cpu_at && current_process->time_slice > 0){
                current_process->time_slice--;
            }else if (time == process_end_cpu_at) {
                current_process->index++;

                // Completion message
                // if (time < 10000) {
                    printf("time %dms: Process %s completed a CPU burst; %d bursts to go ",
                        time, current_process->process->id, current_process->process->numBursts - current_process->index);
                    print_queue(queue, queue_size);
                // }

                // Check if the process has more bursts left
                if (current_process->index < current_process->process->numBursts) {
                    // Blocking on I/O
                    int **cpu_io_bursts = current_process->process->cpu_io_bursts;
                    if (*(*(cpu_io_bursts + current_process->index - 1) + 1) > 0) {  // Correct index for I/O burst

                        current_process->fcfs_blockedio = time + *(*(cpu_io_bursts + current_process->index - 1) + 1) + tcs/2;
                        current_process->remaining_time = current_process->fcfs_blockedio;
                        
                        
                        // if (time < 10000) {
                            printf("time %dms: Process %s switching out of CPU; blocking on I/O until time %dms ",
                                time, current_process->process->id, current_process->fcfs_blockedio);
                            print_queue(queue, queue_size);
                        // }

                        ///need to add some way to use context switch

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
                    print_queue(queue, queue_size);
                }
                
                if(queue_size > 0){
                    queue->time_tcs += tcs/2 + 1;
                }

                free(current_process);
                current_process = NULL;
            }
        }


        // (b) Process starts using the CPU
        if (current_process == NULL && queue_size > 0) {
            queue->time_tcs--;

            if (queue->time_tcs == 0) {
                // Dequeue the first process in the queue
                current_process = malloc(sizeof(Process_helper));
                *current_process = *queue;

                if(current_process->process->cpu_bound){
                    cpuContext++;
                }else{
                    ioContext++;
                }


                // Shift all elements down one index
                for (int i = 0; i < queue_size - 1; i++) {
                    *(queue + i) = *(queue + i + 1);
                }

                // Decrease the queue size
                queue_size--;

                // Reallocate memory for the queue
                queue = realloc(queue, queue_size * sizeof(Process_helper));
                if (queue == NULL && queue_size > 0) {
                    perror("realloc() failed");
                    abort();
                }


                Process *p = current_process->process;

                //waittime calculations
                if(p->cpu_bound){
                    // cpuTurnaround += tcs/2;
                    // cpuContext++;
                    cpu_waittime += time - current_process->time_queuestart;
                    cpuBound_waittime++;
                }else{
                    // ioTurnaround += tcs/2;
                    // ioContext++;
                    
                    io_waittime += time - current_process->time_queuestart;
                    ioBound_waittime++;
                }


                if(current_process->process->cpu_bound){
                    cpuTurn_Count++;
                }else{
                    ioTurn_Count++;
                }
                
                int waittime = time - current_process->time_queuestart;
                current_process->time_queuestart = 0;

                int **cpu_io_bursts = p->cpu_io_bursts;

                // if (time < 10000) {
                    printf("time %dms: Process %s started using the CPU for %dms burst ",
                           time, p->id, *(*(cpu_io_bursts + current_process->index) + 0));
                    print_queue(queue, queue_size);
                // }

                if(p->cpu_bound){
                    cpuTurnaround += *(*(cpu_io_bursts + current_process->index) + 0) + waittime +tcs;
                }else{
                    ioTurnaround += *(*(cpu_io_bursts + current_process->index) + 0)  + waittime + tcs;
                }

                
                process_end_cpu_at = time + *(*(cpu_io_bursts + current_process->index) + 0);
                cpu_activetime += *(*(cpu_io_bursts + current_process->index) + 0);
            }
        }

        // (c) I/O burst completions so add back to queue
        qsort(previous_process,previous_size, sizeof(Process_helper), backtoqueue_comparer);
        for (int n = 0; n < previous_size; n++) {
            Process_helper *previous = previous_process + n;
            if (time == previous->fcfs_blockedio) {

                // Increase the queue size
               queue_size++;
               previous->time_tcs = tcs/2;
               previous->time_queuestart = time + tcs/2;

                
                

                // Reallocate memory for the queue
                queue = realloc(queue, queue_size * sizeof(Process_helper));
                if (queue == NULL) {
                    perror("realloc() failed");
                    abort();
                }


                *(queue + queue_size - 1) = *previous;

                // if (time < 10000) {
                    printf("time %dms: Process %s completed I/O; added to ready queue ",
                           time, previous->process->id);
                    print_queue(queue, queue_size);
                // }

                

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
                P->fcfs_blockedio = 0;
                P->index = 0;
                P->time_queuestart = time + tcs/2;
                P->time_tcs = tcs/2;
                P->time_slice = timeslice;
                P->remaining_time = 0;


                // printf("%s time_tcs = %d\n", process->id, P->time_tcs);
                // if(process->cpu_bound){
                //     cpuTurnaround += tcs/2;
                // }else{
                //     ioTurnaround += tcs/2;
                // }

                // Increase the queue size
                queue_size++;

                // Reallocate memory for the queue
                queue = realloc(queue, queue_size * sizeof(Process_helper));
                if (queue == NULL) {
                    perror("realloc() failed");
                    abort();
                }

                // Add the new process to the end of the queue
                *(queue + queue_size - 1) = *P;
                // if (time < 10000) {
                    printf("time %dms: Process %s arrived; added to ready queue ", time, process->id);
                    print_queue(queue, queue_size);
                // }
            }
        }

        if(visited_count < n_process || queue_size > 0 || current_process != NULL || previous_size > 0){
            time++;
        }
        
        
    }

    printf("time %dms: Simulator ended for FCFS ", time + tcs/2);
    print_queue(queue, queue_size);


    // Free allocated memory
    free(queue);
    free(previous_process);
    free(unvisited);



    double cpu_utilization = cpu_activetime / time *100;
    printf("active time %f\n", cpu_activetime / time);
    double cpu_boundavgTurn = cpuTurnaround/cpuTurn_Count;
    
    double io_boundavgTurn = ioTurnaround / ioTurn_Count;
    double overallavgTurn = (cpuTurnaround + ioTurnaround)/(cpuTurn_Count + ioTurn_Count);

    double cpu_avgWait = cpu_waittime/ cpuBound_waittime;
    printf("cpuavgWait: %f\n",cpu_waittime);
    double io_avgWait = io_waittime / ioBound_waittime;
    double overallavgWait = (cpu_waittime + io_waittime)/(cpuBound_waittime + ioBound_waittime);

    if(cpuTurn_Count == 0){
        cpu_boundavgTurn = 0;
    }
    if(ioTurn_Count == 0){
        io_boundavgTurn = 0;
    }
    if(ioTurn_Count == 0 && cpuTurn_Count == 0){
        overallavgTurn = 0;
    }

    if(cpuBound_waittime == 0){
        cpu_avgWait = 0;
    }
    if(ioBound_waittime == 0){
        io_avgWait = 0;
    }
    if(cpuBound_waittime + ioBound_waittime == 0){
        overallavgWait = 0;
    }

    if(time == 0){
        cpu_utilization = 0;
    }


//     CPU-bound average wait time: 66.280 ms
// -- I/O-bound average wait time: 677.200 ms
// -- overall average wait time: 337.800 ms
// -- CPU-bound average turnaround time: 1670.920 ms
// -- I/O-bound average turnaround time: 928.500 ms
// -- overall average turnaround time: 1340.956 ms
// -- CPU-bound number of context switches: 25
// -- I/O-bound number of context switches: 20
// -- overall number of context switches: 45
// -- CPU-bound number of preemptions: 0
// -- I/O-bound number of preemptions: 0
// -- overall number of preemptions: 0

    fprintf(output,"\nAlgorithm FCFS\n");

    fprintf(output, "-- CPU utilization: %.3f%%\n", cpu_utilization);
    fprintf(output, "-- CPU-bound average wait time: %.3f ms\n", cpu_avgWait);
    fprintf(output, "-- I/O-bound average wait time: %.3f ms\n", io_avgWait);
    fprintf(output, "-- overall average wait time: %.3f ms\n", overallavgWait);
    fprintf(output, "-- CPU-bound average turnaround time: %.3f ms\n", cpu_boundavgTurn);
    fprintf(output, "-- I/O-bound average turnaround time: %.3f ms\n", io_boundavgTurn);
    fprintf(output, "-- overall average turnaround time: %.3f ms\n", overallavgTurn);
    fprintf(output, "-- CPU-bound number of context switches: %d\n", cpuContext);
    fprintf(output, "-- I/O-bound number of context switches: %d\n", ioContext);
    fprintf(output, "-- overall number of context switches: %d\n", ioContext + cpuContext);
    fprintf(output, "-- CPU-bound number of preemptions: 0\n");
    fprintf(output, "-- I/O-bound number of preemptions: 0\n");
    fprintf(output, "-- overall number of preemptions: 0\n");

}

