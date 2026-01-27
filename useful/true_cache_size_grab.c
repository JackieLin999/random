// true_cache_size_grab.c
// increasing the buffer and check latency. This is for probing the cache size
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define MIN_SIZE (1024)              // the smallest size to test
#define MAX_SIZE (64 * 1024 * 1024)   // 64 MB, guarantee to be bigger than all of the cache combine
#define ITERATIONS 100000000         // total access

// tests memory latency for a buffer of size bytes.
void test_size(size_t size) {

    //number of pointers that fit in size
    size_t count = size / sizeof(void*);
    // allocate the memory
    void **buffer = (void **)malloc(size);
    if (!buffer) return;

    // cyclic memory pointer
    for (size_t i = 0; i < count; i++) {
        buffer[i] = (void *)&buffer[(i + 1) % count];
    }

    size_t stride = 128 / sizeof(void*);
    for (size_t i = 0; i < count; i++) {
        buffer[i] = (void *)&buffer[(i + stride) % count];
    }

    // Warm up the cache
    void **ptr = (void **)buffer[0];
    for (int i = 0; i < 1000000; i++) {
        ptr = (void **)*ptr;
    }

    // Timing loop
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < ITERATIONS; i++) {
        ptr = (void **)*ptr;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double nanoseconds = (elapsed * 1e9) / ITERATIONS;

    printf("%8zu KB | Latency: %6.2f ns | Ptr: %p\n", size / 1024, nanoseconds, ptr);

    free(buffer);
}

int main() {
    printf("Probing Cache Latencies...\n");
    printf("------------------------------------------\n");
    
    // Test sizes from 1KB up to MAX_SIZE, doubling each time
    for (size_t size = MIN_SIZE; size <= MAX_SIZE; size *= 2) {
        test_size(size);
    }

    return 0;
}