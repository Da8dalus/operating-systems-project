#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream> 
#include <iomanip>
#include <list>
#include <algorithm>
#include <set>

#include "next_exp.h"
#include "process.h"
#include "operation.h"

void fcfs (std::vector<Process> processes, std::ofstream& fout, const double t_cs);
void sjf (std::vector<Process> processes, std::ofstream& fout, const double t_cs, const double alpha);
void srt (std::vector<Process> processes, std::ofstream& fout, const double t_cs, const double alpha);
void rr (std::vector<Process> processes, std::ofstream& fout, const double t_cs, const unsigned int t_slice);

int main (int argc, char* argv[]) {
  if (argc < 9) {
    fprintf(stderr, "ERROR: usage: <n> <n_cpu> <seed> <lambda> <ceil> <t_cs> <alpha> <t_slice>\n");
    return 1;
  }
  /** Number of processes to simulate */
  const int n = atoi(argv[1]);
  if (n <= 0 || n > 260) {
    fprintf(stderr, "ERROR: n must be 0 < n <= 260\n");
    return 1;
  }
  /** Number of processes CPU-bound. The rest are I/O-bound */
  const int n_cpu = atoi(argv[2]);
  if (n_cpu < 0 || n_cpu > n) {
    fprintf(stderr, "ERROR: n_cpu must be 0 <= n_cpu <= n\n");
    return 1;
  }
  /** Random number generator seed */
  const int seed = atoi(argv[3]);
  /** Used in the formual for exponential distribution generation */
  const double lambda = atof(argv[4]);
  if (!lambda) {
    fprintf(stderr, "ERROR: invalid lambda given for exp dist\n");
    return 1;
  }
  /** The upper limit to exponential values that can be generated */
  const int exp_ceil = atoi(argv[5]);
  /** The time in ms to perform a context switch */
  const int t_cs = atoi(argv[6]);
  if (t_cs < 0 || t_cs % 2) {
    fprintf(stderr, "ERROR: t_cs must be a positive even integer\n");
    return 1;
  }
  /** For SJF and SRT, alpha */
  const double alpha = atof(argv[7]);
  /** For RR, time slice in milliseconds */
  const int t_slice = atoi(argv[8]);
  if (t_slice < 0) {
    fprintf(stderr, "ERROR: t_slice must be positive\n");
    return 1;
  }

  // Seed the RNG
  srand48(seed);

  printf("<<< PROJECT PART I\n");
  if (n_cpu == 1) printf("<<< -- process set (n=%d) with %d CPU-bound process\n", n, n_cpu);
  else printf("<<< -- process set (n=%d) with %d CPU-bound processes\n", n, n_cpu);
  printf("<<< -- seed=%d; lambda=%f; bound=%d\n", seed, lambda, exp_ceil);

  // Create the processes
  std::vector<Process> processes = std::vector<Process>();
  for (int i = 0; i < n; ++i) {
    char id_letter = i / 10 + 'A';
    char id_number = i % 10 + '0';

    int arrival = floor(next_exp(lambda, exp_ceil));
    int bursts = ceil(drand48() * 32);

    processes.emplace_back(
      std::string() + id_letter + id_number, // id
      i >= n_cpu, // process type
      arrival,
      bursts,
      lambda,
      exp_ceil
    );
  }

  for (Process process : processes) {
    process.print(false);
  }

  // Calculations 
  int numBurstsCPU = 0;
  int totalCPU = 0;
  int totalIO = 0;
  std::pair<int, int> temp;

  double CPUboundCPUburst = 0;
  double CPUboundIOburst = 0;
  
  // CPU Bound
  for (int i = 0; i < n_cpu; i++) {
    temp = processes[i].getBurstTime();
    CPUboundCPUburst += temp.first;
    CPUboundIOburst += temp.second;
    numBurstsCPU += processes[i].getNumBursts();
  }

  totalCPU += CPUboundCPUburst;
  totalIO += CPUboundIOburst;

  CPUboundCPUburst = CPUboundCPUburst ? CPUboundCPUburst / numBurstsCPU : 0.f;
  CPUboundIOburst = CPUboundIOburst ? CPUboundIOburst / (numBurstsCPU-n_cpu) : 0.f;

  int numBurstsIO = 0;
  double IOboundCPUburst = 0;
  double IOboundIOburst = 0;

  // IO Bound
  for(int i = n_cpu; i < n; i++){
    temp = processes[i].getBurstTime();
    IOboundCPUburst += temp.first;
    IOboundIOburst += temp.second;
    numBurstsIO += processes[i].getNumBursts();
  }

  totalCPU += IOboundCPUburst;
  totalIO += IOboundIOburst;

  IOboundCPUburst = IOboundCPUburst ? IOboundCPUburst / numBurstsIO : 0.f;
  IOboundIOburst = IOboundIOburst ? IOboundIOburst / (numBurstsIO - n + n_cpu) : 0.f;

  double ovrCPU = (double)(totalCPU) / (numBurstsCPU + numBurstsIO);
  double ovrIO = (double)(totalIO) / (numBurstsCPU + numBurstsIO - n);

  // Ceil averages
  CPUboundCPUburst = ceil(CPUboundCPUburst * 1000) / 1000;
  IOboundCPUburst = ceil(IOboundCPUburst * 1000) / 1000;
  ovrCPU = ceil(ovrCPU * 1000) / 1000;
  CPUboundIOburst = ceil(CPUboundIOburst * 1000) / 1000;
  IOboundIOburst = ceil(IOboundIOburst * 1000) / 1000;
  ovrIO = ceil(ovrIO * 1000) / 1000;

  // File Output
  std::ofstream fout;
  fout.open("simout.txt");
  fout << std::fixed;
  fout << "-- number of processes: " << n << std::endl;
  fout << "-- number of CPU-bound processes: " << n_cpu << std::endl;
  fout << "-- number of I/O-bound processes: " << n - n_cpu << std::endl;
  fout << "-- CPU-bound average CPU burst time: " << std::setprecision(3) << CPUboundCPUburst << " ms" << std::endl;
  fout << "-- I/O-bound average CPU burst time: " << std::setprecision(3) << IOboundCPUburst << " ms" << std::endl;
  fout << "-- overall average CPU burst time: " << std::setprecision(3) << ovrCPU << " ms" << std::endl;
  fout << "-- CPU-bound average I/O burst time: " << std::setprecision(3) << CPUboundIOburst << " ms" << std::endl;
  fout << "-- I/O-bound average I/O burst time: " << std::setprecision(3) << IOboundIOburst << " ms" << std::endl;
  fout << "-- overall average I/O burst time: " << std::setprecision(3) << ovrIO << " ms" << std::endl;
  fout << std::endl;

  // Run algorithms
  printf("\n<<< PROJECT PART II\n<<< -- t_cs=%ums; alpha=%.2f; t_slice=%ums\n", t_cs, alpha, t_slice);

  fcfs(processes, fout, t_cs);
  sjf(processes, fout, t_cs, alpha);
  srt(processes, fout, t_cs, alpha);
  rr(processes, fout, t_cs, t_slice);

  fout.close();

  return 0;
}

