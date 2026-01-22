#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

// We will test sizes from 1KB up to 64MB
#define MIN_SIZE_KB 1
#define MAX_SIZE_KB 65536 

int main() {
    printf("Array_Size(KB)\tLatency(ns)\n");
    printf("---------------------------\n");

    // Loop through sizes: 1KB, 2KB, 4KB... up to 64MB
    // We also test "intermediate" sizes (like 1.5x) to catch boundaries better
    for (int size_kb = MIN_SIZE_KB; size_kb <= MAX_SIZE_KB; size_kb *= 2) {
        
        size_t size_bytes = size_kb * 1024;
        
        // 1. Allocate memory
        void **memory = (void**)malloc(size_bytes);
        if (!memory) break;
        
        // Fill with junk/warmup
        memset(memory, 1, size_bytes);

        // 2. Setup Pointer Chain (Stride = 64 bytes)
        // We link every 64th byte to the next one. 
        // We use 64 because we already proved that is the Line Size.
        int stride = 64; 
        int num_links = size_bytes / stride;
        
        for (int i = 0; i < num_links - 1; i++) {
            memory[i * (stride/sizeof(void*))] = (void*)&memory[(i + 1) * (stride/sizeof(void*))];
        }
        // Circular loop back to start
        memory[(num_links - 1) * (stride/sizeof(void*))] = (void*)&memory[0];

        // 3. Run the test
        int iterations = 10000000; // Total accesses
        
        // Adjust iterations for small arrays so we don't finish too instantly
        // (Optional, but helps stability)
        
        void **p = (void**)memory;
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        // The Critical Loop
        for (int k = 0; k < iterations; k++) {
            p = (void**)*p;
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        
        // Calculate Time
        double total_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        printf("%d\t\t%.4f\n", size_kb, total_ns / iterations);

        free(memory);
    }
    return 0;
}