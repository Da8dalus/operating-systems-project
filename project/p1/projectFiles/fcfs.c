#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

typedef struct{
	char * id;
	int numBursts;
	int ** cpu_io_bursts;
	bool cpu_bound;
	double lambda;
	double ceiling;
	int arrival;
}Process;

typedef struct {
    Process *process;
    int index;
    int fcfs_blockedio;
} Process_helper;

// Comparator function for sorting by arrival time
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

void FCFS(Process *givenProcesses, int n_process, int tcs, FILE *output) {
    Process_helper *unvisited = calloc(n_process, sizeof(Process_helper));
    for (int n = 0; n < n_process; n++) {
        Process_helper *p = (unvisited + n);
        p->process = (givenProcesses + n);
        p->index = 0;
        p->fcfs_blockedio = 0;
    }

    // Sort unvisited processes by arrival time
    qsort(unvisited, n_process, sizeof(Process_helper), starter_compare);

    int visited_count = 0;
    int time = 0;
    int queue_to_cpu = tcs / 2;
    int process_end_cpu_at = 0;
    int queue_size = 0;

    Process_helper *queue = NULL;  // Start with an empty queue
    Process_helper *current_process = NULL; // To track the current process using the CPU

    printf("time 0ms: Simulator started for FCFS [Q empty]\n");

    while (visited_count < n_process || queue_size > 0 || current_process != NULL) {
        

        // (a) CPU burst completion
        if (current_process != NULL) {
            if (time == process_end_cpu_at) {
                current_process->index++;

                //completion message
                if (time < 10000) {
                    printf("time %dms: Process %s completed a CPU burst; %d bursts to go ", time, current_process->process->id, (current_process->process->numBursts - current_process->index));
                    print_queue(queue, queue_size);
                }
                //blocking on I/O
                int **cpu_io_bursts = current_process->process->cpu_io_bursts;
                if (*(*(cpu_io_bursts + current_process->index) + 1) > 0) {
                    current_process->fcfs_blockedio = time + *(*(cpu_io_bursts + current_process->index) + 1);
                    if (time < 10000) {
                        printf("time %dms: Process %s switching out of CPU; blocking on I/O until time %dms ", time, current_process->process->id, current_process->fcfs_blockedio);
                        print_queue(queue, queue_size);
                    }
                }

                //termination message
                if ((current_process->process->numBursts - current_process->index) == 0) {
                    printf("time %dms: Process %s terminated ", time, current_process->process->id);
                    print_queue(queue, queue_size);
                }

                current_process = NULL;
            }
        }

        // (b) Process starts using the CPU

        if (current_process == NULL && queue_size > 0) {
            queue_to_cpu--;

            if (queue_to_cpu == 0) {
                // Dequeue the first process in the queue
                current_process = queue;

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
                int **cpu_io_bursts = p->cpu_io_bursts;
                if (time < 10000){
                    printf("time %dms: Process %s started using the CPU for %dms burst ", time, p->id, *(*(cpu_io_bursts + current_process->index) + 0));
                    print_queue(queue, queue_size);
                }

                process_end_cpu_at = time + *(*(cpu_io_bursts + current_process->index) + 0);
                
                queue_to_cpu = tcs;
            }
        }

        // (c) I/O burst completions so add back to queue
        for (int n = 0; n < n_process; n++) {
            Process_helper *current_unvisited = unvisited + n;
            if (time == current_unvisited->fcfs_blockedio) {
                Process *p = current_unvisited->process;
                

                // Increase the queue size
                queue_size++;

                // Reallocate memory for the queue
                queue = realloc(queue, queue_size * sizeof(Process_helper));
                if (queue == NULL) {
                    perror("realloc() failed");
                    abort();
                }

                *(queue + queue_size - 1) = *current_unvisited;
                if (time < 10000){
                    printf("time %dms: Process %s completed I/O; add to ready queue ", time, p->id);
                    print_queue(queue, queue_size);
                }
                
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
                if (time < 10000){
                    printf("time %dms: Process %s arrived; added to ready queue ", time, process->id);
                    print_queue(queue, queue_size);
                }
            }
        }

        time++;
        // Add logic to exit the loop when all processes are completed
    }
    time++;

    printf("time %d ms: Simulator ended for FCFS ",time);
    print_queue(queue, queue_size);

    // Free allocated memory
    free(queue);
    free(unvisited);
}
