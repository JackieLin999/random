#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define MAX_SIZE (64 * 1024 * 1024)   // 64 MB
#define STRIDE   64                  // cache line size
#define ITERS    10000000

static inline uint64_t ns_diff(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) * 1000000000ULL +
           (b.tv_nsec - a.tv_nsec);
}

int main() {
    char *array;
    posix_memalign((void **)&array, 64, MAX_SIZE);

    volatile char sink;
    printf("WorkingSet(KB), AvgAccess(ns)\n");

    for (size_t size = 1 * 1024; size <= MAX_SIZE; size *= 2) {
        struct timespec start, end;

        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int iter = 0; iter < ITERS; iter++) {
            for (size_t i = 0; i < size; i += STRIDE) {
                sink = array[i];
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        uint64_t total_ns = ns_diff(start, end);
        double accesses = (double)(ITERS * (size / STRIDE));
        double avg_ns = total_ns / accesses;

        printf("%zu, %.2f\n", size / 1024, avg_ns);
    }

    free(array);
    return 0;
}
