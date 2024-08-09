#include <stdio.h>
#include <stdlib.h>

void read_cpu_usage(float *cpu_usage) {
    FILE *file = fopen("/proc/stat", "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int cpu_ticks[4] = {0}; // Array to store CPU ticks

    // Read first line (total CPU statistics)
    fgets(line, sizeof(line), file);

    // Parse the line to obtain CPU ticks
    sscanf(line, "cpu %d %d %d %d", &cpu_ticks[0], &cpu_ticks[1], &cpu_ticks[2], &cpu_ticks[3]);

    fclose(file);

    // Calculate total CPU time
    int total_cpu_time = 0;
    for (int i = 0; i < 4; i++) {
        total_cpu_time += cpu_ticks[i];
    }

    // Calculate CPU usage
    int idle_time = cpu_ticks[3]; // idle time is the 4th value
    int usage_time = total_cpu_time - idle_time;

    // Calculate CPU usage percentage
    *cpu_usage = ((float)usage_time / total_cpu_time) * 100;
}