void print_queue (std::set<Process*, decltype(&sjf_srt_proc_sort)>& queue) {
  if (queue.empty()) printf("[Q empty]\n");
  else {
    printf("[Q");
    for (Process* process : queue) {
      printf(" %s", process->id.c_str());
    }
    printf("]\n");
  }
}
void print_queue (std::list<Process*>& queue) {
  if (queue.empty()) printf("[Q empty]\n");
  else {
    printf("[Q");
    for (Process* process : queue) {
      printf(" %s", process->id.c_str());
    }
    printf("]\n");
  }
}

void fcfs(std::vector<Process> processes, std::ofstream& fout, const double t_cs) {
  printf("time 0ms: Simulator started for FCFS [Q empty]\n");

  unsigned int cpu_time = 0;

  unsigned int cpu_bound_total_wait_time = 0;
  unsigned int io_bound_total_wait_time = 0;

  unsigned int cpu_bound_cs_count = 0;
  unsigned int io_bound_cs_count = 0;

  std::sort(processes.begin(), processes.end(), [](Process& p1, Process& p2) {
      return p1.arrival_time == p2.arrival_time
          ? p1.id.compare(p2.id) < 0
          : p1.arrival_time < p2.arrival_time;
  });

  unsigned int time = 0;
  std::list<Process*> ready_queue = std::list<Process*>();
  std::set<Operation> op_queue = std::set<Operation>();

  for (Process& process : processes) op_queue.emplace(OpType::ARRIVE, process.arrival_time, &process);

  bool cpu_busy = false;
  while (!op_queue.empty() || !ready_queue.empty()) {
    if (!cpu_busy && !ready_queue.empty()) {
      cpu_busy = true;
      op_queue.emplace(OpType::DEQUEUE, time, ready_queue.front());
    }

    const Operation op = *op_queue.begin();
    op_queue.erase(op_queue.begin());
    time = op.time;

    switch (op.type) {
      case OpType::ARRIVE: {
        printf("time %ums: Process %s arrived; added to ready queue ", time, op.process->id.c_str());
        ready_queue.push_back(op.process);
        print_queue(ready_queue);
        op.process->ready_queue_add_time = time;
        break;
      }
      case OpType::DEQUEUE: {
        cpu_busy = true;
        ready_queue.pop_front();
        op_queue.emplace(OpType::BURST, time + (t_cs / 2), op.process);

        if (op.process->type) io_bound_total_wait_time += time - op.process->ready_queue_add_time;
        else cpu_bound_total_wait_time += time - op.process->ready_queue_add_time;

        break;
      }
      case OpType::BURST: {
        unsigned int burst_time = op.process->getCurrentBurstTotalCPU();
        printf("time %ums: Process %s started using the CPU for %ums burst ", time, op.process->id.c_str(), burst_time);
        print_queue(ready_queue);
        op.process->burst_start_time = time;

        op_queue.emplace(OpType::EXPIRE, time + burst_time, op.process);
        op.process->current_burst_elapsed += burst_time;

        cpu_time += burst_time;

        if (op.process->type) ++io_bound_cs_count;
        else ++cpu_bound_cs_count;

        break;
      }
      case OpType::EXPIRE: {
          cpu_busy = false;
          bool finished_burst = true; 

          if (finished_burst) {
            if (op.process->burst_index >= op.process->bursts.size() - 1) {
              printf("time %ums: Process %s terminated ", time, op.process->id.c_str());
              print_queue(ready_queue);
            } else {
              unsigned int unblock_time = time + op.process->getIOTime() + (t_cs / 2);

              op.process->current_burst_elapsed = 0;
              ++op.process->burst_index;
              const size_t remaining = op.process->bursts.size() - op.process->burst_index;
              printf("time %ums: Process %s completed a CPU burst; %lu burst%s to go ", time, op.process->id.c_str(), remaining, remaining == 1 ? "" : "s");
              print_queue(ready_queue);

              printf("time %ums: Process %s switching out of CPU; blocking on I/O until time %ums ", time, op.process->id.c_str(), unblock_time);
              print_queue(ready_queue);
              op_queue.emplace(OpType::IO_END, unblock_time, op.process);
            }
        }

        time += t_cs / 2;

        break;
      }
      case OpType::IO_END: {
        ready_queue.push_back(op.process);
        op.process->ready_queue_add_time = time;
        printf("time %ums: Process %s completed I/O; added to ready queue ", time, op.process->id.c_str());
        print_queue(ready_queue);

        break;
      }
      default: break;
    }
  }

  printf("time %ums: Simulator ended for FCFS ", time);
  print_queue(ready_queue);
  printf("\n");
  unsigned int cpu_bound_burst_count = 0;
  unsigned int io_bound_burst_count = 0;

  unsigned int cpu_bound_total_burst_time = 0;
  unsigned int io_bound_total_burst_time = 0;
  for (const Process& process : processes) {
    for (const std::pair<unsigned int, unsigned int>& burst : process.bursts) {
      if (process.type) io_bound_total_burst_time += burst.first;
      else cpu_bound_total_burst_time += burst.first;
    }

    if (process.type) io_bound_burst_count += process.bursts.size();
    else cpu_bound_burst_count += process.bursts.size();
  }

  cpu_bound_total_burst_time += t_cs * cpu_bound_cs_count;
  io_bound_total_burst_time += t_cs * io_bound_cs_count;

  const double util_percentage = (double)cpu_time / time * 100;
  double cpu_avg_wait = (double)cpu_bound_total_wait_time / cpu_bound_burst_count;
  double io_avg_wait = (double)io_bound_total_wait_time / io_bound_burst_count;
  double overall_avg_wait = (double)(cpu_bound_total_wait_time + io_bound_total_wait_time) / (cpu_bound_burst_count + io_bound_burst_count);
  double cpu_avg_turnaround = (double)(cpu_bound_total_wait_time + cpu_bound_total_burst_time) / cpu_bound_burst_count;
  double io_avg_turnaround = (double)(io_bound_total_wait_time + io_bound_total_burst_time) / io_bound_burst_count;
  double overall_avg_turnaround = (double)(cpu_bound_total_wait_time + cpu_bound_total_burst_time + io_bound_total_wait_time + io_bound_total_burst_time) / (cpu_bound_burst_count + io_bound_burst_count);

  fout << "Algorithm FCFS" << std::endl;
  fout << "-- CPU utilization: " << ceil(util_percentage * 1000) / 1000 << "%" << std::endl;
  fout << "-- CPU-bound average wait time: " << ceil(cpu_avg_wait * 1000) / 1000 << " ms" << std::endl;
  fout << "-- I/O-bound average wait time: " << ceil(io_avg_wait * 1000) / 1000 << " ms" << std::endl;
  fout << "-- overall average wait time: " << ceil(overall_avg_wait * 1000) / 1000 << " ms" << std::endl;
  fout << "-- CPU-bound average turnaround time: " << ceil(cpu_avg_turnaround * 1000) / 1000 << " ms" << std::endl;
  fout << "-- I/O-bound average turnaround time: " << ceil(io_avg_turnaround * 1000) / 1000 << " ms" << std::endl;
  fout << "-- overall average turnaround time: " << ceil(overall_avg_turnaround * 1000) / 1000 << " ms" << std::endl;
  fout << "-- CPU-bound number of context switches: " << cpu_bound_cs_count << std::endl;
  fout << "-- I/O-bound number of context switches: " << io_bound_cs_count << std::endl;
  fout << "-- overall number of context switches: " << cpu_bound_cs_count + io_bound_cs_count << std::endl;
  fout << "-- CPU-bound number of preemptions: 0" << std::endl;
  fout << "-- I/O-bound number of preemptions: 0" << std::endl;
  fout << "-- overall number of preemptions: 0" << std::endl << std::endl;
}

