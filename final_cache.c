#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#define CACHE_LINE_SIZE 64

// This struct guarantees we only touch 8 bytes (the pointer)
// and then skip exactly 56 bytes to the next cache line.
typedef struct Node {
    struct Node *next;
    char pad[CACHE_LINE_SIZE - sizeof(struct Node*)];
} Node;

uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Fisher-Yates shuffle to randomize the memory path
void shuffle(size_t *array, size_t n) {
    if (n <= 1) return;
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        size_t temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

double run_test(size_t size_kb) {
    size_t size_bytes = size_kb * 1024;
    size_t num_lines = size_bytes / CACHE_LINE_SIZE;

    // Safety check for very small sizes
    if (num_lines < 2) return 0.0;

    // Allocate the memory block
    Node *base = (Node*)malloc(size_bytes);
    if (!base) {
        printf("Allocation failed for %zu KB\n", size_kb);
        return 0.0;
    }

    // Create a temporary array of indices to shuffle
    size_t *indices = (size_t*)malloc(num_lines * sizeof(size_t));
    for (size_t i = 0; i < num_lines; i++) indices[i] = i;

    // Randomize the order! This kills the prefetcher.
    shuffle(indices, num_lines);

    // Link the nodes according to the random indices
    for (size_t i = 0; i < num_lines - 1; i++) {
        base[indices[i]].next = &base[indices[i+1]];
    }
    // Close the loop (circular buffer)
    base[indices[num_lines - 1]].next = &base[indices[0]];

    free(indices); // Done with the index list

    // --- MEASUREMENT ---
    Node *ptr = &base[0];
    
    // Warmup: Chase for a bit to get TLB/Cache hot
    // We chase at least enough to touch the whole array once
    for (size_t i = 0; i < num_lines; i++) {
        ptr = ptr->next;
    }

    // The Run: Fixed number of accesses
    int iterations = 5000000; 
    
    uint64_t start = get_time_ns();
    
    for (int i = 0; i < iterations; i++) {
        ptr = ptr->next;
    }
    
    uint64_t end = get_time_ns();

    free(base);
    
    return (double)(end - start) / iterations;
}

int main() {
    srand(time(NULL));

    // A specific list of sizes to catch the boundaries
    // We look for:
    // L1 (32, 48)
    // L2 (1024, 1280, 2048)
    // L3 (12MB - 32MB)
    int sizes_kb[] = {
        // L1 Range
        4, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 128,
        // L2 Range
        256, 512, 768, 1024, 1280, 1536, 2048, 3072, 4096,
        // L3 Range
        8192, 12288, 16384, 20480, 24576, 28672, 32768, 49152, 65536,
        0 // Terminator
    };

    printf("True Random Latency Probe (Defeats Prefetcher)\n");
    printf("Size(KB)\tLatency(ns)\n");
    printf("---------------------------\n");

    for (int i = 0; sizes_kb[i] != 0; i++) {
        double lat = run_test(sizes_kb[i]);
        printf("%d\t\t%.4f\n", sizes_kb[i], lat);
    }

    return 0;
}