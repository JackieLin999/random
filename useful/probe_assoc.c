#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#define MAX_WAYS 32 // max ways to test is 32 ways
#define ACCESSES 1000000

int main(int argc, char *argv[]) {
    // read the stride or the input
    if (argc != 2) {
        printf("Usage: ./probe_assoc <stride_in_bytes>\n");
        return 1;
    }

    int STRIDE = atoi(argv[1]);
    
    if (STRIDE <= 0) {
        printf("Invalid stride.\n");
        return 1;
    }

    // allocate memory for testing
    size_t mem_size = (size_t)MAX_WAYS * STRIDE;
    char *memory = (char*)malloc(mem_size);
    if (!memory) {
        perror("Malloc failed");
        return 1;
    }
    
    // write to the entire space
    memset(memory, 1, mem_size);

    printf("Probing Associativity with Stride = %d bytes\n", STRIDE);
    printf("Ways\tAvg_Time(ns)\n");
    printf("--------------------\n");

    // looping throuhg each way
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