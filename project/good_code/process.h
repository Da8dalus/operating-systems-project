#ifndef _process
#define _process

#include <string>
#include <vector>
#include <utility>
#include <iostream>

#include "next_exp.h"

class Process {

public:
  // FIELDS
  std::string id;
  // cpu time, io time
  std::vector<std::pair<unsigned int, unsigned int>> bursts;
  bool type; // true for I/O, false for CPU
  unsigned int arrival_time;
  // The current burst index (used for algos)
  size_t burst_index;
  // The time elapsed in the active burst (used for algos)
  unsigned int current_burst_elapsed;
  // The time the process was added to the ready queue (used for algos)
  unsigned int ready_queue_add_time;
  // The time the current burst was started (used for algos)
  unsigned int burst_start_time;
  // The value used by SJF and SRT (used for algos)
  unsigned int tau;
  // Amount a process is preempted by (used for SRT)
  unsigned int total_preempted;
  // Time to display for SRT
  int remaining_time;

  // CONSTRUCTOR
  Process (std::string id, bool type, unsigned int arrival_time, unsigned int bursts, double lambda, double ceiling) {
    this->id = id;
    this->type = type;
    this->arrival_time = arrival_time;
    this->burst_index = 0;
    this->current_burst_elapsed = 0;
    this->ready_queue_add_time = 0;
    this->tau = ceil(1.0 / (lambda));
    this->total_preempted = 0;
    this->remaining_time = 0;

    this->bursts = std::vector<std::pair<unsigned int, unsigned int>>(bursts);
    for (unsigned int b = 0; b < bursts; ++b) {
      int cpu_time = ceil(next_exp(lambda, ceiling));
      // For CPU-bound, don't generate final I/O burst
      int io_time = b >= bursts - 1
        ? 0
        : ceil(next_exp(lambda, ceiling));


      if (type) io_time *= 8;
      else cpu_time *= 4;

      this->bursts[b] = std::make_pair(cpu_time, io_time);
    }
  }

  // PRINT METHOD
  void print (bool verbose = false) {
    std::cout << (type ? "I/O" : "CPU") << "-bound process " << id << ": arrival time " << arrival_time << "ms; " << bursts.size() << (bursts.size() == 1 ? " CPU burst" : " CPU bursts") << (verbose ? ":" : "") << std::endl;

    if (verbose) {
      for (size_t b = 0; b < bursts.size(); ++ b) {
        std::pair<unsigned int, unsigned int> burst = bursts[b];

        std::cout << "==> CPU burst " << burst.first << "ms";
        if (b < bursts.size() - 1) std::cout << " ==> I/O burst " << burst.second << "ms";

        std::cout << std::endl;
      }
    }
  }

  // BURST TIME ACCESSOR
  std::pair<unsigned int, unsigned int> getBurstTime () {
    std::pair<unsigned int, unsigned int> total;
    for(size_t i = 0; i < bursts.size(); i++){
      total.first += bursts[i].first;
      if (i < bursts.size() - 1) total.second += bursts[i].second;
    }
    return total;
  }

  unsigned int getNumBursts () {
    return bursts.size();
  }

  unsigned int getCurrentBurstTotalCPU () {
    return bursts[burst_index].first;
  }

  unsigned int getCPUTimeRemaining () {
    return bursts[burst_index].first - current_burst_elapsed;
  }

  unsigned int getIOTime () {
    return bursts[burst_index].second;
  }
};

bool sjf_srt_proc_sort (const Process* p1, const Process* p2) {
  return p1->tau == p2->tau
    ? p1->id.compare(p2->id) < 0
    : p1->tau - p1->current_burst_elapsed < p2->tau - p2->current_burst_elapsed;
}

#endif
