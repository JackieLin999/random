#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h>  // For __rdtsc() and _mm_clflush

#define CACHE_SIZE (1280 * 1024)    // 48 KB L1 cache
#define LINE_SIZE 64              // Cache line size in bytes
#define MAX_WAYS 32               // Max expected associativity
#define ITERATIONS 1000000        // Loop to amplify timing

// Flush a cache line
static inline void flush(void *p) {
    _mm_clflush(p);
}

// Time memory access in cycles
static inline uint64_t time_access(volatile char *addr, int ways) {
    uint64_t start, end;
    start = __rdtsc();

    for (int i = 0; i < ITERATIONS; i++) {
        for (int w = 0; w < ways; w++) {
            addr[w * LINE_SIZE] += 1;
        }
    }

    end = __rdtsc();
    return end - start;
}

int main() {
    printf("Probing L1 cache associativity (48 KB, 64 B line size)...\n");

    size_t set_size = LINE_SIZE; 
    char *array = aligned_alloc(LINE_SIZE, MAX_WAYS * set_size);
    if (!array) {
        perror("aligned_alloc");
        return 1;
    }

    for (int ways = 1; ways <= MAX_WAYS; ways++) {
        // Flush memory from cache
        for (int i = 0; i < ways; i++)
            flush(&array[i * LINE_SIZE]);

        uint64_t cycles = time_access(array, ways);
        printf("Ways = %d, Access time = %lu cycles\n", ways, cycles);
    }

    free(array);
    return 0;
}
