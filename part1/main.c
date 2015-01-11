#include <stdio.h>
#include <string.h>

#define MAX_LINE_LENGTH 256
#define ID_CPUINFO "model name\t: "
#define ID_MEMINFO "MemTotal:"

int main()
{
    char str_input_line[MAX_LINE_LENGTH];
    size_t index;
    FILE *fp_cpuinfo;
    FILE *fp_kernelversion;
    FILE *fp_meminfo;
    FILE *fp_uptime;

    /*
     *  Obtain processor type
     */
    fp_cpuinfo = fopen("/proc/cpuinfo", "r");
    while (fgets(str_input_line, MAX_LINE_LENGTH, fp_cpuinfo))
    {
        //Check for line beginning with "model name", this is the processor type we need
        if (strstr(str_input_line, ID_CPUINFO) != NULL)
        {
            index = strlen(ID_CPUINFO);
            printf("Processor type: %s", str_input_line + index);
            break;
        }
    }
    fclose(fp_cpuinfo);

    /*
     *  Obtain kernel version
     */
    fp_kernelversion = fopen("/proc/sys/kernel/osrelease", "r");
    //The only line in the file should be the kernel version
    fgets(str_input_line, MAX_LINE_LENGTH, fp_kernelversion);
    printf("Kernel version: %s", str_input_line);
    fclose(fp_kernelversion);

    /*
     *  Obtain total memory
     */
    fp_meminfo = fopen("/proc/meminfo", "r");
    while (fgets(str_input_line, MAX_LINE_LENGTH, fp_meminfo))
    {
        //Check for line beginning with "MemTotal:", it is followed by an unknown number of spaces before the memory amount
        if (strstr(str_input_line, ID_MEMINFO) != NULL)
        {
            index = strlen(ID_MEMINFO);
            while(str_input_line[index] == ' ') index++;
            printf("Total memory: %s", str_input_line + index);
            break;
        }
    }
    fclose(fp_meminfo);

    /*
     *  Obtain uptime
     */
    fp_uptime = fopen("/proc/uptime", "r");
    //Use scanf to get the first characters in the file up to the first whitespace character, which is the uptime in seconds
    //fgets(str_input_line, MAX_LINE_LENGTH, fp_uptime);
    fscanf(fp_uptime, "%s", str_input_line);
    printf("System uptime: %s seconds\n", str_input_line);
    fclose(fp_uptime);

    return 0;
}