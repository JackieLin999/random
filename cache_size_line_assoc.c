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

// The core probing function
// size_bytes: Total size of the buffer to test
// stride: How far apart elements are (to test line size)
double measure_access_time(size_t size_bytes, int stride) {
    // Number of elements we can fit
    size_t num_elements = size_bytes / sizeof(void*);
    void **buffer = malloc(size_bytes);
    
    if (!buffer) return -1;

    // Initialize pointer chase
    // We link buffer[i] to buffer[i+stride] to confuse the CPU slightly
    // or create a circular list. 
    for (size_t i = 0; i < num_elements; i++) {
        // Link to the next element based on stride, wrapping around
        size_t next_index = (i + stride) % num_elements;
        buffer[i] = (void*)&buffer[next_index];
    }

    // Warmup: Run through the list once to ensure it's in cache (if it fits)
    void **ptr = buffer;
    for (size_t i = 0; i < num_elements; i++) {
        ptr = (void**)*ptr;
    }

    // Measurement Loop
    // We iterate many times to get a stable average
    int iterations = 10000000; 
    uint64_t start = get_time_ns();
    
    ptr = buffer;
    // Unrolling loop slightly for less overhead
    for (int i = 0; i < iterations; i++) {
        ptr = (void**)*ptr;
    }

    uint64_t end = get_time_ns();

    free(buffer);
    
    // Calculate average time per access in nanoseconds
    return (double)(end - start) / iterations;
}

int main() {
    printf("Size(KB), Stride(B), Time(ns)\n");
    
    // TEST 1: Find L1 Cache Size
    // Fix stride to 64 bytes (likely line size), vary array size
    // Start small (4KB) and go to large (4MB)
    int stride = 64; 
    for (size_t size = 4 * 1024; size <= 4 * 1024 * 1024; size *= 2) {
        double time = measure_access_time(size, stride / sizeof(void*));
        printf("%zu, %d, %.2f\n", size / 1024, stride, time);
    }
    
    return 0;
}