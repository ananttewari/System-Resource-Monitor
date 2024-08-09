// Include the necessary header files
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <mntent.h>
#include "hash.c"
#include "cpu.c"

// Define some constants
#define KB 1024
#define MB 1048576
#define MAXFS 10          // Maximum number of file systems to monitor
#define MAXSOCK 10        // Maximum number of sockets to monitor
#define INTERVAL 5        // Interval in seconds between each update
#define HASH_SIZE 100     // Size of the hash map

// Define a structure to store disk usage information
struct diskinfo {
    char name[256];         // Name of the file system
    unsigned long total;    // Total size in bytes
    unsigned long free;     // Free size in bytes
    unsigned long used;     // Used size in bytes
    double percent;         // Percentage of used size
};

// Define a structure for hash node
struct hash_node {
    char name[256];         // Name of the file system
    struct diskinfo info;   // Disk usage information
    struct hash_node* next; // Pointer to the next node
};

// Define a hash map
struct hash_map {
    struct hash_node* nodes[HASH_SIZE]; // Array of hash nodes
};

// Global variables
struct sysinfo sys;         // System information
struct hash_map fs_map;     // Hash map for file systems
int numfs = 0;              // Number of file systems

// Function prototypes
void get_sysinfo();
void get_diskinfo();
void display_diskinfo();
void print_menu();
void handle_user_input();
unsigned long Hash(const char* str);
void insert_fs(const char* name, struct diskinfo info);
struct diskinfo* search_fs(const char* name);

// Function to get the system information
void get_sysinfo() {
    if (sysinfo(&sys) == -1) {
        perror("sysinfo");
        exit(1);
    }
}

// A function to display the system information
void display_sysinfo() {
    char uptime[256];
    int days, hours, minutes, seconds;
    double loadavg[3];

    seconds = sys.uptime % 60;
    minutes = (sys.uptime / 60) % 60;
    hours = (sys.uptime / 3600) % 24;
    days = sys.uptime / 86400;

    sprintf(uptime, "%d days, %d hours, %d minutes, %d seconds", days, hours, minutes, seconds);

    loadavg[0] = (double) sys.loads[0] / 65536;
    loadavg[1] = (double) sys.loads[1] / 65536.0;
    loadavg[2] = (double) sys.loads[2] / 65536.0;

    printf("System Information\n");
    printf("==================\n");
    printf("Uptime: %s\n", uptime);
    printf("Load average: %.2f, %.2f, %.2f\n", loadavg[0], loadavg[1], loadavg[2]);
    printf("Total RAM: %lu MB\n", sys.totalram / MB);
    printf("Free RAM: %lu MB\n", sys.freeram / MB);
    printf("Used RAM: %lu MB\n", (sys.totalram - sys.freeram) / MB);
    printf("Shared RAM: %lu MB\n", sys.sharedram / MB);
    printf("Buffer RAM: %lu MB\n", sys.bufferram / MB);
    printf("Total swap: %lu MB\n", sys.totalswap / MB);
    printf("Free swap: %lu MB\n", sys.freeswap / MB);
    printf("Used swap: %lu MB\n", (sys.totalswap - sys.freeswap) / MB);
    printf("Number of processes: %d\n", sys.procs);
}

// Function to get the disk usage information
void get_diskinfo() {
    FILE* fp;
    struct mntent* ent;

    fp = setmntent("/etc/mtab", "r");
    if (fp == NULL) {
        perror("setmntent");
        exit(1);
    }

    // Read each entry from the file
    while ((ent = getmntent(fp)) != NULL) {
        struct statvfs buf;

        // Call the statvfs function and check for errors
        if (statvfs(ent->mnt_dir, &buf) == -1) {
            perror("statvfs");
            exit(1);
        }

        // Store the disk usage information in the hash map
        struct diskinfo info;
        strcpy(info.name, ent->mnt_dir);
        info.total = buf.f_blocks * buf.f_frsize;
        info.free = buf.f_bfree * buf.f_frsize;
        info.used = info.total - info.free;
        info.percent = (double)info.used / info.total * 100;

        insert_fs(ent->mnt_dir, info);

        numfs++;

        // Check if the maximum number of file systems is reached
        if (numfs == MAXFS) {
            break;
        }
    }

    // Close the file
    endmntent(fp);
}