void sjf(std::vector<Process> processes, std::ofstream& fout, const double t_cs, const double alpha) {
  printf("time 0ms: Simulator started for SJF [Q empty]\n");

  unsigned int time = 0;
  unsigned int cpu_time = 0;
  unsigned int cpu_bound_total_wait_time = 0;
  unsigned int io_bound_total_wait_time = 0;
  unsigned int cpu_bound_cs_count = 0;
  unsigned int io_bound_cs_count = 0;

  std::sort(processes.begin(), processes.end(), [](Process& p1, Process& p2) {
    return p1.arrival_time == p2.arrival_time
      ? p1.id.compare(p2.id) < 0
      : p1.arrival_time < p2.arrival_time;
  });

  std::set<Process*, decltype(&sjf_srt_proc_sort)> ready_queue = std::set<Process*, decltype(&sjf_srt_proc_sort)>(&sjf_srt_proc_sort);
  std::set<Operation> op_queue;

  for (Process& process : processes) {
    op_queue.emplace(OpType::ARRIVE, process.arrival_time, &process);
  }

  bool cpu_busy = false;
  while (!op_queue.empty() || !ready_queue.empty() || cpu_busy) {
    if (!cpu_busy && !ready_queue.empty()) {
      cpu_busy = true;
      Process* proc = *ready_queue.begin(); // This will not actually be used
      op_queue.emplace(OpType::DEQUEUE, time, proc);
    }

    const Operation op = *op_queue.begin();
    op_queue.erase(op_queue.begin());
    time = op.time;

    switch (op.type) {
      case OpType::ARRIVE:
      case OpType::IO_END: {
        ready_queue.insert(op.process);

        if (op.type == OpType::ARRIVE) {
          printf("time %ums: Process %s (tau %ums) arrived; added to ready queue ", 
                  time, op.process->id.c_str(), op.process->tau);
        } else {
          printf("time %ums: Process %s (tau %ums) completed I/O; added to ready queue ", 
                  time, op.process->id.c_str(), op.process->tau);
        }
        print_queue(ready_queue);
        op.process->ready_queue_add_time = time;
        break;
      }
      case OpType::DEQUEUE: {
        cpu_busy = true;
        Process* dq_proc = *ready_queue.begin();
        ready_queue.erase(ready_queue.begin());
        op_queue.emplace(OpType::BURST, time + (t_cs / 2), dq_proc);

        if (dq_proc->type) io_bound_total_wait_time += time - dq_proc->ready_queue_add_time;
        else cpu_bound_total_wait_time += time - dq_proc->ready_queue_add_time;

        break;
      }
      case OpType::BURST: {
        unsigned int burst_time = op.process->getCPUTimeRemaining();
        printf("time %ums: Process %s (tau %ums) started using the CPU for %ums burst ", 
                time, op.process->id.c_str(), op.process->tau, burst_time);
        print_queue(ready_queue);
        op.process->burst_start_time = time;

        op_queue.emplace(OpType::EXPIRE, time + burst_time, op.process);
        op.process->current_burst_elapsed += burst_time;

        cpu_time += burst_time;

        if (op.process->type) ++io_bound_cs_count;
        else ++cpu_bound_cs_count;

        break;
      }
      case OpType::EXPIRE: {
        cpu_busy = false;
        unsigned int actual_burst_time = time - op.process->burst_start_time;
        unsigned int old_tau = op.process->tau;

        if (op.process->burst_index >= op.process->bursts.size() - 1) {
          printf("time %ums: Process %s terminated ", 
                  time, op.process->id.c_str());
          print_queue(ready_queue);
        } else {
          const size_t remaining = op.process->bursts.size() - op.process->burst_index - 1;
          printf("time %ums: Process %s (tau %ums) completed a CPU burst; %lu burst%s to go ", 
                  time, op.process->id.c_str(), old_tau, 
                  remaining, remaining == 1 ? "" : "s");
          print_queue(ready_queue);

          // Calculate new tau after printing
          op.process->tau = ceil(alpha * actual_burst_time + (1.0 - alpha) * old_tau);

          printf("time %ums: Recalculated tau for process %s: old tau %ums ==> new tau %ums ", 
                  time, op.process->id.c_str(), old_tau, op.process->tau);
          print_queue(ready_queue);

          unsigned int unblock_time = time + op.process->getIOTime() + (t_cs / 2);
          printf("time %ums: Process %s switching out of CPU; blocking on I/O until time %ums ", 
                  time, op.process->id.c_str(), unblock_time);
          print_queue(ready_queue);
          op_queue.emplace(OpType::IO_END, unblock_time, op.process);
        }

        op.process->current_burst_elapsed = 0;
        ++op.process->burst_index;
        time += t_cs / 2;

        break;
      }
      default: break;
    }
  }

  printf("time %ums: Simulator ended for SJF ", time);
  print_queue(ready_queue);
  printf("\n");

  // Calculate and output statistics
  unsigned int cpu_bound_burst_count = 0;
  unsigned int io_bound_burst_count = 0;
  unsigned int cpu_bound_total_burst_time = 0;
  unsigned int io_bound_total_burst_time = 0;

  for (const Process& process : processes) {
    for (const std::pair<unsigned int, unsigned int>& burst : process.bursts) {
      if (process.type) {
        io_bound_total_burst_time += burst.first;
        ++io_bound_burst_count;
      } else {
        cpu_bound_total_burst_time += burst.first;
        ++cpu_bound_burst_count;
      }
    }
  }

  cpu_bound_total_burst_time += t_cs * cpu_bound_cs_count;
  io_bound_total_burst_time += t_cs * io_bound_cs_count;

  const double util_percentage = (double)cpu_time / time * 100;
  double cpu_avg_wait = cpu_bound_burst_count ? (double)cpu_bound_total_wait_time / cpu_bound_burst_count : 0;
  double io_avg_wait = io_bound_burst_count ? (double)io_bound_total_wait_time / io_bound_burst_count : 0;
  double overall_avg_wait = (double)(cpu_bound_total_wait_time + io_bound_total_wait_time) / (cpu_bound_burst_count + io_bound_burst_count);
  double cpu_avg_turnaround = cpu_bound_burst_count ? (double)(cpu_bound_total_wait_time + cpu_bound_total_burst_time) / cpu_bound_burst_count : 0;
  double io_avg_turnaround = io_bound_burst_count ? (double)(io_bound_total_wait_time + io_bound_total_burst_time) / io_bound_burst_count : 0;
  double overall_avg_turnaround = (double)(cpu_bound_total_wait_time + cpu_bound_total_burst_time + io_bound_total_wait_time + io_bound_total_burst_time) / (cpu_bound_burst_count + io_bound_burst_count);

  fout << "Algorithm SJF" << std::endl;
  fout << std::fixed << std::setprecision(3);
  fout << "-- CPU utilization: " << ceil(util_percentage * 1000) / 1000 << "%" << std::endl;
  fout << "-- CPU-bound average wait time: " << ceil(cpu_avg_wait * 1000) / 1000 << " ms" << std::endl;
  fout << "-- I/O-bound average wait time: " << ceil(io_avg_wait * 1000) / 1000 << " ms" << std::endl;
  fout << "-- overall average wait time: " << ceil(overall_avg_wait * 1000) / 1000 << " ms" << std::endl;
  fout << "-- CPU-bound average turnaround time: " << ceil(cpu_avg_turnaround * 1000) / 1000 << " ms" << std::endl;
  fout << "-- I/O-bound average turnaround time: " << ceil(io_avg_turnaround * 1000) / 1000 << " ms" << std::endl;
  fout << "-- overall average turnaround time: " << ceil(overall_avg_turnaround * 1000) / 1000 << " ms" << std::endl;
  fout << "-- CPU-bound number of context switches: " << cpu_bound_cs_count << std::endl;
  fout << "-- I/O-bound number of context switches: " << io_bound_cs_count << std::endl;
  fout << "-- overall number of context switches: " << cpu_bound_cs_count + io_bound_cs_count << std::endl;
  fout << "-- CPU-bound number of preemptions: 0" << std::endl;
  fout << "-- I/O-bound number of preemptions: 0" << std::endl;
  fout << "-- overall number of preemptions: 0" << std::endl << std::endl;
}

