// probing the tlb sizes
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 3000
#define ACCESSES 1000000

int main() {
    // allocate a huge buffer
    size_t total_bytes = (size_t)MAX_PAGES * PAGE_SIZE;
    char *memory = (char*)malloc(total_bytes);
    if (!memory) return 1;

    // force allocations
    for (size_t i = 0; i < total_bytes; i += PAGE_SIZE) {
        memory[i] = 1;
    }

    printf("Probing TLB Size (Stride = 4KB)\n");
    printf("Entries\t\tAvg_Time(ns)\n");
    printf("----------------------------\n");

    int test_counts[] = {
        8, 16, 32, 48, 64, 72, 96, 128,
        256, 512, 1024, 1500, 1536, 1600,
        2000, 2048, 2100, 2500, 0
    };

    for (int t = 0; test_counts[t] != 0; t++) {
        int entries = test_counts[t];
        
        // Linking pages forces each memory access to touch a different virtual page.
        // When the number of distinct pages exceeds the TLB capacity, TLB misses occur.
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