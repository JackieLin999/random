#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>

// Simple function to get current time in nanoseconds
uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// core probing function
double measure_access_time(size_t size_bytes) {
    // Number of pointers we can fit in this size
    size_t num_elements = size_bytes / sizeof(void*);
    
    // Allocate the buffer
    void **buffer = malloc(size_bytes);
    if (!buffer) {
        printf("Failed to allocate %zu bytes\n", size_bytes);
        return -1;
    }

    // 1. Create a temporary array of indices: 0, 1, 2, ...
    size_t *indices = malloc(num_elements * sizeof(size_t));
    if (!indices) { free(buffer); return -1; }
    
    for (size_t i = 0; i < num_elements; i++) indices[i] = i;

    // 2. Fisher-Yates Shuffle
    // This randomizes the order so the CPU prefetcher cannot cheat.
    // It ensures we visit every single cache line randomly.
    for (size_t i = num_elements - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        size_t temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }

    // 3. Link the buffer based on the shuffled indices
    // buffer[ random_index_A ] points to buffer[ random_index_B ]
    for (size_t i = 0; i < num_elements - 1; i++) {
        buffer[indices[i]] = (void*)&buffer[indices[i+1]];
    }
    // Link the last element back to the start to close the circle
    buffer[indices[num_elements - 1]] = (void*)&buffer[indices[0]];

    free(indices); // We don't need the index array anymore

    // 4. Measurement Loop
    void **ptr = buffer;
    
    // Warmup (ensure data is in cache if it fits)
    // We chase the pointers for a bit
    int warmup_steps = 1000000;
    for (int i = 0; i < warmup_steps; i++) {
        ptr = (void**)*ptr;
    }

    // Actual Test
    int iterations = 10000000; // 10 million accesses
    uint64_t start = get_time_ns();

    // The critical loop
    for (int i = 0; i < iterations; i++) {
        ptr = (void**)*ptr;
    }

    uint64_t end = get_time_ns();

    free(buffer);

    return (double)(end - start) / iterations;
}

int main() {
    srand(time(NULL)); // Seed the random number generator
    
    printf("Size(KB), Time(ns)\n");

    // Test from 4KB up to 64MB (64 * 1024 * 1024)
    // This ensures we cover L1, L2, L3, and hit RAM.
    for (size_t size = 4 * 1024; size <= 64 * 1024 * 1024; size *= 2) {
        double time = measure_access_time(size);
        printf("%zu, %.2f\n", size / 1024, time);
    }

    return 0;
}