#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

// Use a large array size (e.g., 64MB) to ensure we are working effectively
#define ARRAY_SIZE (64 * 1024 * 1024)

int main() {
    // 1. Allocate a large chunk of memory
    // Using int8_t (bytes) for precise stride control
    int8_t *array = (int8_t*)malloc(ARRAY_SIZE * sizeof(int8_t));
    if (!array) {
        perror("Malloc failed");
        return 1;
    }

    // Initialize to prevent page faults during the timing loop
    for (int i = 0; i < ARRAY_SIZE; i++) {
        array[i] = 1;
    }

    struct timespec start, end;
    long long steps;
    int dummy = 0; // To prevent compiler optimization

    printf("Stride(B)\tAvg_Time(ns)\n");
    printf("------------------------\n");

    // 2. Loop through different strides (powers of 2)
    // From 1 byte up to 512 bytes
    for (int stride = 1; stride <= 512; stride *= 2) {
        
        steps = 0;
        
        // Start Timer
        clock_gettime(CLOCK_MONOTONIC, &start);

        // THE CRITICAL LOOP
        // We iterate the array with the current 'stride'
        // We mask with (ARRAY_SIZE - 1) to ensure we wrap around if needed (requires power of 2 size),
        // or just stop before the end. Here, standard loop is safer for clarity:
        for (int i = 0; i < ARRAY_SIZE; i += stride) {
            dummy += array[i]; // Read access
            steps++;
        }

        // Stop Timer
        clock_gettime(CLOCK_MONOTONIC, &end);

        // Calculate time diff in nanoseconds
        double time_taken = (end.tv_sec - start.tv_sec) * 1e9 + 
                            (end.tv_nsec - start.tv_nsec);
        
        printf("%d\t\t%.4f\n", stride, time_taken / steps);
    }

    free(array);
    return 0;
}