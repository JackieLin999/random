// probe_assoc.c
// takes in an estimated cache size and test it with different stride size by force feeding a different associtivity each time
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#define LINE_SIZE 64       // typical cache line size
#define MAX_WAYS 32        // upper bound on associativity
#define ACCESSES 1000000

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./probe_assoc_est <estimated_cache_size_in_bytes>\n");
        return 1;
    }

    size_t cache_size = atol(argv[1]);
    if (cache_size == 0) {
        printf("Invalid cache size.\n");
        return 1;
    }

    // approximate number of sets
    int num_sets_guess = cache_size / (LINE_SIZE * MAX_WAYS);
    int stride = num_sets_guess * LINE_SIZE;
    printf("Estimated stride for single set: %d bytes\n", stride);

    // allocate memory to cover MAX_WAYS
    size_t mem_size = (size_t)MAX_WAYS * stride;
    char *memory = malloc(mem_size);
    if (!memory) { perror("malloc failed"); return 1; }

    memset(memory, 1, mem_size);

    printf("Probing associativity...\n");
    printf("Ways\tAvg_Time(ns)\n");
    printf("--------------------\n");

    for (int ways = 1; ways <= MAX_WAYS; ways++) {
        // setup pointer chain
        for (int k = 0; k < ways; k++) {
            void **current = (void**)(&memory[k * stride]);
            void **next = (k == ways - 1) ? (void**)&memory[0] : (void**)&memory[(k + 1) * stride];
            *current = next;
        }

        // measure latency
        void **p = (void**)&memory[0];
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < ACCESSES; i++) p = (void**)*p;
        clock_gettime(CLOCK_MONOTONIC, &end);

        double total_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        printf("%d\t%.4f\n", ways, total_ns / ACCESSES);
    }

    free(memory);
    return 0;
}
