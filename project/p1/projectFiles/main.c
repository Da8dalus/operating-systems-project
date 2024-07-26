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


double next_exp(double lambda, double ceiling){
	double next = ceiling * 100;
	while(ceil(next) > ceiling){
		next = -log(drand48()) / lambda;
	}

	return next;
}


void createProcess(Process * p, char* id, bool bound, int numBursts, double lambda, double ceiling, int arrivalTime){
	p->id = strdup(id);
	//printf("%s\n", p->id);
	p->numBursts = numBursts;
	p->cpu_io_bursts = (int **)calloc(numBursts, sizeof(int *));
	p->cpu_bound = bound;
	p->arrival = arrivalTime;
	p->lambda = lambda;
	p->ceiling = ceiling;

	for(int b = 0; b < numBursts; b++){
		int cpuTime = 0;
		int ioTime = 0;

		*(p->cpu_io_bursts + b) = (int *)calloc(2, sizeof(int));
		cpuTime = ceil(next_exp(lambda, ceiling));

		if(b < numBursts - 1){
			ioTime = ceil(next_exp(lambda, ceiling));
			if(p->cpu_bound == false){
				ioTime = ioTime * 8;
			}
		}

		if(p->cpu_bound == true){
			cpuTime = cpuTime * 4;
		}

		*(*(p->cpu_io_bursts + b) + 0) = cpuTime;
		*(*(p->cpu_io_bursts + b) + 1) = ioTime;

	}

}

double round_up(double value, int decimal_places) {
    double factor = pow(10, decimal_places);
    return ceil(value * factor) / factor;
}