void srt(std::vector<Process> processes, std::ofstream& fout, const double t_cs, const double alpha) {
  printf("time 0ms: Simulator started for SRT [Q empty]\n");

  unsigned int time = 0;
  unsigned int cpu_time = 0;
  unsigned int cpu_bound_total_wait_time = 0;
  unsigned int io_bound_total_wait_time = 0;
  unsigned int cpu_bound_cs_count = 0;
  unsigned int io_bound_cs_count = 0;
  unsigned int cpu_bound_preemptions = 0;
  unsigned int io_bound_preemptions = 0;

  std::sort(processes.begin(), processes.end(), [](Process& p1, Process& p2) {
    return p1.arrival_time == p2.arrival_time
      ? p1.id.compare(p2.id) < 0
      : p1.arrival_time < p2.arrival_time;
  });

  std::set<Process*, decltype(&sjf_srt_proc_sort)> ready_queue = std::set<Process*, decltype(&sjf_srt_proc_sort)>(&sjf_srt_proc_sort);
  std::set<Operation> op_queue;

  for (Process& process : processes) {
    op_queue.emplace(OpType::ARRIVE, process.arrival_time, &process);
  }

  std::set<Operation>::iterator active_process_expiry = op_queue.end();
  bool cpu_busy = false;
  Process* will_preempt_process = NULL;
  while (!op_queue.empty() || !ready_queue.empty() || cpu_busy) {
    if (!cpu_busy && !ready_queue.empty()) {
      cpu_busy = true;
      Process* proc = *ready_queue.begin(); // This will not actually be used
      op_queue.emplace(OpType::DEQUEUE, time, proc);
    }

    Operation op = *op_queue.begin();
    op_queue.erase(op_queue.begin());
    time = op.time;

    switch (op.type) {
      case OpType::CPU_UNBUSY:
        cpu_busy = false;

        break;
      case OpType::ARRIVE:
      case OpType::IO_END: {
        ready_queue.insert(op.process);
        op.process->ready_queue_add_time = time;

        std::set<Operation>::iterator next_burst = op_queue.begin();
        while (next_burst != op_queue.end() && next_burst->type != OpType::BURST) ++next_burst;
        if (next_burst != op_queue.end() && next_burst->time - time <= (t_cs / 2) && op.process->tau < next_burst->process->tau) will_preempt_process = op.process;

        if (active_process_expiry != op_queue.end() && (signed int)op.process->tau < ((signed int)active_process_expiry->process->tau - (signed int)(active_process_expiry->process->current_burst_elapsed + (time - active_process_expiry->process->burst_start_time))) && will_preempt_process == NULL) {
          Operation old_op = *active_process_expiry;
          op_queue.erase(active_process_expiry);
          active_process_expiry = op_queue.end();
          const unsigned int burst_time = time - old_op.process->burst_start_time;
          old_op.process->current_burst_elapsed += burst_time;

          cpu_time += burst_time;
          if (old_op.process->type) ++io_bound_preemptions;
          else ++cpu_bound_preemptions;


          printf("time %ums: Process %s (tau %ums) %s; preempting %s (predicted remaining time %ums) ",
            time,
            op.process->id.c_str(),
            op.process->tau,
            op.type == OpType::ARRIVE ? "arrived" : "completed I/O",
            old_op.process->id.c_str(),
            old_op.process->tau - old_op.process->current_burst_elapsed
          );
          print_queue(ready_queue);
          old_op.process->ready_queue_add_time = time + (t_cs / 2);
          ready_queue.insert(old_op.process);
          time += t_cs / 2;
          op_queue.emplace(OpType::CPU_UNBUSY, time, old_op.process);

          break;
        }

        if (op.type == OpType::ARRIVE) {
          printf("time %ums: Process %s (tau %ums) arrived; added to ready queue ", 
                  time, op.process->id.c_str(), op.process->tau);
        } else {
          printf("time %ums: Process %s (tau %ums) completed I/O; added to ready queue ", 
                  time, op.process->id.c_str(), op.process->tau);
        }
        print_queue(ready_queue);

        break;
      }
      case OpType::DEQUEUE: {
        cpu_busy = true;
        Process* dq_proc = *ready_queue.begin();
        ready_queue.erase(ready_queue.begin());
        op_queue.emplace(OpType::BURST, time + (t_cs / 2), dq_proc);

        if (dq_proc->type) io_bound_total_wait_time += time - dq_proc->ready_queue_add_time;
        else cpu_bound_total_wait_time += time - dq_proc->ready_queue_add_time;

        break;
      }
      case OpType::BURST: {
        unsigned int burst_time = op.process->getCPUTimeRemaining();
        if (op.process->current_burst_elapsed > 0) {
          printf("time %ums: Process %s (tau %ums) started using the CPU for remaining %ums of %ums burst ", 
                time, op.process->id.c_str(), op.process->tau, burst_time, op.process->getCurrentBurstTotalCPU());
        } else {
          printf("time %ums: Process %s (tau %ums) started using the CPU for %ums burst ", 
                  time, op.process->id.c_str(), op.process->tau, burst_time);
        }
        print_queue(ready_queue);

        if (will_preempt_process != NULL) {
          printf("time %ums: Process %s (tau %ums) will preempt %s ", time, will_preempt_process->id.c_str(), will_preempt_process->tau, op.process->id.c_str());
          print_queue(ready_queue);

          op.process->ready_queue_add_time = time + (t_cs / 2);
          ready_queue.insert(op.process);
          will_preempt_process = NULL;
          op_queue.emplace(OpType::CPU_UNBUSY, time + (t_cs / 2), op.process);

          if (op.process->type) {
            ++io_bound_preemptions;
            ++io_bound_cs_count;
          }
          else {
            ++cpu_bound_preemptions;
            ++cpu_bound_cs_count;
          }

          break;
        }

        op.process->burst_start_time = time;

        auto expiry_op = op_queue.emplace(OpType::EXPIRE, time + burst_time, op.process);
        active_process_expiry = expiry_op.first;

        if (op.process->type) ++io_bound_cs_count;
        else ++cpu_bound_cs_count;

        break;
      }
      case OpType::EXPIRE: {
        unsigned int actual_burst_time = time - op.process->burst_start_time;
        op.process->current_burst_elapsed += actual_burst_time;
        cpu_time += actual_burst_time;
        unsigned int old_tau = op.process->tau;
        active_process_expiry = op_queue.end();

        if (op.process->burst_index >= op.process->bursts.size() - 1) {
          printf("time %ums: Process %s terminated ", 
                  time, op.process->id.c_str());
          print_queue(ready_queue);
        } else {
          const size_t remaining = op.process->bursts.size() - op.process->burst_index - 1;
          printf("time %ums: Process %s (tau %ums) completed a CPU burst; %lu burst%s to go ", 
                  time, op.process->id.c_str(), old_tau, 
                  remaining, remaining == 1 ? "" : "s");
          print_queue(ready_queue);

          // Calculate new tau after printing
          op.process->tau = ceil(alpha * op.process->current_burst_elapsed + (1.0 - alpha) * old_tau);

          printf("time %ums: Recalculated tau for process %s: old tau %ums ==> new tau %ums ", 
                  time, op.process->id.c_str(), old_tau, op.process->tau);
          print_queue(ready_queue);

          unsigned int unblock_time = time + op.process->getIOTime() + (t_cs / 2);
          printf("time %ums: Process %s switching out of CPU; blocking on I/O until time %ums ", 
                  time, op.process->id.c_str(), unblock_time);
          print_queue(ready_queue);
          op_queue.emplace(OpType::IO_END, unblock_time, op.process);
        }

        op.process->current_burst_elapsed = 0;
        ++op.process->burst_index;
        time += t_cs / 2;
        op_queue.emplace(OpType::CPU_UNBUSY, time, op.process);

        break;
      }
      default: break;
    }
  }

  printf("time %ums: Simulator ended for SRT ", time);
  print_queue(ready_queue);
  printf("\n");

  // Calculate and output statistics
  unsigned int cpu_bound_burst_count = 0;
  unsigned int io_bound_burst_count = 0;
  unsigned int cpu_bound_total_burst_time = 0;
  unsigned int io_bound_total_burst_time = 0;

  for (const Process& process : processes) {
    for (const std::pair<unsigned int, unsigned int>& burst : process.bursts) {
      if (process.type) {
        io_bound_total_burst_time += burst.first;
        ++io_bound_burst_count;
      } else {
        cpu_bound_total_burst_time += burst.first;
        ++cpu_bound_burst_count;
      }
    }
  }

  cpu_bound_total_burst_time += t_cs * cpu_bound_cs_count;
  io_bound_total_burst_time += t_cs * io_bound_cs_count;

  const double util_percentage = (double)cpu_time / time * 100;
  double cpu_avg_wait = cpu_bound_burst_count ? (double)cpu_bound_total_wait_time / cpu_bound_burst_count : 0;
  double io_avg_wait = io_bound_burst_count ? (double)io_bound_total_wait_time / io_bound_burst_count : 0;
  double overall_avg_wait = (double)(cpu_bound_total_wait_time + io_bound_total_wait_time) / (cpu_bound_burst_count + io_bound_burst_count);
  double cpu_avg_turnaround = cpu_bound_burst_count ? (double)(cpu_bound_total_wait_time + cpu_bound_total_burst_time) / cpu_bound_burst_count : 0;
  double io_avg_turnaround = io_bound_burst_count ? (double)(io_bound_total_wait_time + io_bound_total_burst_time) / io_bound_burst_count : 0;
  double overall_avg_turnaround = (double)(cpu_bound_total_wait_time + cpu_bound_total_burst_time + io_bound_total_wait_time + io_bound_total_burst_time) / (cpu_bound_burst_count + io_bound_burst_count);

  fout << "Algorithm SRT" << std::endl;
  fout << std::fixed << std::setprecision(3);
  fout << "-- CPU utilization: " << ceil(util_percentage * 1000) / 1000 << "%" << std::endl;
  fout << "-- CPU-bound average wait time: " << ceil(cpu_avg_wait * 1000) / 1000 << " ms" << std::endl;
  fout << "-- I/O-bound average wait time: " << ceil(io_avg_wait * 1000) / 1000 << " ms" << std::endl;
  fout << "-- overall average wait time: " << ceil(overall_avg_wait * 1000) / 1000 << " ms" << std::endl;
  fout << "-- CPU-bound average turnaround time: " << ceil(cpu_avg_turnaround * 1000) / 1000 << " ms" << std::endl;
  fout << "-- I/O-bound average turnaround time: " << ceil(io_avg_turnaround * 1000) / 1000 << " ms" << std::endl;
  fout << "-- overall average turnaround time: " << ceil(overall_avg_turnaround * 1000) / 1000 << " ms" << std::endl;
  fout << "-- CPU-bound number of context switches: " << cpu_bound_cs_count << std::endl;
  fout << "-- I/O-bound number of context switches: " << io_bound_cs_count << std::endl;
  fout << "-- overall number of context switches: " << cpu_bound_cs_count + io_bound_cs_count << std::endl;
  fout << "-- CPU-bound number of preemptions: " << cpu_bound_preemptions << std::endl;
  fout << "-- I/O-bound number of preemptions: " << io_bound_preemptions << std::endl;
  fout << "-- overall number of preemptions: " << cpu_bound_preemptions + io_bound_preemptions << std::endl << std::endl;
}

