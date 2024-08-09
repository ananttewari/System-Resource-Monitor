#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/sysinfo.h> // Include sysinfo.h for total RAM information

#define MAX_SIZE 1000

typedef struct {
    int key;
    int value;
    float percentage; // New field for percentage
} Item;

typedef struct {
    Item items[MAX_SIZE];
} HashTable;

void init_table(HashTable* table) {
    for (int i = 0; i < MAX_SIZE; i++) {
        table->items[i].key = -1;
        table->items[i].percentage = 0.0; // Initialize percentage
    }
}

int hash(int key) {
    return key % MAX_SIZE;
}

void insert(HashTable* table, int key, int value) {
    int index = hash(key);
    while (table->items[index].key != -1) {
        index = (index + 1) % MAX_SIZE;
    }
    table->items[index].key = key;
    table->items[index].value = value;
}

int get(HashTable* table, int key) {
    int index = hash(key);
    while (table->items[index].key != key) {
        index = (index + 1) % MAX_SIZE;
    }
    return table->items[index].value;
}

void monitor_memory_usage() {
    DIR* dir = opendir("/proc");
    if (dir == NULL) {
        printf("Could not open /proc directory.\n");
        return;
    }

    HashTable table;
    init_table(&table);

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        int pid = atoi(entry->d_name);
        if (pid > 0) {
            char filename[256];
            sprintf(filename, "/proc/%d/statm", pid);

            FILE* file = fopen(filename, "r");
            if (file != NULL) {
                int size;
                fscanf(file, "%d", &size);
                fclose(file);

                insert(&table, pid, size);
            }
        }
    }

    closedir(dir);

    // Get total system RAM
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        printf("Error getting system information.\n");
        return;
    }
    long long total_memory = info.totalram * info.mem_unit; // Convert to bytes

    // Calculate percentage for each process
    for (int i = 0; i < MAX_SIZE; i++) {
        if (table.items[i].key != -1) {
            table.items[i].percentage = (float)(table.items[i].value * 100) / (float)total_memory;
        }
    }

    // Bubble sort to sort memory usage in descending order
    for (int i = 0; i < MAX_SIZE - 1; i++) {
        for (int j = 0; j < MAX_SIZE - i - 1; j++) {
            if (table.items[j].value < table.items[j + 1].value) {
                Item temp = table.items[j];
                table.items[j] = table.items[j + 1];
                table.items[j + 1] = temp;
            }
        }
    }

    // Print memory usage in descending order with aligned columns
    printf("=================================================================\n");
    printf("%-10s %-10s %-10s\n", "PID", "Memory", "Usage");
       printf("-----------------------------------------------------------------\n");
    for (int i = 0; i < MAX_SIZE; i++) {
        if (table.items[i].key != -1) {
            printf("%-10d %-10d %6.2f %%\n", table.items[i].key, table.items[i].value, table.items[i].percentage);
        }
    }
}



void print_memory_usage() {
    // Get memory usage
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    unsigned long totalMemory = memInfo.totalram * memInfo.mem_unit;
    unsigned long freeMemory = memInfo.freeram * memInfo.mem_unit;
    unsigned long usedMemory = totalMemory - freeMemory;
    float freeMemoryPercent = ((float)freeMemory / totalMemory) * 100;
    float usedMemoryPercent = ((float)usedMemory / totalMemory) * 100;

    printf("Memory Usage:\n");
    printf("Total: %lu KB\n", totalMemory / 1024);
    printf("Free: %lu KB (%.2f%%)\n", freeMemory / 1024, freeMemoryPercent);
    printf("Used: %lu KB (%.2f%%)\n\n", usedMemory / 1024, usedMemoryPercent);
}


