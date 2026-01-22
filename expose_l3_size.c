#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

// Search deeper to find the DRAM plateau
#define MIN_SIZE_KB 1024       // Start at 1MB
#define MAX_SIZE_KB (128*1024) // Go up to 128MB

int main() {
    printf("Array_Size(KB)\tLatency(ns)\n");
    printf("---------------------------\n");

    // Seed random number generator
    srand(time(NULL));

    for (int size_kb = MIN_SIZE_KB; size_kb <= MAX_SIZE_KB; size_kb *= 2) {
        
        size_t size_bytes = size_kb * 1024;
        size_t num_pointers = size_bytes / sizeof(void*);
        
        // 1. Allocate buffer
        void **memory = (void**)malloc(size_bytes);
        if (!memory) break;
        
        // 2. Allocate temporary index array for shuffling
        // We use this to decide the random order of linking
        int *indices = (int*)malloc(num_pointers * sizeof(int));
        if (!indices) { free(memory); break; }

        for (int i = 0; i < num_pointers; i++) indices[i] = i;

        // 3. Fisher-Yates Shuffle (Randomize the indices)
        // This is the key step to break the prefetcher!
        for (int i = num_pointers - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int temp = indices[i];
            indices[i] = indices[j];
            indices[j] = temp;
        }

        // 4. Link the pointers based on the random indices
        for (int i = 0; i < num_pointers - 1; i++) {
            // memory[current_random_index] points to memory[next_random_index]
            memory[indices[i]] = (void*)&memory[indices[i+1]];
        }
        // Loop back to start to close the circle
        memory[indices[num_pointers - 1]] = (void*)&memory[indices[0]];

        // 5. Run the Timing Test
        int iterations = 10000000; 
        void **p = (void*)&memory[indices[0]];
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int k = 0; k < iterations; k++) {
            p = (void**)*p;
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        
        double total_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        printf("%d\t\t%.4f\n", size_kb, total_ns / iterations);

        free(indices);
        free(memory);
    }
    return 0;
}