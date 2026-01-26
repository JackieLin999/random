#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 3000 // Test up to 3000 pages (approx 12MB, fits in L3)
#define ACCESSES 1000000

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
    pin_to_core(16);
    // Allocate enough memory for the max test size
    size_t total_bytes = (size_t)MAX_PAGES * PAGE_SIZE;
    char *memory = (char*)malloc(total_bytes);
    if (!memory) return 1;

    // Warmup: Write to every page to force OS allocation
    for (size_t i = 0; i < total_bytes; i += PAGE_SIZE) {
        memory[i] = 1;
    }

    printf("Probing TLB Size (Stride = 4KB)\n");
    printf("Entries\t\tAvg_Time(ns)\n");
    printf("----------------------------\n");

    // We test specific counts to catch L1 (usually 64) and L2 (usually 1536/2048)
    int test_counts[] = {
        8, 16, 32, 48, 64, 72, 96, 128,      // L1 Range
        256, 512, 1024, 1500, 1536, 1600,    // L2 Range (Lower)
        2000, 2048, 2100, 2500, 0            // L2 Range (Upper)
    };

    for (int t = 0; test_counts[t] != 0; t++) {
        int entries = test_counts[t];
        
        // --- Setup Pointer Chain ---
        // Link exactly 'entries' number of pages together
        for (int i = 0; i < entries; i++) {
            void **current = (void**)(&memory[i * PAGE_SIZE]);
            void **next;
            
            // Loop back on the last one
            if (i == entries - 1) {
                next = (void**)(&memory[0]);
            } else {
                next = (void**)(&memory[(i + 1) * PAGE_SIZE]);
            }
            *current = next;
        }

        // --- Measure ---
        void **p = (void**)(&memory[0]);
        struct timespec start, end;
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        for (int k = 0; k < ACCESSES; k++) {
            p = (void**)*p;
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);

        double total_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        printf("%d\t\t%.4f\n", entries, total_ns / ACCESSES);
    }

    free(memory);
    return 0;
}