void rr (std::vector<Process> processes, std::ofstream& fout, const double t_cs, const unsigned int t_slice) {
  printf("time 0ms: Simulator started for RR [Q empty]\n");

  unsigned int cpu_time = 0;

  unsigned int cpu_bound_total_wait_time = 0;
  unsigned int io_bound_total_wait_time = 0;

  unsigned int cpu_bound_cs_count = 0;
  unsigned int io_bound_cs_count = 0;
  unsigned int cpu_bound_preempt_count = 0;
  unsigned int io_bound_preempt_count = 0;

  unsigned int cpu_bound_single_slice_bursts = 0;
  unsigned int io_bound_single_slice_bursts = 0;

  // Sort by arrival time (descending)
  std::sort(processes.begin(), processes.end(), [](Process& p1, Process& p2) {
    return p1.arrival_time == p2.arrival_time
      ? p1.id.compare(p2.id) < 0
      : p1.arrival_time < p2.arrival_time;
  });

  unsigned int time = 0;
  std::list<Process*> ready_queue = std::list<Process*>();
  std::set<Operation> op_queue = std::set<Operation>();

  for (Process& process : processes) op_queue.emplace(OpType::ARRIVE, process.arrival_time, &process);

  bool cpu_busy = false;
  while (!op_queue.empty() || !ready_queue.empty()) {
    if (!cpu_busy && !ready_queue.empty()) {
      cpu_busy = true;
      op_queue.emplace(OpType::DEQUEUE, time, ready_queue.front());
    }

    const Operation op = *op_queue.begin();
    op_queue.erase(op_queue.begin());
    time = op.time;

    switch (op.type) {
      case OpType::ARRIVE: {
        printf("time %ums: Process %s arrived; added to ready queue ", time, op.process->id.c_str());
        ready_queue.push_back(op.process);
        print_queue(ready_queue);
        op.process->ready_queue_add_time = time;

        break;
      }
      case OpType::CPU_UNBUSY: {
        cpu_busy = false;

        break;
      }
      case OpType::ENQUEUE: {
        cpu_busy = false;
        ready_queue.push_back(op.process);
        op.process->ready_queue_add_time = time;

        break;
      }
      case OpType::DEQUEUE: {
        cpu_busy = true;
        ready_queue.pop_front();
        op_queue.emplace(OpType::BURST, time + (t_cs / 2), op.process);

        if (op.process->type) io_bound_total_wait_time += time - op.process->ready_queue_add_time;
        else cpu_bound_total_wait_time += time - op.process->ready_queue_add_time;

        break;
      }
      case OpType::CONTINUED_BURST:
      case OpType::BURST: {
        cpu_busy = true; // Needed for continued bursts
        unsigned int remaining = op.process->getCPUTimeRemaining();
        if (op.type != OpType::CONTINUED_BURST) {
          if (op.process->current_burst_elapsed) printf("time %ums: Process %s started using the CPU for remaining %ums of %ums burst ", time, op.process->id.c_str(), remaining, op.process->getCurrentBurstTotalCPU());
          else printf("time %ums: Process %s started using the CPU for %ums burst ", time, op.process->id.c_str(), op.process->getCurrentBurstTotalCPU());
          print_queue(ready_queue);
          op.process->burst_start_time = time;

          if (op.process->type) ++io_bound_cs_count;
          else ++cpu_bound_cs_count;
        }

        const unsigned int burst_time = remaining < t_slice ? remaining : t_slice;
        op_queue.emplace(OpType::EXPIRE, time + burst_time, op.process);
        op.process->current_burst_elapsed += burst_time;

        cpu_time += burst_time;

        break;
      }
      case OpType::EXPIRE: {
        cpu_busy = false;
        bool finished_burst = op.process->getCPUTimeRemaining() == 0;

        if (finished_burst) {
          if (op.process->getCurrentBurstTotalCPU() <= t_slice) {
            if (op.process->type) ++io_bound_single_slice_bursts;
            else ++cpu_bound_single_slice_bursts;
          }

          if (op.process->burst_index >= op.process->bursts.size() - 1) {
            printf("time %ums: Process %s terminated ", time, op.process->id.c_str());
            print_queue(ready_queue);
          } else {
            unsigned int unblock_time = time + op.process->getIOTime() + (t_cs / 2);

            op.process->current_burst_elapsed = 0;
            ++op.process->burst_index;
            const size_t remaining = op.process->bursts.size() - op.process->burst_index;
            printf("time %ums: Process %s completed a CPU burst; %lu burst%s to go ", time, op.process->id.c_str(), remaining, remaining == 1 ? "" : "s");
            print_queue(ready_queue);

            printf("time %ums: Process %s switching out of CPU; blocking on I/O until time %ums ", time, op.process->id.c_str(), unblock_time);
            print_queue(ready_queue);
            op_queue.emplace(OpType::IO_END, unblock_time, op.process);

            cpu_busy = true;
            op_queue.emplace(OpType::CPU_UNBUSY, time + (t_cs / 2), op.process);
          }
        } else if (ready_queue.size()) {
          cpu_busy = true;
          printf("time %ums: Time slice expired; preempting process %s with %ums remaining ", time, op.process->id.c_str(), op.process->getCPUTimeRemaining());
          print_queue(ready_queue);
          op_queue.emplace(OpType::ENQUEUE, time + (t_cs / 2), op.process);

          if (op.process->type) ++io_bound_preempt_count;
          else ++cpu_bound_preempt_count;
        } else {
          printf("time %ums: Time slice expired; no preemption because ready queue is empty ", time);
          print_queue(ready_queue);
          op_queue.emplace(OpType::CONTINUED_BURST, time, op.process);

          break;
        }

        time += t_cs / 2; // Needed for auto-dequeue

        break;
      }
      case OpType::IO_END: {
        ready_queue.push_back(op.process);
        op.process->ready_queue_add_time = time;
        printf("time %ums: Process %s completed I/O; added to ready queue ", time, op.process->id.c_str());
        print_queue(ready_queue);

        break;
      }
    }
  }

  printf("time %ums: Simulator ended for RR ", time);
  print_queue(ready_queue);

  unsigned int cpu_bound_burst_count = 0;
  unsigned int io_bound_burst_count = 0;

  unsigned int cpu_bound_total_burst_time = 0;
  unsigned int io_bound_total_burst_time = 0;
  for (const Process& process : processes) {
    for (const std::pair<unsigned int, unsigned int>& burst : process.bursts) {
      if (process.type) io_bound_total_burst_time += burst.first;
      else cpu_bound_total_burst_time += burst.first;
    }

    if (process.type) io_bound_burst_count += process.bursts.size();
    else cpu_bound_burst_count += process.bursts.size();
  }

  cpu_bound_total_burst_time += t_cs * cpu_bound_cs_count;
  io_bound_total_burst_time += t_cs * io_bound_cs_count;

  const double util_percentage = (double)cpu_time / time * 100;
  double cpu_avg_wait = (double)cpu_bound_total_wait_time / cpu_bound_burst_count;
  double io_avg_wait = (double)io_bound_total_wait_time / io_bound_burst_count;
  double overall_avg_wait = (double)(cpu_bound_total_wait_time + io_bound_total_wait_time) / (cpu_bound_burst_count + io_bound_burst_count);
  double cpu_avg_turnaround = (double)(cpu_bound_total_wait_time + cpu_bound_total_burst_time) / cpu_bound_burst_count;
  double io_avg_turnaround = (double)(io_bound_total_wait_time + io_bound_total_burst_time) / io_bound_burst_count;
  double overall_avg_turnaround = (double)(cpu_bound_total_wait_time + cpu_bound_total_burst_time + io_bound_total_wait_time + io_bound_total_burst_time) / (cpu_bound_burst_count + io_bound_burst_count);
  const double cpu_single_percentage = (double)cpu_bound_single_slice_bursts / cpu_bound_burst_count * 100;
  const double io_single_percentage = (double)io_bound_single_slice_bursts / io_bound_burst_count * 100;
  const double overall_single_percentage = (double)(cpu_bound_single_slice_bursts + io_bound_single_slice_bursts) / (cpu_bound_burst_count + io_bound_burst_count) * 100;

  if (isnan(cpu_avg_wait) || isinf(cpu_avg_wait)) cpu_avg_wait = 0;
  if (isnan(io_avg_wait) || isinf(io_avg_wait)) io_avg_wait = 0;
  if (isnan(overall_avg_wait) || isinf(overall_avg_wait)) overall_avg_wait = 0;
  if (isnan(cpu_avg_turnaround) || isinf(cpu_avg_turnaround)) cpu_avg_turnaround = 0;
  if (isnan(io_avg_turnaround) || isinf(io_avg_turnaround)) io_avg_turnaround = 0;
  if (isnan(overall_avg_turnaround) || isinf(overall_avg_turnaround)) overall_avg_turnaround = 0;

  fout << "Algorithm RR" << std::endl;
  fout << "-- CPU utilization: " << ceil(util_percentage * 1000) / 1000 << "%" << std::endl;
  fout << "-- CPU-bound average wait time: " << ceil(cpu_avg_wait * 1000) / 1000 << " ms" << std::endl;
  fout << "-- I/O-bound average wait time: " << ceil(io_avg_wait * 1000) / 1000 << " ms" << std::endl;
  fout << "-- overall average wait time: " << ceil(overall_avg_wait * 1000) / 1000 << " ms" << std::endl;
  fout << "-- CPU-bound average turnaround time: " << ceil(cpu_avg_turnaround * 1000) / 1000 << " ms" << std::endl;
  fout << "-- I/O-bound average turnaround time: " << ceil(io_avg_turnaround * 1000) / 1000 << " ms" << std::endl;
  fout << "-- overall average turnaround time: " << ceil(overall_avg_turnaround * 1000) / 1000 << " ms" << std::endl;
  fout << "-- CPU-bound number of context switches: " << cpu_bound_cs_count << std::endl;
  fout << "-- I/O-bound number of context switches: " << io_bound_cs_count << std::endl;
  fout << "-- overall number of context switches: " << cpu_bound_cs_count + io_bound_cs_count << std::endl;
  fout << "-- CPU-bound number of preemptions: " << cpu_bound_preempt_count << std::endl;
  fout << "-- I/O-bound number of preemptions: " << io_bound_preempt_count << std::endl;
  fout << "-- overall number of preemptions: " << cpu_bound_preempt_count + io_bound_preempt_count << std::endl;
  fout << "-- CPU-bound percentage of CPU bursts completed within one time slice: " << ceil(cpu_single_percentage * 1000) / 1000 << "%" << std::endl;
  fout << "-- I/O-bound percentage of CPU bursts completed within one time slice: " << ceil(io_single_percentage * 1000) / 1000 << "%" << std::endl;
  fout << "-- overall percentage of CPU bursts completed within one time slice: " << ceil(overall_single_percentage * 1000) / 1000 << "%" << std::endl;
}
