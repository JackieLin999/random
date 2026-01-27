// exposes the size of the I cache for L1
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

// throw empty instructions
// NOP is no operations
#define NOP 0x90
#define RET 0xC3

int main() {
    printf("I-Cache Probe (Code Size vs Latency)\n");
    printf("Code_Size(KB)\tAvg_Time(ns)\n");
    printf("------------------------------------\n");

    // We test code sizes from 2KB up to 128KB
    // L1i is usually 32KB or 64KB, so this range is perfect.
    for (int size_kb = 2; size_kb <= 128; size_kb += 2) {
        
        size_t size_bytes = size_kb * 1024;
        
        // allocate memory for instructions
        void *code_mem = mmap(NULL, size_bytes, 
                              PROT_READ | PROT_WRITE | PROT_EXEC, 
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (code_mem == MAP_FAILED) {
            perror("mmap");
            return 1;
        }

        // fill it up with NOPs
        memset(code_mem, NOP, size_bytes);
        
        // last instr is a return statement
        unsigned char *p = (unsigned char *)code_mem;
        p[size_bytes - 1] = RET;

        // run
        void (*func_ptr)() = (void (*)())code_mem;
        
        struct timespec start, end;
        int iterations = 10000; // Run the function many times

        clock_gettime(CLOCK_MONOTONIC, &start);
        
        for (int i = 0; i < iterations; i++) {
            func_ptr(); // Execute the NOPs
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);

        // cleaning
        munmap(code_mem, size_bytes);

        // Calculate time per iteration
        double total_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        printf("%d\t\t%.4f\n", size_kb, total_ns / iterations);
    }

    return 0;
}