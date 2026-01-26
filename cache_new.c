#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <sched.h>
#include <string.h>

#define CACHE_LINE 64

uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Fisher-Yates shuffle for indices
void shuffle(int *array, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

double measure(size_t size_kb) {
    size_t size_bytes = size_kb * 1024;
    int num_lines = size_bytes / CACHE_LINE;
    if (num_lines < 2) return 0;

    // Allocate memory
    void **mem = (void**)malloc(size_bytes);
    if (!mem) return 0;

    // Create random permutation of cache lines
    int *indices = malloc(num_lines * sizeof(int));
    for (int i = 0; i < num_lines; i++) indices[i] = i;
    shuffle(indices, num_lines);

    // Link them: mem[index[i]] points to mem[index[i+1]]
    for (int i = 0; i < num_lines - 1; i++) {
        int current = indices[i] * (CACHE_LINE / sizeof(void*));
        int next = indices[i+1] * (CACHE_LINE / sizeof(void*));
        mem[current] = (void*)&mem[next];
    }
    // Close the loop
    mem[indices[num_lines-1] * (CACHE_LINE / sizeof(void*))] = (void*)&mem[indices[0] * (CACHE_LINE / sizeof(void*))];

    // Benchmark
    void **p = (void**)mem;
    int iterations = 10000000;
    
    // Warmup
    for(int i=0; i<100000; i++) p = (void**)*p;

    uint64_t start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        p = (void**)*p;
    }
    uint64_t end = get_time_ns();

    free(indices);
    free(mem);

    return (double)(end - start) / iterations;
}

int main() {
    // Pin to core 0 (usually a P-Core)
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    sched_setaffinity(0, sizeof(set), &set);

    printf("High-Res Cache Probe\nSize(KB)\tLatency(ns)\n---------------------------\n");

    // 1. Scan L1 Boundary (Fine steps of 4KB)
    for (int s = 16; s <= 128; s += 4) {
        printf("%d\t\t%.4f\n", s, measure(s));
    }
    printf("--- L2 Transition ---\n");

    // 2. Scan L2 Boundary (Steps of 64KB)
    for (int s = 256; s <= 3072; s += 64) {
        printf("%d\t\t%.4f\n", s, measure(s));
    }
    printf("--- L3 Transition ---\n");

    // 3. Scan L3 Boundary (Steps of 1MB)
    for (int s = 4096; s <= 65536; s += 1024) {
        printf("%d\t\t%.4f\n", s, measure(s));
    }

    return 0;
}