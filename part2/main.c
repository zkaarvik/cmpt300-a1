#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT_LENGTH 4096
#define DELIMS " \t\n\r"

int main()
{
    char user_input[MAX_INPUT_LENGTH];
    char *saveptr;
    char *cur_token;
    char *cur_prog;
    char **cur_argv;
    int cur_argc;
    int i;

    //main loop
    while (1)
    {
        cur_argc = 0;

        printf("$ ");
        fgets(user_input, MAX_INPUT_LENGTH, stdin);

        //Parse the string into tokens - first the program name
        if (cur_token = strtok_r(user_input, DELIMS, &saveptr))
        {
            cur_prog = cur_token;
        }
        else continue;

        //Then each subsequent token is an argument (until we see | or &)
        cur_argv = malloc(sizeof(char *));
        while (cur_token = strtok_r(NULL, DELIMS, &saveptr))
        {
            cur_argc++;
            cur_argv = realloc(cur_argv, sizeof(char *) * cur_argc);
            if (cur_argv == NULL)
            {
                perror("Error allocating memory\n");
                free(cur_argv);
                break;
            }
            cur_argv[cur_argc - 1] = cur_token;
        }



        //*******DEBUG
        printf("program name: %s \n", cur_prog);
        for (i = 0; i < cur_argc; i++)
        {
            printf("argv array[%i]: %s \n", i, cur_argv[i]);
        }
        //END *******DEBUG




        //We have the program/file to execute and a list of arguments.. now we can fork and execute




        //Free the argv array
        free(cur_argv);
    }

    return 0;
}