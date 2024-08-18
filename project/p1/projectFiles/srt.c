#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct process {
    char pid[10];
    int arrival_time;
    int *cpu_bursts;
    int *io_bursts;
    int is_cpu_bound;
    int context_switches;
    int preempt_remaining_time;
    int last_ready_time;
    int last_burst_time;
    int total_burst_time;
    int *waiting_time;
    int *static_cpu_bursts;
} process;

typedef struct SRT {
    process **queue;
    int queue_size;
    process **ready;
    int ready_size;
    process **io_list;
    int io_list_size;
    process *waiting_io;
    int *lengths;
    process *current_process;
    int current_time;
    int context_switch;
    int context_switching;
    int theta;
    int alpha;
    int x;
    int cpu_burst_start_time;
    int cpu_use_time;
    int cpu_bound_context_switches;
    int io_bound_context_switches;
    int io_bound_wait_time_total;
    int cpu_bound_wait_time_total;
    int cpu_bound_turn_time_total;
    int io_bound_turn_time_total;
    int cpu_bound_preemptions;
    int io_bound_preemptions;
    int cpu_bound_bursts;
    int io_bound_bursts;
    process **queue_wait;
    int queue_wait_size;
    process *io_arrived;
    int time_context_switching;
} SRT;

void init_srt(SRT *srt, process **queue, int queue_size, int context_switch, int theta, int alpha) {
    srt->queue = queue;
    srt->queue_size = queue_size;
    srt->ready = (process **)calloc(queue_size, sizeof(process *));
    srt->ready_size = 0;
    srt->io_list = (process **)calloc(queue_size, sizeof(process *));
    srt->io_list_size = 0;
    srt->waiting_io = NULL;
    srt->lengths = (int *)calloc(queue_size, sizeof(int));
    srt->current_process = NULL;
    srt->current_time = 0;
    srt->context_switch = context_switch;
    srt->context_switching = 0;
    srt->theta = theta;
    srt->alpha = alpha;
    srt->x = 0;
    srt->cpu_burst_start_time = 0;
    srt->cpu_use_time = 0;
    srt->cpu_bound_context_switches = 0;
    srt->io_bound_context_switches = 0;
    srt->io_bound_wait_time_total = 0;
    srt->cpu_bound_wait_time_total = 0;
    srt->cpu_bound_turn_time_total = 0;
    srt->io_bound_turn_time_total = 0;
    srt->cpu_bound_preemptions = 0;
    srt->io_bound_preemptions = 0;
    srt->cpu_bound_bursts = 0;
    srt->io_bound_bursts = 0;
    srt->queue_wait = (process **)calloc(queue_size, sizeof(process *));
    srt->queue_wait_size = 0;
    srt->io_arrived = NULL;
    srt->time_context_switching = 0;
}

void place_process(SRT *srt, process *proc, int tau) {
    if (srt->ready_size == 0) {
        srt->ready[srt->ready_size++] = proc;
        proc->last_ready_time = srt->current_time;
        return;
    }
    for (int i = 0; i < srt->ready_size; i++) {
        int burst = srt->lengths[srt->ready[i] - srt->queue[0]] - srt->ready[i]->preempt_remaining_time;
        if (srt->lengths[srt->ready[i] - srt->queue[0]] == tau) {
            if (strcmp(proc->pid, srt->ready[i]->pid) < 0) {
                for (int j = srt->ready_size; j > i; j--) {
                    srt->ready[j] = srt->ready[j - 1];
                }
                srt->ready[i] = proc;
                proc->last_ready_time = srt->current_time;
                srt->ready_size++;
                return;
            }
        }
        if (srt->ready[i]->preempt_remaining_time != 0) {
            if (proc->preempt_remaining_time != 0 && srt->ready[i]->preempt_remaining_time > proc->preempt_remaining_time) {
                for (int j = srt->ready_size; j > i; j--) {
                    srt->ready[j] = srt->ready[j - 1];
                }
                srt->ready[i] = proc;
                proc->last_ready_time = srt->current_time;
                srt->ready_size++;
                return;
            }
            if (srt->ready[i]->preempt_remaining_time > tau) {
                for (int j = srt->ready_size; j > i; j--) {
                    srt->ready[j] = srt->ready[j - 1];
                }
                srt->ready[i] = proc;
                proc->last_ready_time = srt->current_time;
                srt->ready_size++;
                return;
            }
        } else if (proc->preempt_remaining_time != 0) {
            if (srt->lengths[srt->ready[i] - srt->queue[0]] > proc->preempt_remaining_time) {
                for (int j = srt->ready_size; j > i; j--) {
                    srt->ready[j] = srt->ready[j - 1];
                }
                srt->ready[i] = proc;
                proc->last_ready_time = srt->current_time;
                srt->ready_size++;
                return;
            }
        } else if (srt->lengths[srt->ready[i] - srt->queue[0]] > tau) {
            for (int j = srt->ready_size; j > i; j--) {
                srt->ready[j] = srt->ready[j - 1];
            }
            srt->ready[i] = proc;
            proc->last_ready_time = srt->current_time;
            srt->ready_size++;
            return;
        }
    }
    srt->ready[srt->ready_size++] = proc;
    proc->last_ready_time = srt->current_time;
}

