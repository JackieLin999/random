#include <stdio.h>
#include <cpuid.h>

int main() {
    unsigned int eax, ebx, ecx, edx;

    printf("=== Intel CPUID Leaf 0x18 (TLB Info) ===\n");

    for (unsigned int i = 0; ; i++) {
        __cpuid_count(0x18, i, eax, ebx, ecx, edx);

        if ((eax & 0x1F) == 0)
            break;  // No more TLB entries

        unsigned int tlb_type = eax & 0x1F;
        unsigned int level    = (eax >> 5) & 0x7;
        unsigned int ways     = ebx + 1;
        unsigned int entries  = ecx + 1;

        unsigned int page_4k = edx & 0x1;
        unsigned int page_2m = (edx >> 1) & 0x1;
        unsigned int page_4m = (edx >> 2) & 0x1;
        unsigned int page_1g = (edx >> 3) & 0x1;

        printf("\nSubleaf %u:\n", i);

        printf("  TLB Type: ");
        if (tlb_type == 1) printf("Data TLB\n");
        else if (tlb_type == 2) printf("Instruction TLB\n");
        else if (tlb_type == 3) printf("Unified TLB\n");
        else printf("Unknown\n");

        printf("  Level: L%u\n", level);
        printf("  Associativity (ways): %u\n", ways);
        printf("  Entries: %u\n", entries);

        printf("  Page sizes supported:");
        if (page_4k) printf(" 4K");
        if (page_2m) printf(" 2M");
        if (page_4m) printf(" 4M");
        if (page_1g) printf(" 1G");
        printf("\n");
    }

    return 0;
}
