class SRT:
    def __init__(self, queue: list[process], context_switch, theta, alpha):
        self.queue = queue
        self.queue.sort(key=lambda x: x.pid)
        self.ready = []
        self.io_list = []
        self.waiting_io = None
        self.lengths = dict()
        self.current_process = None
        self.current_time = 0
        self.context_switch = context_switch
        self.context_switching = 0
        self.theta = theta
        self.alpha = alpha
        self.x = 0
        self.cpu_burst_start_time = 0
        self.cpu_use_time = 0 #How many ms the cpu has been in use for, used only in simout
        self.cpu_bound_context_switches = 0 #Simout only
        self.io_bound_context_switches = 0 #Simout only
        self.io_bound_wait_time_total = 0 #Simout only
        self.cpu_bound_wait_time_total = 0 #Simout only
        self.cpu_bound_turn_time_total = 0 #Simout only
        self.io_bound_turn_time_total = 0 #Simout only
        self.cpu_bound_preemptions = 0 #Simout only
        self.io_bound_preemptions = 0 #Simout only 
        self.cpu_bound_bursts = 0#Keeps track of how many cpu bound bursts there are
        self.io_bound_bursts = 0#Keeps track of how many i/o bound bursts there are
        self.queue_wait = []
        self.io_arrived = None
        self.time_context_switching = 0

    def place_process(self, process, tau):
        #places processes based on their tau value, if there is a process with a tau higher than the given one,
        #place the input one before it and return.
        if len(self.ready) == 0:
            self.ready.append(process)
            process.last_ready_time = self.current_time
            return
        #Time remaining: tau - preempt_time_remaining = time spent bursting, tau - time_spent_bursting = preempt_time_remaining
        for i in range(len(self.ready)):
            burst = self.lengths[self.ready[i]] - self.ready[i].preempt_remaining_time
            if self.lengths[self.ready[i]] == tau: #If we've found a process with the same tau as us, ok like, the chance of two tau's being the same, and the rem times being the same is small.
               if process.pid < self.ready[i].pid:#If the input process is lexographically ahead of the other process, place it first
                   self.ready.insert(i, process)
                   process.last_ready_time = self.current_time
                   return
            if self.ready[i].preempt_remaining_time != 0: 
                if process.preempt_remaining_time != 0 and self.ready[i].preempt_remaining_time > process.preempt_remaining_time:
                    self.ready.insert(i, process)
                    process.last_ready_time = self.current_time
                    return
                if self.ready[i].preempt_remaining_time > tau:
                    self.ready.insert(i, process)
                    process.last_ready_time = self.current_time
                    return
            elif process.preempt_remaining_time != 0:
                if self.lengths[self.ready[i]] > process.preempt_remaining_time:
                    self.ready.insert(i, process)
                    process.last_ready_time = self.current_time
                    return
            elif self.lengths[self.ready[i]] > tau:
                self.ready.insert(i, process)
                process.last_ready_time = self.current_time
                return
        self.ready.append(process)
        process.last_ready_time = self.current_time
        return

    def check_arrival(self):
        #ok so this just puts processes on the ready queue. 
        #It also calculates the tau for each process.
        #Because of this, it is only called when a process initally arrives in the simulation.
        p = 0
        while (p < len(self.queue)):
            if self.queue[p].arrival_time == self.current_time:
                tau_0 = math.ceil(1 / self.theta)
                self.lengths[self.queue[p]] = tau_0
                self.place_process(self.queue[p], tau_0)
                if(self.current_time < 10000):
                    print(f"time {self.current_time}ms: Process {self.queue[p].pid} (tau {tau_0}ms) arrived; added to ready queue {self.queue_print()}")
                self.queue.pop(p)
                #print(self.__str__())
                p = 0
            else:
                p +=1
    def cpu_burst(self):
        if self.current_process.cpu_bursts[0] > 0:
            self.current_process.cpu_bursts[0] -= 1
            #self.lengths[self.current_process] -= 1
            self.cpu_use_time += 1
            if self.current_process.cpu_bursts[0] == 0:
                if(self.current_process.is_cpu_bound):
                    self.cpu_bound_bursts += 1
                else:
                    self.io_bound_bursts += 1
                if(len(self.current_process.cpu_bursts) == 2):
                    if(self.current_time < 10000):
                        print(f"time {self.current_time}ms: Process {self.current_process.pid} (tau {self.lengths[self.current_process]}ms) completed a CPU burst; 1 burst to go {self.queue_print()}")
                elif(len(self.current_process.cpu_bursts) == 1):
                    s = "" #Idk what else to do here
                    #print(f"time {self.current_time}ms: Process {self.current_process.pid} completed a CPU burst; {len(self.current_process.cpu_bursts) - 1} bursts to go {self.queue_print()}")
                else:
                    if(self.current_time < 10000):
                        print(f"time {self.current_time}ms: Process {self.current_process.pid} (tau {self.lengths[self.current_process]}ms) completed a CPU burst; {len(self.current_process.cpu_bursts) - 1} bursts to go {self.queue_print()}")
                #print(self.__str__())
                self.context_switching = self.context_switch // 2
                self.current_process.cpu_bursts.pop(0)
                self.current_process.last_burst_time = 0
                self.current_process.preempt_remaining_time = 0
                if len(self.current_process.io_bursts) > 0:
                    block_until = self.current_time + self.context_switching + self.current_process.io_bursts[0]
                    tau_n = math.ceil(self.x*self.alpha + (1 - self.alpha)*self.lengths[self.current_process])
                    if(self.current_time < 10000):
                        print(f"time {self.current_time}ms: Recalculated tau for process {self.current_process.pid}: old tau {self.lengths[self.current_process]}ms ==> new tau {tau_n}ms {self.queue_print()}")
                    self.lengths[self.current_process] = tau_n
                    if(self.current_time < 10000):
                        print(f"time {self.current_time}ms: Process {self.current_process.pid} switching out of CPU; blocking on I/O until time {self.current_time + self.context_switching + self.current_process.io_bursts[0]}ms {self.queue_print()}")
                    self.io_place(self.current_process, block_until)
                    self.current_process = None
                    #print(self.__str__())
                else:
                    wait_sum = 0
                    if(self.current_process.is_cpu_bound):
                        self.cpu_bound_context_switches += self.current_process.context_switches
                        for num in self.current_process.waiting_time:
                            wait_sum += max(num -1, 0)
                        self.cpu_bound_wait_time_total += max(wait_sum - self.current_process.context_switches + len(self.current_process.static_cpu_bursts), 0)
                        self.cpu_bound_turn_time_total += (wait_sum + self.current_process.total_burst_time + self.context_switch * len(self.current_process.static_cpu_bursts))
                    else:
                        self.io_bound_context_switches += self.current_process.context_switches
                        for num in self.current_process.waiting_time:
                            wait_sum += max(num -1, 0)
                        self.io_bound_wait_time_total += max(wait_sum - self.current_process.context_switches + len(self.current_process.static_cpu_bursts), 0)
                        self.io_bound_turn_time_total += (wait_sum + self.current_process.total_burst_time + self.context_switch * len(self.current_process.static_cpu_bursts))
                    print(f"time {self.current_time}ms: Process {self.current_process.pid} terminated {self.queue_print()}")
                    self.current_process = None
                    #print(self.__str__())
    
    def io_place(self, proc, block): #Helper function to place processes on the i/o queue based on their name and i/o block end time
        for i in range(len(self.io_list)):
            process = self.io_list[i][0]
            block_until = self.io_list[i][1]
            if block < block_until: #If we're before another process
                self.io_list.insert(i, (proc, block))
                return
            if block == block_until: #If our process will unblock at the same time as another one
                if proc.pid < process.pid:
                    self.io_list.insert(i, (proc, block))
                    return
                self.io_list.insert(i+1, (proc, block))
                return
        self.io_list.append((proc, block)) #If we have not found a process that we go before or tie with, then just put us at the end

    def io_burst(self):
        for process, block_until in self.io_list:
            if self.current_time >= block_until:
                process.io_bursts.pop(0)
                rem_time = -1
                if(self.current_process != None):
                    rem_time = self.lengths[self.current_process] - (self.current_time + self.context_switch - self.cpu_burst_start_time)
                    if(self.current_process.preempt_remaining_time != 0):
                        rem_time = self.current_process.preempt_remaining_time - self.current_time +  self.cpu_burst_start_time
                if(self.current_process != None and self.lengths[process] < rem_time and rem_time != -1):
                    #switch process onto queue
                    #subtract the current proocess' tau by the amount of time it spent on the queue.
                    #self.lengths[self.current_process] -= (self.current_time - self.cpu_burst_start_time)
                    #preempt a process onto the queue by placing a new process onto the queue and setting the current one to 0.
                    self.place_process(process, self.lengths[process])
                    if(self.current_process.is_cpu_bound):
                        self.cpu_bound_preemptions += 1
                    else:
                        self.io_bound_preemptions += 1
                    if(self.current_process.preempt_remaining_time == 0):
                        if(self.current_time < 10000):
                            print(f"time {self.current_time + self.context_switching}ms: Process {process.pid} (tau {self.lengths[process]}ms) completed I/O; preempting {self.current_process.pid} (predicted remaining time {self.lengths[self.current_process] - (self.current_time - self.cpu_burst_start_time)}ms) {self.queue_print()}")
                        self.current_process.preempt_remaining_time = self.lengths[self.current_process] - (self.current_time - self.cpu_burst_start_time)
                    else:
                        if(self.current_time < 10000):
                            print(f"time {self.current_time + self.context_switching}ms: Process {process.pid} (tau {self.lengths[process]}ms) completed I/O; preempting {self.current_process.pid} (predicted remaining time {self.current_process.preempt_remaining_time - self.current_time + self.cpu_burst_start_time}ms) {self.queue_print()}")
                        self.current_process.preempt_remaining_time -= (self.current_time - self.cpu_burst_start_time)
                    #self.place_process(self.current_process, self.lengths[self.current_process])
                    heapq.heappush(self.queue_wait, (self.current_time + self.context_switch // 2, self.current_process))
                    #self.current_process.waiting_time[len(self.current_process.static_cpu_bursts) - len(self.current_process.cpu_bursts)] += (self.context_switch // 2)
                    self.current_process = None
                    self.context_switching += (self.context_switch // 2)
                else:
                    self.place_process(process, self.lengths[process])
                    if(self.current_time < 10000):
                        print(f"time {self.current_time + self.context_switching}ms: Process {process.pid} (tau {self.lengths[process]}ms) completed I/O; added to ready queue {self.queue_print()}")
                self.io_list.remove((process, block_until))
                #print(self.__str__())

    def run(self):
        print(f"time {self.current_time + self.context_switching}ms: Simulator started for SRT {self.queue_print()}")
        
        while len(self.queue) > 0 or len(self.ready) > 0 or self.current_process is not None or len(self.io_list) > 0 or self.context_switching > 0:
            if len(self.io_list) > 0:
                self.io_burst()
            self.check_arrival()
            self.current_time += 1
            if len(self.queue_wait) > 0:
                for i in range(len(self.queue_wait)):
                    if self.queue_wait[i][0] == self.current_time:
                        self.place_process(self.queue_wait[i][1], self.lengths[self.queue_wait[i][1]])
                        self.queue_wait.pop(i)
            #for i in range(len(self.ready)):
                #self.ready[i].waiting_time[len(self.ready[i].static_cpu_bursts) - len(self.ready[i].cpu_bursts)] += 1
            if self.context_switching > 0:
                self.context_switching -= 1
                self.time_context_switching += 1
            elif self.current_process is None and len(self.ready) > 0:
                self.current_process = self.ready.pop(0)
                self.context_switching = max((self.context_switch // 2) - 1, 0)
                if(self.current_process.last_burst_time != 0 and self.current_process.last_burst_time != self.current_process.cpu_bursts[0]):
                    if(self.current_time < 10000):
                        print(f"time {self.current_time + self.context_switching}ms: Process {self.current_process.pid} (tau {self.lengths[self.current_process]}ms) started using the CPU for remaining {self.current_process.cpu_bursts[0]}ms of {self.current_process.last_burst_time}ms burst {self.queue_print()}")
                else:
                    if(self.current_time < 10000):
                        print(f"time {self.current_time + self.context_switching}ms: Process {self.current_process.pid} (tau {self.lengths[self.current_process]}ms) started using the CPU for {self.current_process.cpu_bursts[0]}ms burst {self.queue_print()}")
                    self.current_process.last_burst_time = self.current_process.cpu_bursts[0]
                self.cpu_burst_start_time = self.current_time + self.context_switching
                self.current_process.context_switches += 1
                self.current_process.waiting_time[len(self.current_process.static_cpu_bursts) - len(self.current_process.cpu_bursts)] += (self.current_time - self.current_process.last_ready_time)
                self.x = self.current_process.cpu_bursts[0]
                if(self.current_process.last_burst_time != 0):
                    self.x = self.current_process.last_burst_time
                #print(self.__str__())
            elif self.current_process is not None:
                self.cpu_burst()
            else: #TODO: Remove
                inserted = self.check_arrival()
                if (inserted != None and self.current_process != None and self.lengths[inserted] < self.lengths[self.current_process]):
                    #preempt a process onto the queue (this may never happen??)
                    self.place_process(self.current_process, self.lengths[self.current_process])
                    print(f"time {self.current_time + self.context_switching}ms: Process {self.current_process.pid} preempted by Process {process.pid} {self.queue_print()}")
                    self.queue.remove(inserted)
                    self.current_process = inserted
                    self.context_switching =  max((self.context_switch // 2) - 1, 0)
                    self.x = self.current_process.cpu_bursts[0]
                
                
            if self.current_time % 500 == 0:
                #print(f"time {self.current_time}ms: State: {self.__str__()}")
                pass
            
        print(f"time {self.current_time + self.context_switching}ms: Simulator ended for SRT {self.queue_print()}")
            
    def queue_print(self):
        return f"[Q {' '.join(p.pid for p in self.ready) if self.ready else 'empty'}]"

    def __str__(self) -> str:
        s = "<<< CURRENT STATE\n"
        s += f"[{' '.join(p.pid for p in self.queue)}]\n"
        s += f"[{' '.join(p.pid for p in self.ready)}]\n"
        s += f"[{' '.join(f'{p.pid} (until {t}ms)' for p, t in self.io_list)}]\n"
        s += f"{self.current_process.pid if self.current_process else 'None'}"
        return s