void check_arrival(SRT *srt) {
    int p = 0;
    while (p < srt->queue_size) {
        if (srt->queue[p]->arrival_time == srt->current_time) {
            int tau_0 = (int)ceil(1.0 / srt->theta);
            srt->lengths[srt->queue[p] - srt->queue[0]] = tau_0;
            place_process(srt, srt->queue[p], tau_0);
            if (srt->current_time < 10000) {
                printf("time %dms: Process %s (tau %dms) arrived; added to ready queue [Q", srt->current_time, srt->queue[p]->pid, tau_0);
                for (int i = 0; i < srt->ready_size; i++) {
                    printf(" %s", srt->ready[i]->pid);
                }
                printf("]\n");
            }
            for (int j = p; j < srt->queue_size - 1; j++) {
                srt->queue[j] = srt->queue[j + 1];
            }
            srt->queue_size--;
            p = 0;
        } else {
            p++;
        }
    }
}

void cpu_burst(SRT *srt) {
    if (srt->current_process->cpu_bursts[0] > 0) {
        srt->current_process->cpu_bursts[0]--;
        srt->cpu_use_time++;
        if (srt->current_process->cpu_bursts[0] == 0) {
            if (srt->current_process->is_cpu_bound) {
                srt->cpu_bound_bursts++;
            } else {
                srt->io_bound_bursts++;
            }
            if (srt->current_time < 10000) {
                if (srt->current_process->cpu_bursts[1] == 1) {
                    printf("time %dms: Process %s (tau %dms) completed a CPU burst; 1 burst to go [Q", srt->current_time, srt->current_process->pid, srt->lengths[srt->current_process - srt->queue[0]]);
                } else {
                    printf("time %dms: Process %s (tau %dms) completed a CPU burst; %d bursts to go [Q", srt->current_time, srt->current_process->pid, srt->lengths[srt->current_process - srt->queue[0]], srt->current_process->cpu_bursts[1] - 1);
                }
                for (int i = 0; i < srt->ready_size; i++) {
                    printf(" %s", srt->ready[i]->pid);
                }
                printf("]\n");
            }
            srt->context_switching = srt->context_switch / 2;
            srt->current_process->cpu_bursts++;
            srt->current_process->last_burst_time = 0;
            srt->current_process->preempt_remaining_time = 0;
            if (srt->current_process->io_bursts[0] > 0) {
                int block_until = srt->current_time + srt->context_switching + srt->current_process->io_bursts[0];
                int tau_n = (int)ceil(srt->x * srt->alpha + (1 - srt->alpha) * srt->lengths[srt->current_process - srt->queue[0]]);
                if (srt->current_time < 10000) {
                    printf("time %dms: Recalculated tau for process %s: old tau %dms ==> new tau %dms [Q", srt->current_time, srt->current_process->pid, srt->lengths[srt->current_process - srt->queue[0]], tau_n);
                    for (int i = 0; i < srt->ready_size; i++) {
                        printf(" %s", srt->ready[i]->pid);
                    }
                    printf("]\n");
                }
                srt->lengths[srt->current_process - srt->queue[0]] = tau_n;
                if (srt->current_time < 10000) {
                    printf("time %dms: Process %s switching out of CPU; blocking on I/O until time %dms [Q", srt->current_time, srt->current_process->pid, block_until);
                    for (int i = 0; i < srt->ready_size; i++) {
                        printf(" %s", srt->ready[i]->pid);
                    }
                    printf("]\n");
                }
                io_place(srt, srt->current_process, block_until);
                srt->current_process = NULL;
            } else {
                int wait_sum = 0;
                if (srt->current_process->is_cpu_bound) {
                    srt->cpu_bound_context_switches += srt->current_process->context_switches;
                    for (int i = 0; i < srt->ready_size; i++) {
                        wait_sum += fmax(srt->current_process->waiting_time[i] - 1, 0);
                    }
                    srt->cpu_bound_wait_time_total += fmax(wait_sum - srt->current_process->context_switches + srt->current_process->cpu_bursts[0], 0);
                    srt->cpu_bound_turn_time_total += (wait_sum + srt->current_process->total_burst_time + srt->context_switch * srt->current_process->cpu_bursts[0]);
                } else {
                    srt->io_bound_context_switches += srt->current_process->context_switches;
                    for (int i = 0; i < srt->ready_size; i++) {
                        wait_sum += fmax(srt->current_process->waiting_time[i] - 1, 0);
                    }
                    srt->io_bound_wait_time_total += fmax(wait_sum - srt->current_process->context_switches + srt->current_process->cpu_bursts[0], 0);
                    srt->io_bound_turn_time_total += (wait_sum + srt->current_process->total_burst_time + srt->context_switch * srt->current_process->cpu_bursts[0]);
                }
                printf("time %dms: Process %s terminated [Q", srt->current_time, srt->current_process->pid);
                for (int i = 0; i < srt->ready_size; i++) {
                    printf(" %s", srt->ready[i]->pid);
                }
                printf("]\n");
                srt->current_process = NULL;
            }
        }
    }
}

