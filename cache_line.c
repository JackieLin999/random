#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <x86intrin.h>
#include <unistd.h>

#define STRIDE 4096   // one page
#define MAX_PAGES 4096
#define REPEAT 1000

int main() {
    int page_size = sysconf(_SC_PAGESIZE);
    printf("Page size = %d bytes\n", page_size);

    char *buf = aligned_alloc(page_size, MAX_PAGES * page_size);

    for (int pages = 1; pages <= 1024; pages *= 2) {
        volatile char sum = 0;
        uint64_t start = __rdtsc();

        for (int r = 0; r < REPEAT; r++) {
            for (int i = 0; i < pages; i++) {
                sum += buf[i * STRIDE];
            }
        }

        uint64_t end = __rdtsc();
        printf("Pages: %4d | cycles/access: %.2f\n",
               pages,
               (double)(end - start) / (pages * REPEAT));
    }

    free(buf);
    return 0;
}