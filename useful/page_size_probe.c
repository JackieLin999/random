#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#define NUM_ACCESSES 10000 
// We need a large enough buffer to accommodate the largest stride * accesses
// 10,000 accesses * 32KB stride = ~320 MB. 
// We use mmap to ensure we get a clean chunk of memory.
#define MAX_MEM (size_t)(NUM_ACCESSES * 32768) 

int main() {
    // Allocate a huge block of memory
    char *memory = mmap(NULL, MAX_MEM, PROT_READ | PROT_WRITE, 
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (memory == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Warmup: Write to every page to ensure physical memory is allocated
    // We assume 4KB is the minimum possible page size for the warmup loop
    for (size_t i = 0; i < MAX_MEM; i += 4096) {
        memory[i] = 1;
    }

    printf("Probing Page Size (Fixed Access Count, Variable Stride)\n");
    printf("Stride(B)\tAvg_Time(ns)\n");
    printf("------------------------\n");

    // Test strides from 512 bytes up to 16KB
    // We expect the graph to rise and then flatten at 4096
    int strides[] = {512, 1024, 2048, 4096, 8192, 16384, 0};

    for (int s = 0; strides[s] != 0; s++) {
        int stride = strides[s];
        
        // --- Build Pointer Chain ---
        // We only link enough items for NUM_ACCESSES
        for (int i = 0; i < NUM_ACCESSES; i++) {
            void **current = (void**)(&memory[i * stride]);
            void **next;
            
            // On the last one, loop back to start
            if (i == NUM_ACCESSES - 1) {
                next = (void**)(&memory[0]); 
            } else {
                next = (void**)(&memory[(i + 1) * stride]);
            }
            *current = next;
        }

        // --- Measure ---
        void **p = (void**)(&memory[0]);
        struct timespec start, end;

        clock_gettime(CLOCK_MONOTONIC, &start);
        
        // This loop executes exactly NUM_ACCESSES times
        for (int k = 0; k < NUM_ACCESSES; k++) {
            p = (void**)*p;
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);

        double total_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        printf("%d\t\t%.4f\n", stride, total_ns / NUM_ACCESSES);
    }

    munmap(memory, MAX_MEM);
    return 0;
}