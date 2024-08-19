#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "processC.h"
#include "operationC.h"
#include "next_expC.h"

// Function prototypes
void fcfs(Process *givenProcesses, int n_process, int tcs, FILE *output, int n_cpuBound, int n_ioBound);
void sjf(Process *processes, int n_process, double t_cs, double alpha, FILE *output);
void srt(Process *processes, int n_process, double t_cs, double alpha, FILE *output);
void rr(Process *processes, int n_process, double t_cs, unsigned int t_slice, FILE *output);

// Helper function for sorting processes by arrival time and ID
int process_compare(const void *a, const void *b) {
    Process *p1 = (Process *)a;
    Process *p2 = (Process *)b;
    if (p1->arrival == p2->arrival) {
        return strcmp(p1->id, p2->id);
    }
    return p1->arrival - p2->arrival;
}

// Helper function to print the process queue
void print_queue(Process **queue, int queue_count) {
    if (queue_count == 0) {
        printf("[Q empty]\n");
    } else {
        printf("[Q");
        for (int i = 0; i < queue_count; i++) {
            printf(" %s", queue[i]->id);
        }
        printf("]\n");
    }
}

// SJF Algorithm Implementation
void sjf(Process *processes, int n_process, double t_cs, double alpha, FILE *output) {
    printf("time 0ms: Simulator started for SJF [Q empty]\n");

    unsigned int time = 0;
    unsigned int cpu_time = 0;
    unsigned int cpu_bound_total_wait_time = 0;
    unsigned int io_bound_total_wait_time = 0;
    unsigned int cpu_bound_cs_count = 0;
    unsigned int io_bound_cs_count = 0;

    // Sort processes by arrival time
    qsort(processes, n_process, sizeof(Process), process_compare);

    Process **ready_queue = NULL;
    int ready_queue_size = 0;
    Operation *op_queue = NULL;
    int op_queue_size = 0;

    for (int i = 0; i < n_process; i++) {
        op_queue_size++;
        op_queue = realloc(op_queue, op_queue_size * sizeof(Operation));
        op_queue[op_queue_size - 1].type = ARRIVE;
        op_queue[op_queue_size - 1].time = processes[i].arrival;
        op_queue[op_queue_size - 1].process = &processes[i];
    }

    bool cpu_busy = false;
    while (op_queue_size > 0 || ready_queue_size > 0 || cpu_busy) {
        if (!cpu_busy && ready_queue_size > 0) {
            cpu_busy = true;
            Process *proc = ready_queue[0];
            ready_queue_size--;
            for (int i = 0; i < ready_queue_size; i++) {
                ready_queue[i] = ready_queue[i + 1];
            }
            ready_queue = realloc(ready_queue, ready_queue_size * sizeof(Process *));
            op_queue_size++;
            op_queue = realloc(op_queue, op_queue_size * sizeof(Operation));
            op_queue[op_queue_size - 1].type = DEQUEUE;
            op_queue[op_queue_size - 1].time = time;
            op_queue[op_queue_size - 1].process = proc;
        }

        Operation op = op_queue[0];
        op_queue_size--;
        for (int i = 0; i < op_queue_size; i++) {
            op_queue[i] = op_queue[i + 1];
        }
        op_queue = realloc(op_queue, op_queue_size * sizeof(Operation));
        time = op.time;

        switch (op.type) {
            case ARRIVE:
            case IO_END:
                ready_queue_size++;
                ready_queue = realloc(ready_queue, ready_queue_size * sizeof(Process *));
                ready_queue[ready_queue_size - 1] = op.process;

                qsort(ready_queue, ready_queue_size, sizeof(Process *), process_compare);

                if (op.type == ARRIVE) {
                    printf("time %ums: Process %s (tau %ums) arrived; added to ready queue ", time, op.process->id, op.process->tau);
                } else {
                    printf("time %ums: Process %s (tau %ums) completed I/O; added to ready queue ", time, op.process->id, op.process->tau);
                }
                print_queue(ready_queue, ready_queue_size);
                op.process->ready_queue_add_time = time;
                break;

            case DEQUEUE:
                cpu_busy = true;
                Process *dq_proc = op.process;
                op_queue_size++;
                op_queue = realloc(op_queue, op_queue_size * sizeof(Operation));
                op_queue[op_queue_size - 1].type = BURST;
                op_queue[op_queue_size - 1].time = time + (t_cs / 2);
                op_queue[op_queue_size - 1].process = dq_proc;

                if (dq_proc->cpu_bound) {
                    cpu_bound_total_wait_time += time - dq_proc->ready_queue_add_time;
                } else {
                    io_bound_total_wait_time += time - dq_proc->ready_queue_add_time;
                }
                break;

            case BURST: {
                unsigned int burst_time = dq_proc->cpu_io_bursts[dq_proc->index][0];
                printf("time %ums: Process %s (tau %ums) started using the CPU for %ums burst ", time, dq_proc->id, dq_proc->tau, burst_time);
                print_queue(ready_queue, ready_queue_size);
                dq_proc->burst_start_time = time;

                op_queue_size++;
                op_queue = realloc(op_queue, op_queue_size * sizeof(Operation));
                op_queue[op_queue_size - 1].type = EXPIRE;
                op_queue[op_queue_size - 1].time = time + burst_time;
                op_queue[op_queue_size - 1].process = dq_proc;
                dq_proc->current_burst_elapsed += burst_time;

                cpu_time += burst_time;

                if (dq_proc->cpu_bound) {
                    cpu_bound_cs_count++;
                } else {
                    io_bound_cs_count++;
                }
                break;
            }

            case EXPIRE: {
                cpu_busy = false;
                unsigned int actual_burst_time = time - dq_proc->burst_start_time;
                unsigned int old_tau = dq_proc->tau;

                if (dq_proc->index >= dq_proc->numBursts - 1) {
                    printf("time %ums: Process %s terminated ", time, dq_proc->id);
                    print_queue(ready_queue, ready_queue_size);
                } else {
                    dq_proc->tau = ceil(alpha * actual_burst_time + (1.0 - alpha) * old_tau);
                    printf("time %ums: Process %s (tau %ums) completed a CPU burst; %lu bursts to go ", time, dq_proc->id, old_tau, dq_proc->numBursts - dq_proc->index - 1);
                    print_queue(ready_queue, ready_queue_size);

                    printf("time %ums: Recalculated tau for process %s: old tau %ums ==> new tau %ums ", time, dq_proc->id, old_tau, dq_proc->tau);
                    print_queue(ready_queue, ready_queue_size);

                    unsigned int unblock_time = time + dq_proc->cpu_io_bursts[dq_proc->index][1] + (t_cs / 2);
                    op_queue_size++;
                    op_queue = realloc(op_queue, op_queue_size * sizeof(Operation));
                    op_queue[op_queue_size - 1].type = IO_END;
                    op_queue[op_queue_size - 1].time = unblock_time;
                    op_queue[op_queue_size - 1].process = dq_proc;
                }

                dq_proc->current_burst_elapsed = 0;
                dq_proc->index++;
                time += t_cs / 2;
                break;
            }

            default:
                break;
        }
    }

    printf("time %ums: Simulator ended for SJF ", time);
    print_queue(ready_queue, ready_queue_size);
    printf("\n");

    // Calculate and output statistics
    unsigned int cpu_bound_burst_count = 0;
    unsigned int io_bound_burst_count = 0;
    unsigned int cpu_bound_total_burst_time = 0;
    unsigned int io_bound_total_burst_time = 0;

    for (int i = 0; i < n_process; i++) {
        for (int j = 0; j < processes[i].numBursts; j++) {
            if (processes[i].cpu_bound) {
                io_bound_total_burst_time += processes[i].cpu_io_bursts[j][0];
                io_bound_burst_count++;
            } else {
                cpu_bound_total_burst_time += processes[i].cpu_io_bursts[j][0];
                cpu_bound_burst_count++;
            }
        }
    }

    cpu_bound_total_burst_time += t_cs * cpu_bound_cs_count;
    io_bound_total_burst_time += t_cs * io_bound_cs_count;

    double util_percentage = (double)cpu_time / time * 100;
    double cpu_avg_wait = cpu_bound_burst_count ? (double)cpu_bound_total_wait_time / cpu_bound_burst_count : 0;
    double io_avg_wait = io_bound_burst_count ? (double)io_bound_total_wait_time / io_bound_burst_count : 0;
    double overall_avg_wait = (double)(cpu_bound_total_wait_time + io_bound_total_wait_time) / (cpu_bound_burst_count + io_bound_burst_count);
    double cpu_avg_turnaround = cpu_bound_burst_count ? (double)(cpu_bound_total_wait_time + cpu_bound_total_burst_time) / cpu_bound_burst_count : 0;
    double io_avg_turnaround = io_bound_burst_count ? (double)(io_bound_total_wait_time + io_bound_total_burst_time) / io_bound_burst_count : 0;
    double overall_avg_turnaround = (double)(cpu_bound_total_wait_time + cpu_bound_total_burst_time + io_bound_total_wait_time + io_bound_total_burst_time) / (cpu_bound_burst_count + io_bound_burst_count);

    fprintf(output, "Algorithm SJF\n");
    fprintf(output, "-- CPU utilization: %.3f%%\n", util_percentage);
    fprintf(output, "-- CPU-bound average wait time: %.3f ms\n", cpu_avg_wait);
    fprintf(output, "-- I/O-bound average wait time: %.3f ms\n", io_avg_wait);
    fprintf(output, "-- overall average wait time: %.3f ms\n", overall_avg_wait);
    fprintf(output, "-- CPU-bound average turnaround time: %.3f ms\n", cpu_avg_turnaround);
    fprintf(output, "-- I/O-bound average turnaround time: %.3f ms\n", io_avg_turnaround);
    fprintf(output, "-- overall average turnaround time: %.3f ms\n", overall_avg_turnaround);
    fprintf(output, "-- CPU-bound number of context switches: %d\n", cpu_bound_cs_count);
    fprintf(output, "-- I/O-bound number of context switches: %d\n", io_bound_cs_count);
    fprintf(output, "-- overall number of context switches: %d\n", cpu_bound_cs_count + io_bound_cs_count);
    fprintf(output, "-- CPU-bound number of preemptions: 0\n");
    fprintf(output, "-- I/O-bound number of preemptions: 0\n");
    fprintf(output, "-- overall number of preemptions: 0\n\n");

    // Free memory
    free(ready_queue);
    free(op_queue);
}