// Function to display the disk usage information
void display_diskinfo() {
    printf("\nDisk Usage Information\n");
    printf("=================================================================\n");
    printf("Name                   	Total         	Free          	Used          	Percent\n");
    printf("-----------------------------------------------------------------\n");

    // Iterate through the hash map and display disk usage information
    for (int i = 0; i < HASH_SIZE; ++i) {
        struct hash_node* current = fs_map.nodes[i];
        while (current != NULL) {
            printf("%-20s %12lu KB %12lu KB %12lu KB %12.2f%%\n", current->info.name, 
                current->info.total / KB, current->info.free / KB, current->info.used / KB, 
                current->info.percent);
            current = current->next;
        }
    }
}


void print_disk_usage() {
    // Get disk usage
    struct statvfs buffer;
    statvfs("/", &buffer);
    unsigned long totalDiskSpace = buffer.f_blocks * buffer.f_frsize;
    unsigned long freeDiskSpace = buffer.f_bfree * buffer.f_frsize;
    unsigned long usedDiskSpace = totalDiskSpace - freeDiskSpace;
    float freeDiskPercent = ((float)freeDiskSpace / totalDiskSpace) * 100;
    float usedDiskPercent = ((float)usedDiskSpace / totalDiskSpace) * 100;

    printf("Disk Usage:\n");
    printf("Total: %lu KB\n", totalDiskSpace / 1024);
    printf("Free: %lu KB (%.2f%%)\n", freeDiskSpace / 1024, freeDiskPercent);
    printf("Used: %lu KB (%.2f%%)\n\n", usedDiskSpace / 1024, usedDiskPercent);
}




// Function to insert disk usage information into the hash map
void insert_fs(const char* name, struct diskinfo info) {
    unsigned long index = Hash(name) % HASH_SIZE;
    struct hash_node* node = (struct hash_node*)malloc(sizeof(struct hash_node));
    if (node == NULL) {
        perror("Memory allocation failed");
        exit(1);
    }
    strcpy(node->name, name);
    node->info = info;
    node->next = fs_map.nodes[index];
    fs_map.nodes[index] = node;
}

// Function to search for disk usage information in the hash map
struct diskinfo* search_fs(const char* name) {
    unsigned long index = Hash(name) % HASH_SIZE;
    struct hash_node* current = fs_map.nodes[index];
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return &(current->info);
        }
        current = current->next;
    }
    return NULL;
}

// Hash function
unsigned long Hash(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++) != 0) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

void print_menu() {
    printf("\n=== Menu ===\n");
    printf("1. Display System Information\n");
    printf("2. Display Disk Usage Information\n");
    printf("3. Display Memory Usage Information\n");
    printf("4. Cpu Usage Information\n");
    printf("5. Exit\n");
    printf("Select an option: ");
}

void handle_user_input() {
    int option;
    scanf("%d", &option);
    getchar(); // Consume the newline character

    switch (option) {
        case 1:
            get_sysinfo();
            printf("\n");
            display_sysinfo();
            break;
        case 2:
            get_diskinfo();
            printf("\n");
            display_diskinfo();
            printf("\n");
            print_disk_usage();
            break;
        case 3: printf("\n");
        	print_memory_usage();
        	printf("\n");
        	monitor_memory_usage();
        	break;
        case 4: float cpu_usage;
    		read_cpu_usage(&cpu_usage);
    		printf("CPU Usage: %.2f%%\n", cpu_usage);
    		break;
        case 5:
            exit(0);
        default:
            printf("Invalid option. Please select again.\n");
            break;
    }
}



int main() {
    while (1) {
        print_menu();
        handle_user_input();
    }
    return 0;
}

/*// A function to display the disk space analysis information
void display_disk_analysis() {
    printf("\nDisk Space Analysis\n");
    printf("===================\n");
    printf("File System            Free Space       Free Space Percentage\n");

    // Iterate through the hash map and display disk usage information
    for (int i = 0; i < HASH_SIZE; ++i) {
        struct hash_node* current = fs_map.nodes[i];
        while (current != NULL) {
            printf("%-20s %12lu KB      %12.2f%%\n", current->info.name, 
                current->info.free / KB, current->info.percent);
            current = current->next;
        }
    }
}
*/

// Main function
/*int main() {
    get_sysinfo();
    print_disk_usage();
    print_memory_usage();
    get_diskinfo();
    display_sysinfo();
    display_diskinfo();    
    sleep(INTERVAL);

    return 0;
}
*/







