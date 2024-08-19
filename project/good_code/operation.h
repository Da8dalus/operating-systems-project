#ifndef _operation_h
#define _operation_h
#include "process.h"

// NOTE: This is ordered according to priority spec'd in PDF
enum OpType {
  EXPIRE,
  CONTINUED_BURST,
  BURST,
  ENQUEUE,
  IO_END,
  CPU_UNBUSY,
  ARRIVE,
  DEQUEUE
};

class Operation {
public:
  OpType type;
  unsigned int time;
  Process* process;

  Operation (OpType type, unsigned int time, Process* process) {
    this->type = type;
    this->time = time;
    this->process = process;
  }

  bool operator< (const Operation& op2) const {
    return this->time == op2.time
      ? this->type == op2.type
        ? this->process->id.compare(op2.process->id) < 0
        : this->type < op2.type
      : this->time < op2.time;
  }
};

#endif