// SRT Algorithm Implementation
void srt(Process *processes, int n_process, double t_cs, double alpha, FILE *output) {
    printf("time 0ms: Simulator started for SRT [Q empty]\n");

    unsigned int time = 0;
    unsigned int cpu_time = 0;
    unsigned int cpu_bound_total_wait_time = 0;
    unsigned int io_bound_total_wait_time = 0;
    unsigned int cpu_bound_cs_count = 0;
    unsigned int io_bound_cs_count = 0;
    unsigned int cpu_bound_preemptions = 0;
    unsigned int io_bound_preemptions = 0;

    // Sort processes by arrival time
    qsort(processes, n_process, sizeof(Process), process_compare);

    Process **ready_queue = NULL;
    int ready_queue_size = 0;
    Operation *op_queue = NULL;
    int op_queue_size = 0;

    for (int i = 0; i < n_process; i++) {
        op_queue_size++;
        op_queue = realloc(op_queue, op_queue_size * sizeof(Operation));
        op_queue[op_queue_size - 1].type = ARRIVE;
        op_queue[op_queue_size - 1].time = processes[i].arrival;
        op_queue[op_queue_size - 1].process = &processes[i];
    }

    Process *current_process = NULL;
    bool cpu_busy = false;
    while (op_queue_size > 0 || ready_queue_size > 0 || cpu_busy) {
        if (!cpu_busy && ready_queue_size > 0) {
            cpu_busy = true;
            Process *proc = ready_queue[0];
            ready_queue_size--;
            for (int i = 0; i < ready_queue_size; i++) {
                ready_queue[i] = ready_queue[i + 1];
            }
            ready_queue = realloc(ready_queue, ready_queue_size * sizeof(Process *));
            op_queue_size++;
            op_queue = realloc(op_queue, op_queue_size * sizeof(Operation));
            op_queue[op_queue_size - 1].type = DEQUEUE;
            op_queue[op_queue_size - 1].time = time;
            op_queue[op_queue_size - 1].process = proc;
        }

        Operation op = op_queue[0];
        op_queue_size--;
        for (int i = 0; i < op_queue_size; i++) {
            op_queue[i] = op_queue[i + 1];
        }
        op_queue = realloc(op_queue, op_queue_size * sizeof(Operation));
        time = op.time;

        switch (op.type) {
            case ARRIVE:
            case IO_END:
                ready_queue_size++;
                ready_queue = realloc(ready_queue, ready_queue_size * sizeof(Process *));
                ready_queue[ready_queue_size - 1] = op.process;

                qsort(ready_queue, ready_queue_size, sizeof(Process *), process_compare);

                if (op.type == ARRIVE) {
                    printf("time %ums: Process %s (tau %ums) arrived; added to ready queue ", time, op.process->id, op.process->tau);
                } else {
                    printf("time %ums: Process %s (tau %ums) completed I/O; added to ready queue ", time, op.process->id, op.process->tau);
                }
                print_queue(ready_queue, ready_queue_size);
                op.process->ready_queue_add_time = time;

                // Check for preemption
                if (cpu_busy && current_process && current_process->remaining_time > op.process->tau) {
                    printf("time %ums: Process %s (tau %ums) will preempt %s ", time, op.process->id, op.process->tau, current_process->id);
                    print_queue(ready_queue, ready_queue_size);

                    op_queue_size++;
                    op_queue = realloc(op_queue, op_queue_size * sizeof(Operation));
                    op_queue[op_queue_size - 1].type = CPU_UNBUSY;
                    op_queue[op_queue_size - 1].time = time + (t_cs / 2);
                    op_queue[op_queue_size - 1].process = current_process;

                    ready_queue_size++;
                    ready_queue = realloc(ready_queue, ready_queue_size * sizeof(Process *));
                    ready_queue[ready_queue_size - 1] = current_process;
                    current_process = NULL;

                    if (current_process->cpu_bound) {
                        cpu_bound_preemptions++;
                    } else {
                        io_bound_preemptions++;
                    }

                    cpu_busy = false;
                }
                break;

            case DEQUEUE:
                cpu_busy = true;
                current_process = op.process;
                op_queue_size++;
                op_queue = realloc(op_queue, op_queue_size * sizeof(Operation));
                op_queue[op_queue_size - 1].type = BURST;
                op_queue[op_queue_size - 1].time = time + (t_cs / 2);
                op_queue[op_queue_size - 1].process = current_process;

                if (current_process->cpu_bound) {
                    cpu_bound_total_wait_time += time - current_process->ready_queue_add_time;
                } else {
                    io_bound_total_wait_time += time - current_process->ready_queue_add_time;
                }
                break;

            case BURST: {
                unsigned int burst_time = current_process->remaining_time;
                printf("time %ums: Process %s (tau %ums) started using the CPU for %ums burst ", time, current_process->id, current_process->tau, burst_time);
                print_queue(ready_queue, ready_queue_size);

                op_queue_size++;
                op_queue = realloc(op_queue, op_queue_size * sizeof(Operation));
                op_queue[op_queue_size - 1].type = EXPIRE;
                op_queue[op_queue_size - 1].time = time + burst_time;
                op_queue[op_queue_size - 1].process = current_process;
                current_process->current_burst_elapsed += burst_time;

                cpu_time += burst_time;

                if (current_process->cpu_bound) {
                    cpu_bound_cs_count++;
                } else {
                    io_bound_cs_count++;
                }
                break;
            }

            case EXPIRE: {
                cpu_busy = false;
                unsigned int actual_burst_time = time - current_process->burst_start_time;
                unsigned int old_tau = current_process->tau;

                if (current_process->index >= current_process->numBursts - 1) {
                    printf("time %ums: Process %s terminated ", time, current_process->id);
                    print_queue(ready_queue, ready_queue_size);
                } else {
                    current_process->tau = ceil(alpha * actual_burst_time + (1.0 - alpha) * old_tau);
                    printf("time %ums: Process %s (tau %ums) completed a CPU burst; %lu bursts to go ", time, current_process->id, old_tau, current_process->numBursts - current_process->index - 1);
                    print_queue(ready_queue, ready_queue_size);

                    printf("time %ums: Recalculated tau for process %s: old tau %ums ==> new tau %ums ", time, current_process->id, old_tau, current_process->tau);
                    print_queue(ready_queue, ready_queue_size);

                    unsigned int unblock_time = time + current_process->cpu_io_bursts[current_process->index][1] + (t_cs / 2);
                    op_queue_size++;
                    op_queue = realloc(op_queue, op_queue_size * sizeof(Operation));
                    op_queue[op_queue_size - 1].type = IO_END;
                    op_queue[op_queue_size - 1].time = unblock_time;
                    op_queue[op_queue_size - 1].process = current_process;
                }

                current_process->current_burst_elapsed = 0;
                current_process->index++;
                time += t_cs / 2;
                break;
            }

            default:
                break;
        }
    }

    printf("time %ums: Simulator ended for SRT ", time);
    print_queue(ready_queue, ready_queue_size);
    printf("\n");

    // Calculate and output statistics
    unsigned int cpu_bound_burst_count = 0;
    unsigned int io_bound_burst_count = 0;
    unsigned int cpu_bound_total_burst_time = 0;
    unsigned int io_bound_total_burst_time = 0;

    for (int i = 0; i < n_process; i++) {
        for (int j = 0; j < processes[i].numBursts; j++) {
            if (processes[i].cpu_bound) {
                io_bound_total_burst_time += processes[i].cpu_io_bursts[j][0];
                io_bound_burst_count++;
            } else {
                cpu_bound_total_burst_time += processes[i].cpu_io_bursts[j][0];
                cpu_bound_burst_count++;
            }
        }
    }

    cpu_bound_total_burst_time += t_cs * cpu_bound_cs_count;
    io_bound_total_burst_time += t_cs * io_bound_cs_count;

    double util_percentage = (double)cpu_time / time * 100;
    double cpu_avg_wait = cpu_bound_burst_count ? (double)cpu_bound_total_wait_time / cpu_bound_burst_count : 0;
    double io_avg_wait = io_bound_burst_count ? (double)io_bound_total_wait_time / io_bound_burst_count : 0;
    double overall_avg_wait = (double)(cpu_bound_total_wait_time + io_bound_total_wait_time) / (cpu_bound_burst_count + io_bound_burst_count);
    double cpu_avg_turnaround = cpu_bound_burst_count ? (double)(cpu_bound_total_wait_time + cpu_bound_total_burst_time) / cpu_bound_burst_count : 0;
    double io_avg_turnaround = io_bound_burst_count ? (double)(io_bound_total_wait_time + io_bound_total_burst_time) / io_bound_burst_count : 0;
    double overall_avg_turnaround = (double)(cpu_bound_total_wait_time + cpu_bound_total_burst_time + io_bound_total_wait_time + io_bound_total_burst_time) / (cpu_bound_burst_count + io_bound_burst_count);

    fprintf(output, "Algorithm SRT\n");
    fprintf(output, "-- CPU utilization: %.3f%%\n", util_percentage);
    fprintf(output, "-- CPU-bound average wait time: %.3f ms\n", cpu_avg_wait);
    fprintf(output, "-- I/O-bound average wait time: %.3f ms\n", io_avg_wait);
    fprintf(output, "-- overall average wait time: %.3f ms\n", overall_avg_wait);
    fprintf(output, "-- CPU-bound average turnaround time: %.3f ms\n", cpu_avg_turnaround);
    fprintf(output, "-- I/O-bound average turnaround time: %.3f ms\n", io_avg_turnaround);
    fprintf(output, "-- overall average turnaround time: %.3f ms\n", overall_avg_turnaround);
    fprintf(output, "-- CPU-bound number of context switches: %d\n", cpu_bound_cs_count);
    fprintf(output, "-- I/O-bound number of context switches: %d\n", io_bound_cs_count);
    fprintf(output, "-- overall number of context switches: %d\n", cpu_bound_cs_count + io_bound_cs_count);
    fprintf(output, "-- CPU-bound number of preemptions: %d\n", cpu_bound_preemptions);
    fprintf(output, "-- I/O-bound number of preemptions: %d\n", io_bound_preemptions);
    fprintf(output, "-- overall number of preemptions: %d\n\n", cpu_bound_preemptions + io_bound_preemptions);

    // Free memory
    free(ready_queue);
    free(op_queue);
}
