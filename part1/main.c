#include <stdio.h>
#include <string.h>

#define MAX_LINE_LENGTH 256
#define ID_CPUINFO "model name\t: "
#define ID_MEMINFO "MemTotal:"

void print_cpuinfo();
void print_kernel_version();
void print_total_memory();
void print_uptime();

int main()
{
    print_cpuinfo();
    print_kernel_version();
	print_total_memory();
	print_uptime();

    return 0;
}

/*
 *  Obtain processor type
 */
void print_cpuinfo()
{
    FILE *fp_cpuinfo;
    char str_input_line[MAX_LINE_LENGTH];
    size_t index;

    fp_cpuinfo = fopen("/proc/cpuinfo", "r");
    while (fgets(str_input_line, MAX_LINE_LENGTH, fp_cpuinfo))
    {
        //Check for line beginning with "model name", this is the processor type we need
        if (strstr(str_input_line, ID_CPUINFO) != NULL)
        {
            index = strlen(ID_CPUINFO);
            printf("Processor type:\t%s", str_input_line + index);
            break;
        }
    }
    fclose(fp_cpuinfo);
}

/*
 *  Obtain kernel version
 */
void print_kernel_version()
{
    FILE *fp_kernelversion;
    char str_input_line[MAX_LINE_LENGTH];

    fp_kernelversion = fopen("/proc/sys/kernel/osrelease", "r");
    //The only line in the file should be the kernel version
    fgets(str_input_line, MAX_LINE_LENGTH, fp_kernelversion);
    printf("Kernel version:\t%s", str_input_line);
    fclose(fp_kernelversion);
}

/*
 *  Obtain total memory
 */
void print_total_memory()
{
	FILE *fp_meminfo;
	char str_input_line[MAX_LINE_LENGTH];
	size_t index;

    fp_meminfo = fopen("/proc/meminfo", "r");
    while (fgets(str_input_line, MAX_LINE_LENGTH, fp_meminfo))
    {
        //Check for line beginning with "MemTotal:", it is followed by an unknown number of spaces before the memory amount
        if (strstr(str_input_line, ID_MEMINFO) != NULL)
        {
            index = strlen(ID_MEMINFO);
            while (str_input_line[index] == ' ') index++;
            printf("Total memory:\t%s", str_input_line + index);
            break;
        }
    }
    fclose(fp_meminfo);
}

/*
 *  Obtain uptime
 */
void print_uptime()
{
	FILE *fp_uptime;
	char str_input_line[MAX_LINE_LENGTH];

	fp_uptime = fopen("/proc/uptime", "r");
    //Use scanf to get the first characters in the file up to the first whitespace character, which is the uptime in seconds
    fscanf(fp_uptime, "%s", str_input_line);
    printf("System uptime:\t%s seconds\n", str_input_line);
    fclose(fp_uptime);
}
