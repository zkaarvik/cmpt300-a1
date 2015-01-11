#include <stdio.h>
#include <string.h>

#define MAX_LINE_LENGTH 256

int main()
{
    char input_line[MAX_LINE_LENGTH];
    FILE *fp_cpuinfo;

    // Read cpuinfo file and obtain processor type
    fp_cpuinfo = fopen("/proc/cpuinfo", "r");
    while (fgets(input_line, MAX_LINE_LENGTH, fp_cpuinfo))
    {
        //printf("%s", input_line);
        if (strstr(input_line, "model name") != NULL)
        {
            printf("%s", input_line);
        }
    }
    fclose(fp_cpuinfo);


    //printf("Hello World!\n%s", input_line);
    //getchar();
    return 0;
}


// /proc/cpuinfo - cpu info
// /proc/version - kernel info
// /proc/meminfo - memory info
// /proc/uptime - uptime