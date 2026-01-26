#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>



// We test up to 32 ways. 
// (L1 is usually 8 or 12, L2 is 10-16, L3 is 12-16, so 32 is safe)
#define MAX_WAYS 32 
#define ACCESSES 1000000 // Enough loops to get a stable average

void pin_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("sched_setaffinity");
        exit(1);
    }
    printf("Successfully pinned to Core %d\n", core_id);
}

int main(int argc, char *argv[]) {
    // 1. Read the Stride from the command line
    pin_to_core(0);
    if (argc != 2) {
        printf("Usage: ./probe_assoc <stride_in_bytes>\n");
        return 1;
    }

    int STRIDE = atoi(argv[1]);
    
    if (STRIDE <= 0) {
        printf("Invalid stride.\n");
        return 1;
    }

    // 2. Allocate memory: Enough for the maximum number of ways we want to test
    size_t mem_size = (size_t)MAX_WAYS * STRIDE;
    char *memory = (char*)malloc(mem_size);
    if (!memory) {
        perror("Malloc failed");
        return 1;
    }
    
    // Warmup: Write to memory to ensure pages are physically allocated
    memset(memory, 1, mem_size);

    printf("Probing Associativity with Stride = %d bytes\n", STRIDE);
    printf("Ways\tAvg_Time(ns)\n");
    printf("--------------------\n");

    // 3. The Main Experiment Loop
    // We try chains of length 1, then 2, then 3... up to MAX_WAYS
    for (int ways = 1; ways <= MAX_WAYS; ways++) {
        
        // --- Setup the Pointer Chain ---
        // We link 'ways' number of pointers, each STRIDE bytes apart.
        // This forces them all to map to the same Cache Set Index.
        for (int k = 0; k < ways; k++) {
            void **current = (void**)(&memory[k * STRIDE]);
            void **next;
            
            if (k == ways - 1) {
                next = (void**)(&memory[0]); // Loop back to start (Circular)
            } else {
                next = (void**)(&memory[(k + 1) * STRIDE]);
            }
            *current = next;
        }

        // --- Measure Latency ---
        void **p = (void**)(&memory[0]);
        struct timespec start, end;
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        // Critical Loop: Pointer Chasing
        for (int i = 0; i < ACCESSES; i++) {
            p = (void**)*p; 
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);

        double total_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        printf("%d\t%.4f\n", ways, total_ns / ACCESSES);
    }
    
    free(memory);
    return 0;
}