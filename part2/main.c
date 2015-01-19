#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

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
    pid_t pid;
    int child_status;

    //main loop
    while (1)
    {
        cur_argc = 0;

        printf("turtle$ ");
        fgets(user_input, MAX_INPUT_LENGTH, stdin);

        //Parse the string into tokens - first the program name
        if (cur_token = strtok_r(user_input, DELIMS, &saveptr))
        {
            cur_prog = cur_token;
        }
        else continue;

        //First argument must be the filename (req'd for execvp)
        //each subsequent token is an argument (until we see | or &)
        //Leaving an extra space in cur_argv array for NULL
        cur_argc++;
        cur_argv = malloc(sizeof(char *) * (cur_argc + 1));
        cur_argv[cur_argc - 1] = basename(cur_prog);
        while (cur_token = strtok_r(NULL, DELIMS, &saveptr))
        {
            cur_argc++;
            cur_argv = realloc(cur_argv, sizeof(char *) * (cur_argc + 1));
            if (cur_argv == NULL)
            {
                perror("Error allocating memory\n");
                free(cur_argv);
                break;
            }
            cur_argv[cur_argc - 1] = cur_token;
        }
        //Last element in argv array must be null
        cur_argv[cur_argc] = NULL;

        //Handle internal commands - cd, exit, jobs
        //TODO: finish this
        if (strcmp(cur_prog, "cd") == 0)
        {
            if(chdir(cur_argv[1]) == -1)
            {
                perror("chdir error");
            }
            continue;
        }
        else if (strcmp(cur_prog, "exit") == 0)
        {
            exit(0);
            continue;
        }
        else if (strcmp(cur_prog, "jobs") == 0)
        {
            continue;
        }

        //We have the program/file to execute and a list of arguments.. now we can fork and execute
        pid = fork();
        if (pid < 0)
        {
            perror("Error: Fork failure");
            continue;
        }
        else if (pid == 0) //Running in the child process, run command
        {
            execvp(cur_prog, cur_argv);
            //execvp only returns in the case of an error - errno will be set
            perror("Error");
            exit(0);
        }
        else //Running in the parent process, wait for child to complete
        {
            waitpid(pid, &child_status, 0);
        }

        //Free the argv array
        free(cur_argv);
    }

    return 0;
}