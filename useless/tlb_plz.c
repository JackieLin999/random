#include <stdio.h>
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <cpuid.h>
#endif

// AMD raw CPUID wrapper
void amd_cpuid(unsigned int leaf, unsigned int sub, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx) {
#ifdef _MSC_VER
    int regs[4];
    __cpuidex(regs, leaf, sub);
    *eax = regs[0]; *ebx = regs[1]; *ecx = regs[2]; *edx = regs[3];
#else
    __cpuid_count(leaf, sub, *eax, *ebx, *ecx, *edx);
#endif
}

// Intel leaf 2 descriptor decoder
void intel_leaf2() {
    unsigned int eax, ebx, ecx, edx;
#ifdef _MSC_VER
    int regs[4];
    __cpuid(regs, 2);
    eax = regs[0]; ebx = regs[1]; ecx = regs[2]; edx = regs[3];
#else
    __cpuid(2, eax, ebx, ecx, edx);
#endif

    unsigned int regs_arr[4] = {eax, ebx, ecx, edx};
    char *names[4] = {"EAX", "EBX", "ECX", "EDX"};

    printf("Intel Leaf 2 TLB Descriptors:\n");
    int found = 0;

    for (int r = 0; r < 4; r++) {
        int start = (r==0)?1:0; // Skip EAX first byte
        for (int b = start; b < 4; b++) {
            unsigned char val = (regs_arr[r] >> (b*8)) & 0xFF;
            if (val != 0) {
                printf("  [%s Byte %d] Descriptor: 0x%02X -> ", names[r], b, val);
                switch(val) {
                    case 0x01: printf("ITLB: 4KB, 4-way, 32 entries\n"); break;
                    case 0x02: printf("ITLB: 4MB, fully assoc, 2 entries\n"); break;
                    case 0x03: printf("DTLB: 4KB, 4-way, 64 entries\n"); break;
                    case 0x0B: printf("ITLB: 4MB, 4-way, 4 entries\n"); break;
                    case 0x50: printf("ITLB: 4K/2M/4M, 64 entries\n"); break;
                    case 0x51: printf("ITLB: 4K/2M/4M, 128 entries\n"); break;
                    case 0x5A: printf("DTLB0: 2M/4M, 4-way, 32 entries\n"); break;
                    case 0x5B: printf("DTLB: 4K/4M, 64 entries\n"); break;
                    case 0xFE: printf("Use Leaf 0x18 for TLB info\n"); break;
                    case 0xFF: printf("Leaf 2 does not contain cache info\n"); break;
                    default: printf("Unknown descriptor\n"); break;
                }
                found = 1;
            }
        }
    }

    if (!found) printf("No TLB descriptors found (VM may hide info)\n");
}

// AMD Leaf 0x80000005 decoder
void amd_leaf5() {
    unsigned int eax, ebx, ecx, edx;
    amd_cpuid(0x80000005, 0, &eax, &ebx, &ecx, &edx);

    printf("AMD Leaf 0x80000005 (L1 TLB) info:\n");

    // Decode EBX: 4KB pages
    unsigned int i_tlb_entries_4k = ebx & 0xFF;
    unsigned int i_tlb_assoc_4k   = (ebx >> 8) & 0xFF;
    unsigned int d_tlb_entries_4k = (ebx >> 16) & 0xFF;
    unsigned int d_tlb_assoc_4k   = (ebx >> 24) & 0xFF;

    printf("  4KB Pages:\n");
    printf("    ITLB: %u entries, Associativity: %u\n", i_tlb_entries_4k, i_tlb_assoc_4k);
    printf("    DTLB: %u entries, Associativity: %u\n", d_tlb_entries_4k, d_tlb_assoc_4k);

    // Decode EAX: 2MB/4MB pages
    unsigned int i_tlb_entries_large = eax & 0xFF;
    unsigned int i_tlb_assoc_large   = (eax >> 8) & 0xFF;
    unsigned int d_tlb_entries_large = (eax >> 16) & 0xFF;
    unsigned int d_tlb_assoc_large   = (eax >> 24) & 0xFF;

    printf("  2MB/4MB Pages:\n");
    printf("    ITLB: %u entries, Associativity: %u\n", i_tlb_entries_large, i_tlb_assoc_large);
    printf("    DTLB: %u entries, Associativity: %u\n", d_tlb_entries_large, d_tlb_assoc_large);
}

int main() {
    printf("==== Intel TLB info (Leaf 2) ====\n");
    intel_leaf2();

    printf("\n==== AMD TLB info (Leaf 0x80000005) ====\n");
    amd_leaf5();

    return 0;
}
