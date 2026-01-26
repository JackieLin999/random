#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

// 128KB fits in E-Core L2 but forces L1 misses
#define ARRAY_SIZE (128 * 1024) 
#define ACCESSES 10000000

void pin_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("sched_setaffinity");
        exit(1);
    }
    printf("Successfully pinned to Core %d\n", core_id);
}

int main() {
    pin_to_core(16); // Target E-Core

    char *memory = (char*)malloc(ARRAY_SIZE);
    memset(memory, 1, ARRAY_SIZE);

    printf("Probing E-Core Cache Line Size\n");
    printf("Stride(B)\tAvg_Time(ns)\n");
    printf("------------------------\n");

    // Test strides: 4, 8, ..., 256
    for (int stride = 4; stride <= 256; stride *= 2) {
        
        for (int i = 0; i < ARRAY_SIZE - stride; i += stride) {
            *(void**)(&memory[i]) = (void**)(&memory[i + stride]);
        }
        *(void**)(&memory[ARRAY_SIZE - stride]) = (void**)(&memory[0]);

        void **p = (void**)(&memory[0]);
        struct timespec start, end;

        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int k = 0; k < ACCESSES; k++) {
            p = (void**)*p;
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        double total_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        printf("%d\t\t%.4f\n", stride, total_ns / ACCESSES);
    }

    free(memory);
    return 0;
}