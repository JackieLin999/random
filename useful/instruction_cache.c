// exposes the size of the I cache for L1
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

// Instructions to fill: 0x90 is NOP (No Operation) on x86
// 0xC3 is RET (Return)
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
        
        // 1. Allocate memory that is EXECUTABLE (PROT_EXEC)
        // This allows us to run the array as if it were a function
        void *code_mem = mmap(NULL, size_bytes, 
                              PROT_READ | PROT_WRITE | PROT_EXEC, 
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (code_mem == MAP_FAILED) {
            perror("mmap");
            return 1;
        }

        // 2. Fill the memory with NOPs (No-Operations)
        memset(code_mem, NOP, size_bytes);
        
        // 3. Make the last byte a RETURN instruction
        // This turns the array into a valid void function()
        unsigned char *p = (unsigned char *)code_mem;
        p[size_bytes - 1] = RET;

        // 4. Run the test
        // Cast the memory pointer to a function pointer
        void (*func_ptr)() = (void (*)())code_mem;
        
        struct timespec start, end;
        int iterations = 10000; // Run the function many times

        clock_gettime(CLOCK_MONOTONIC, &start);
        
        for (int i = 0; i < iterations; i++) {
            func_ptr(); // Execute the NOPs
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);

        // 5. Cleanup
        munmap(code_mem, size_bytes);

        // Calculate time per iteration
        double total_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        printf("%d\t\t%.4f\n", size_kb, total_ns / iterations);
    }

    return 0;
}