void io_place(SRT *srt, process *proc, int block) {
    for (int i = 0; i < srt->io_list_size; i++) {
        process *p = srt->io_list[i];
        int block_until = srt->current_time + srt->context_switching + p->io_bursts[0];
        if (block < block_until) {
            for (int j = srt->io_list_size; j > i; j--) {
                srt->io_list[j] = srt->io_list[j - 1];
            }
            srt->io_list[i] = proc;
            srt->io_list_size++;
            return;
        }
        if (block == block_until) {
            if (strcmp(proc->pid, p->pid) < 0) {
                for (int j = srt->io_list_size; j > i; j--) {
                    srt->io_list[j] = srt->io_list[j - 1];
                }
                srt->io_list[i] = proc;
                srt->io_list_size++;
                return;
            }
            srt->io_list[i + 1] = proc;
            srt->io_list_size++;
            return;
        }
    }
    srt->io_list[srt->io_list_size++] = proc;
}

void io_burst(SRT *srt) {
    for (int i = 0; i < srt->io_list_size; i++) {
        process *p = srt->io_list[i];
        int block_until = srt->current_time + srt->context_switching + p->io_bursts[0];
        if (srt->current_time >= block_until) {
            p->io_bursts++;
            int rem_time = -1;
            if (srt->current_process != NULL) {
                rem_time = srt->lengths[srt->current_process - srt->queue[0]] - (srt->current_time + srt->context_switch - srt->cpu_burst_start_time);
                if (srt->current_process->preempt_remaining_time != 0) {
                    rem_time = srt->current_process->preempt_remaining_time - srt->current_time + srt->cpu_burst_start_time;
                }
            }
            if (srt->current_process != NULL && srt->lengths[p - srt->queue[0]] < rem_time && rem_time != -1) {
                place_process(srt, p, srt->lengths[p - srt->queue[0]]);
                if (srt->current_process->is_cpu_bound) {
                    srt->cpu_bound_preemptions++;
                } else {
                    srt->io_bound_preemptions++;
                }
                if (srt->current_process->preempt_remaining_time == 0) {
                    if (srt->current_time < 10000) {
                        printf("time %dms: Process %s (tau %dms) completed I/O; preempting %s (predicted remaining time %dms) [Q", srt->current_time + srt->context_switching, p->pid, srt->lengths[p - srt->queue[0]], srt->current_process->pid, srt->lengths[srt->current_process - srt->queue[0]] - (srt->current_time - srt->cpu_burst_start_time));
                        for (int i = 0; i < srt->ready_size; i++) {
                            printf(" %s", srt->ready[i]->pid);
                        }
                        printf("]\n");
                    }
                    srt->current_process->preempt_remaining_time = srt->lengths[srt->current_process - srt->queue[0]] - (srt->current_time - srt->cpu_burst_start_time);
                } else {
                    if (srt->current_time < 10000) {
                        printf("time %dms: Process %s (tau %dms) completed I/O; preempting %s (predicted remaining time %dms) [Q", srt->current_time + srt->context_switching, p->pid, srt->lengths[p - srt->queue[0]], srt->current_process->pid, srt->current_process->preempt_remaining_time - srt->current_time + srt->cpu_burst_start_time);
                        for (int i = 0; i < srt->ready_size; i++) {
                            printf(" %s", srt->ready[i]->pid);
                        }
                        printf("]\n");
                    }
                    srt->current_process->preempt_remaining_time -= (srt->current_time - srt->cpu_burst_start_time);
                }
                srt->queue_wait[srt->queue_wait_size++] = srt->current_process;
                srt->current_process = NULL;
                srt->context_switching += (srt->context_switch / 2);
            } else {
                place_process(srt, p, srt->lengths[p - srt->queue[0]]);
                if (srt->current_time < 10000) {
                    printf("time %dms: Process %s (tau %dms) completed I/O; added to ready queue [Q", srt->current_time + srt->context_switching, p->pid, srt->lengths[p - srt->queue[0]]);
                    for (int i = 0; i < srt->ready_size; i++) {
                        printf(" %s", srt->ready[i]->pid);
                    }
                    printf("]\n");
                }
            }
            for (int j = i; j < srt->io_list_size - 1; j++) {
                srt->io_list[j] = srt->io_list[j + 1];
            }
            srt->io_list_size--;
            i--;
        }
    }
}