int main(int argc, char **argv) {
    if (argc != 6) {
        fprintf(stderr, "Invalid amount of arguments\n");
        abort();
    }

    int n_processes = atoi(*(argv + 1));
    if (n_processes < 0) {
        fprintf(stderr, "Invalid n processes\n");
        abort();
    }

    int n_cpu = atoi(*(argv + 2));
    if (n_cpu < 0 || n_cpu > n_processes) {
        fprintf(stderr, "Invalid amount of CPU-bound processes\n");
        abort();
    }

    int seed = atoi(*(argv + 3));

    double lambda = atof(*(argv + 4));

    int ceiling = atoi(*(argv + 5));

    if (ceiling == 0) {
        perror("Upper Bound(ceiling) way too low");
        abort();
    }

    Process *processes = (Process *)calloc(n_processes, sizeof(Process));
    srand48(seed);

    char *id = calloc(3, sizeof(char));  // Adjusted size to 3 to ensure space for the null terminator

    char *LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char *DIGITS = "0123456789";

    for (int n = 0; n < n_processes; n++) {
        *(id + 1) = *(DIGITS + n % 10);
        *(id + 0) = *(LETTERS + (n - n % 10) / 10);
        *(id + 2) = '\0';  // Ensure the id string is null-terminated

        int arrivalTime = floor(next_exp(lambda, ceiling));
        int bursts = ceil(drand48() * 32);

        bool isCPUBound = n < n_cpu;

        createProcess((processes + n), id, isCPUBound, bursts, lambda, ceiling, arrivalTime);
    }

    // Printing output and calculating averages
    FILE *output = fopen("simout.txt", "w");
    if (output == NULL) {
        perror("Error opening file");
        abort();
    }

    double cpu_sum = 0;
	double io_sum = 0;
    double cpu_io_sum = 0; 
	double io_io_sum = 0;
    int cpu_burst_count = 0;
	int io_burst_count = 0;
    int cpu_io_burst_count = 0; 
	int io_io_burst_count = 0;

    printf("<<< PROJECT PART I\n");
    if (n_cpu != 1) {
        printf("<<< -- process set (n=%d) with %d CPU-bound processes\n", n_processes, n_cpu);
    } else {
        printf("<<< -- process set (n=%d) with %d CPU-bound process\n", n_processes, n_cpu);
    }
    printf("<<< -- seed=%d; lambda=%f; bound=%d\n", seed, lambda, ceiling);

    for (int n = 0; n < n_processes; n++) {
        Process p = *(processes + n);

        if (p.cpu_bound) {
			if(p.numBursts > 1){
            printf("CPU-bound process %s: arrival time %dms; %d CPU bursts:\n", p.id, p.arrival, p.numBursts);
			}else{
				printf("CPU-bound process %s: arrival time %dms; %d CPU burst:\n", p.id, p.arrival, p.numBursts);
			}
        } else {
			if(p.numBursts > 1){
            printf("I/O-bound process %s: arrival time %dms; %d CPU bursts:\n", p.id, p.arrival, p.numBursts);
			}else{
				printf("I/O-bound process %s: arrival time %dms; %d CPU burst:\n", p.id, p.arrival, p.numBursts);
			}
        }

        for (int b = 0; b < p.numBursts; b++) {
            printf("==> CPU burst %dms", *(*(p.cpu_io_bursts + b) + 0));
            if (b < p.numBursts - 1) {
                printf(" ==> I/O burst %dms", *(*(p.cpu_io_bursts + b) + 1));
                if (p.cpu_bound) {
                    cpu_io_sum += *(*(p.cpu_io_bursts + b) + 1);
                    cpu_io_burst_count++;
                } else {
                    io_io_sum += *(*(p.cpu_io_bursts + b) + 1);
                    io_io_burst_count++;
                }
            }
            printf("\n");

            if (p.cpu_bound) {
                cpu_sum += *(*(p.cpu_io_bursts + b) + 0);
                cpu_burst_count++;
            } else {
                io_sum += *(*(p.cpu_io_bursts + b) + 0);
                io_burst_count++;
            }
        }
    }


    // Calculate averages
    double cpu_avg = cpu_sum / cpu_burst_count;
    double io_avg = io_sum / io_burst_count;
    double overall_avg = (cpu_sum + io_sum) / (cpu_burst_count + io_burst_count);
	
	double cpu_avg_io = cpu_io_sum / cpu_io_burst_count;
    double io_avg_io = io_io_sum / io_io_burst_count;
    double overall_avg_io = (cpu_io_sum + io_io_sum) / (cpu_io_burst_count + io_io_burst_count);
	
	if(cpu_burst_count == 0){
		cpu_avg = 0.000;
	}
	if(io_burst_count == 0){
		io_avg = 0.000;
	}
	if(cpu_io_burst_count == 0){
		cpu_avg_io = 0.000;
	}
	if(io_io_burst_count == 0){
		io_avg_io = 0.000;
	}
	if(cpu_burst_count + io_burst_count == 0){
		overall_avg = 0.000;
	}
	if(cpu_io_burst_count + io_io_burst_count == 0){
		overall_avg_io = 0.000;
	}
	
		
	


    // Write summary to simout.txt
    fprintf(output, "-- number of processes: %d\n", n_processes);
    fprintf(output, "-- number of CPU-bound processes: %d\n", n_cpu);
    fprintf(output, "-- number of I/O-bound processes: %d\n", n_processes - n_cpu);
    fprintf(output, "-- CPU-bound average CPU burst time: %.3f ms\n", round_up(cpu_avg, 3));
    fprintf(output, "-- I/O-bound average CPU burst time: %.3f ms\n", round_up(io_avg, 3));
    fprintf(output, "-- overall average CPU burst time: %.3f ms\n", round_up(overall_avg, 3));
    fprintf(output, "-- CPU-bound average I/O burst time: %.3f ms\n", round_up(cpu_avg_io, 3));
    fprintf(output, "-- I/O-bound average I/O burst time: %.3f ms\n", round_up(io_avg_io, 3));
    fprintf(output, "-- overall average I/O burst time: %.3f ms\n", round_up(overall_avg_io, 3));

    fclose(output);
}