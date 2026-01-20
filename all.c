#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Helper function to read a single line from a file in /sys */
void read_sysfs_entry(int index, char *entry, char *buffer, size_t size) {
    char path[256];
    // Construct the path: /sys/devices/system/cpu/cpu0/cache/indexX/ENTRY
    snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu0/cache/index%d/%s", index, entry);
    
    FILE *fp = fopen(path, "r");
    if (fp) {
        if (fgets(buffer, size, fp) != NULL) {
            // Remove trailing newline
            buffer[strcspn(buffer, "\n")] = 0;
        }
        fclose(fp);
    } else {
        // If file doesn't exist, assume we reached the end of cache levels
        buffer[0] = '\0';
    }
}

int main() {
    printf("=== Virtual Machine Memory Stats ===\n\n");

    // ---------------------------------------------------------
    // 1. Determine Page Size
    // ---------------------------------------------------------
    long pageSize = sysconf(_SC_PAGESIZE);
    printf("1. Memory Page Size: %ld bytes (%.2f KB)\n", 
           pageSize, pageSize / 1024.0);

    // ---------------------------------------------------------
    // 2. Determine Cache Stats (L1, L2, etc.)
    // ---------------------------------------------------------
    // We iterate through cache indices (index0, index1, etc.)
    // usually: index0=L1 Data, index1=L1 Instruction, index2=L2...
    
    printf("\n2. Cache Information (via /sys/devices/...):\n");
    printf("----------------------------------------------------------------\n");
    printf("%-10s | %-15s | %-10s | %-10s | %-15s\n", 
           "Level", "Type", "Size", "Line Size", "Associativity");
    printf("----------------------------------------------------------------\n");

    int i = 0;
    while (1) {
        char level[16], type[32], size[32], line_size[32], assoc[32];
        
        // Check if this index exists by trying to read the 'level'
        read_sysfs_entry(i, "level", level, sizeof(level));
        if (level[0] == '\0') break; // Stop if no more cache indices

        read_sysfs_entry(i, "type", type, sizeof(type));
        read_sysfs_entry(i, "size", size, sizeof(size));
        read_sysfs_entry(i, "coherency_line_size", line_size, sizeof(line_size));
        read_sysfs_entry(i, "ways_of_associativity", assoc, sizeof(assoc));

        printf("L%-9s | %-15s | %-10s | %-10s | %-15s\n", 
               level, type, size, line_size, assoc);
        i++;
    }

    // ---------------------------------------------------------
    // 3. Note on TLB
    // ---------------------------------------------------------
    // TLB sizes are architecture specific (x86 vs ARM) and usually
    // require executing assembly ('cpuid' instruction) rather than 
    // standard C file I/O. 
    printf("\n3. TLB Information:\n");
    printf("   (TLB size is architecture-specific and difficult to query via standard C APIs.\n");
    printf("    To see TLB info, try running the command: 'cpuid -1' or 'x86info -c')\n");

    return 0;
}