void run(SRT *srt) {
    printf("time %dms: Simulator started for SRT [Q", srt->current_time);
    for (int i = 0; i < srt->ready_size; i++) {
        printf(" %s", srt->ready[i]->pid);
    }
    printf("]\n");
    
    while (srt->queue_size > 0 || srt->ready_size > 0 || srt->current_process != NULL || srt->io_list_size > 0 || srt->context_switching > 0) {
        if (srt->io_list_size > 0) {
            io_burst(srt);
        }
        check_arrival(srt);
        srt->current_time++;
        if (srt->queue_wait_size > 0) {
            for (int i = 0; i < srt->queue_wait_size; i++) {
                if (srt->queue_wait[i] != NULL) {
                    place_process(srt, srt->queue_wait[i], srt->lengths[srt->queue_wait[i] - srt->queue[0]]);
                    srt->queue_wait[i] = NULL;
                }
            }
        }
        if (srt->context_switching > 0) {
            srt->context_switching--;
            srt->time_context_switching++;
        } else if (srt->current_process == NULL && srt->ready_size > 0) {
            srt->current_process = srt->ready[0];
            for (int i = 0; i < srt->ready_size - 1; i++) {
                srt->ready[i] = srt->ready[i + 1];
            }
            srt->ready_size--;
            srt->context_switching = fmax((srt->context_switch / 2) - 1, 0);
            if (srt->current_process->last_burst_time != 0 && srt->current_process->last_burst_time != srt->current_process->cpu_bursts[0]) {
                if (srt->current_time < 10000) {
                    printf("time %dms: Process %s (tau %dms) started using the CPU for remaining %dms of %dms burst [Q", srt->current_time + srt->context_switching, srt->current_process->pid, srt->lengths[srt->current_process - srt->queue[0]], srt->current_process->cpu_bursts[0], srt->current_process->last_burst_time);
                    for (int i = 0; i < srt->ready_size; i++) {
                        printf(" %s", srt->ready[i]->pid);
                    }
                    printf("]\n");
                }
            } else {
                if (srt->current_time < 10000) {
                    printf("time %dms: Process %s (tau %dms) started using the CPU for %dms burst [Q", srt->current_time + srt->context_switching, srt->current_process->pid, srt->lengths[srt->current_process - srt->queue[0]], srt->current_process->cpu_bursts[0]);
                    for (int i = 0; i < srt->ready_size; i++) {
                        printf(" %s", srt->ready[i]->pid);
                    }
                    printf("]\n");
                }
                srt->current_process->last_burst_time = srt->current_process->cpu_bursts[0];
            }
            srt->cpu_burst_start_time = srt->current_time + srt->context_switching;
            srt->current_process->context_switches++;
            srt->current_process->waiting_time[srt->current_process->cpu_bursts[0]] += (srt->current_time - srt->current_process->last_ready_time);
            srt->x = srt->current_process->cpu_bursts[0];
            if (srt->current_process->last_burst_time != 0) {
                srt->x = srt->current_process->last_burst_time;
            }
        } else if (srt->current_process != NULL) {
            cpu_burst(srt);
        }
    }
    printf("time %dms: Simulator ended for SRT [Q", srt->current_time);
    for (int i = 0; i < srt->ready_size; i++) {
        printf(" %s", srt->ready[i]->pid);
    }
    printf("]\n");
}

int main() {

    // Process initialization
    process p1 = {"P1", 0, (int[]){8, 5, 2}, (int[]){10, 15}, 1, 0, 0, 0, 0, 15, NULL, (int[]){8, 5, 2}};
    process p2 = {"P2", 2, (int[]){6, 4, 3}, (int[]){7, 8}, 0, 0, 0, 0, 0, 13, NULL, (int[]){6, 4, 3}};
    process *queue[] = {&p1, &p2};

    // SRT initialization
    SRT srt;
    init_srt(&srt, queue, 2, 8, 2, 1);

    // Running the simulation
    run(&srt);

    // Free allocated memory
    free(srt.ready);
    free(srt.io_list);
    free(srt.lengths);
    free(srt.queue_wait);

    return 0;
}
