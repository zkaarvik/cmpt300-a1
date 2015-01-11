#include <stdio.h>
#include <string.h>

#define MAX_LINE_LENGTH 256
#define ID_CPUINFO "model name\t: "

int main()
{
    char str_input_line[MAX_LINE_LENGTH];
    size_t index;
    FILE *fp_cpuinfo;

    // Obtain processor type
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
    
    return 0;
}


// /proc/cpuinfo - cpu info
// /proc/version - kernel info
// /proc/meminfo - memory info
// /proc/uptime - uptime