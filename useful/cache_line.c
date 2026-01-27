// cache_line.c
// for finding the cache_line
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

// we will create a large region of memory. 64 MB is bigger and all cache combined
#define ARRAY_SIZE_BYTES (64 * 1024 * 1024) // 64 MB
#define NUM_ACCESSES 10000000 // Number of hops to measure

int main() {
    // allocate memory
    char *memory = (char*)malloc(ARRAY_SIZE_BYTES);
    if (!memory) return 1;

    // Fill with junk to force physical allocation
    memset(memory, 1, ARRAY_SIZE_BYTES);

    // use void** to treat the memory as a chain of pointers
    void **head;
    
    printf("Stride(B)\tAvg_Time(ns)\n");
    printf("------------------------\n");

    // Test strides from 16 up to 512
    for (int stride = 16; stride <= 512; stride *= 2) {
        
        // manually link memory[i] to point to memory[i+stride]
        for (int i = 0; i < ARRAY_SIZE_BYTES - stride; i += stride) {
            // The value at 'memory + i' is the address of 'memory + i + stride'
            *(void**)(&memory[i]) = (void*)(&memory[i + stride]);
        }

        // Close the loop
        *(void**)(&memory[ARRAY_SIZE_BYTES - stride]) = (void*)(&memory[0]);

        head = (void**)memory; // Start at beginning
        
        // Warm up cache slightly
        void **p = head;
        for(int k=0; k<1000; k++) p = (void**)*p;

        // measure
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        // The CPU cannot predict 'p' until it reads '*p'.
        // This forces serialization and exposes true latency.
        p = head;
        for (int k = 0; k < NUM_ACCESSES; k++) {
            p = (void**)*p; 
        }

        clock_gettime(CLOCK_MONOTONIC, &end);

        // Calculate
        double time_ns = ((end.tv_sec - start.tv_sec) * 1e9 + 
                          (end.tv_nsec - start.tv_nsec)) / NUM_ACCESSES;

        printf("%d\t\t%.4f\n", stride, time_ns);
    }

    free(memory);
    return 0;
}