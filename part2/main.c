#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT_LENGTH 4096
#define DELIMS " \t\n\r"

char **getCurrentArgv(char **, int, int);

int main()
{
    char user_input[MAX_INPUT_LENGTH];
    char *saveptr;
    char *cur_token;
    char *cur_dir;
    char **cur_argv;
    char **orig_argv;
    int orig_argc;
    int pipe_count;
    int i;
    pid_t pid;
    int child_status;
    int isBackground;

    //Ignore control-c and control-z signal
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    //main loop
    while (1)
    {
        orig_argc = 0;
        isBackground = 0;
        pipe_count = 0;

        cur_dir = getcwd(NULL, 0);
        printf("%s$ ", cur_dir);
        fgets(user_input, MAX_INPUT_LENGTH, stdin);

        //Parse the string into tokens - first the program name
        //Program name must be the first argument in the arg array
        //If first token is NULL, then skip this iteration
        if (cur_token = strtok_r(user_input, DELIMS, &saveptr))
        {
            orig_argc++;
            orig_argv = malloc(sizeof(char *) * (orig_argc + 1));
            orig_argv[orig_argc - 1] = basename(cur_token);
        }
        else continue;

        //First argument must be the filename (req'd for execvp)
        //each subsequent token is an argument (until we see | or &)
        //Leaving an extra space in orig_argv array for NULL
        while (cur_token = strtok_r(NULL, DELIMS, &saveptr))
        {
            //Check if token specifies backgrounding (only if last token)
            if (strcmp(cur_token, "&") == 0) isBackground = 1;
            else isBackground = 0;

            if (strcmp(cur_token, "|") == 0) pipe_count++;

            orig_argc++;
            orig_argv = realloc(orig_argv, sizeof(char *) * (orig_argc));
            if (orig_argv == NULL)
            {
                perror("Error allocating memory\n");
                free(orig_argv);
                break;
            }
            orig_argv[orig_argc - 1] = cur_token;
        }
        //Last element in argv array must be null
        //If isBackground is true, overwrite the ending & with NULL
        if (isBackground == 1) orig_argv[orig_argc - 1] = NULL;
        else
        {
            orig_argv = realloc(orig_argv, sizeof(char *) * (orig_argc + 1));
            orig_argv[orig_argc] = NULL;
        }


        //Handle internal commands - cd, exit, jobs
        if (strcmp(orig_argv[0], "cd") == 0)
        {
            if(orig_argv[1] == NULL) printf("Error: No directory specified\n");
            else if (chdir(orig_argv[1]) == -1)
            {
                perror("chdir error");
            }
            continue;
        }
        else if (strcmp(orig_argv[0], "exit") == 0)
        {
            exit(0);
            continue;
        }
        else if (strcmp(orig_argv[0], "jobs") == 0)
        {
            printf("Need to add support for jobs command....\n");
            continue;
        }


        //Get current argument array
        cur_argv = getCurrentArgv(orig_argv, orig_argc, 0);

        //We have the program/file to execute and a list of arguments.. now we can fork and execute
        pid = fork();
        if (pid < 0)
        {
            perror("Error: Fork failure");
            continue;
        }
        else if (pid == 0) //Running in the child process, run command
        {
            execvp(cur_argv[0], cur_argv);
            //execvp only returns in the case of an error - errno will be set
            perror("Error");
            exit(0);
        }
        else //Running in the parent process, wait for child to complete if not running in background
        {
            if (isBackground == 0) waitpid(pid, &child_status, 0);
        }

        //Free necessary variables
        free(cur_argv);
        free(orig_argv);
        free(cur_dir);
    }

    return 0;
}

char **getCurrentArgv(char **orig_argv, int orig_argc, int pipe_index)
{
    //Pipe index indicates where we should be getting the cur_argv from
    //ex. orig_argv = ["ls", "-al", "|", "grep", "-i", "m", NULL]
    //With pipe_index = 0, return ["ls", "-al", NULL]
    //With pipe_index = 1, return ["grep", "-i", "m", NULL]

    char **cur_argv = NULL;
    int cur_argc = 0;

    int cur_pipe_count = 0;

    int i;
    for (i = 0; i < orig_argc; i++)
    {
        if (strcmp(orig_argv[i], "|") == 0)
        {
            cur_pipe_count++;
            continue;
        }

        if (cur_pipe_count == pipe_index)
        {
            cur_argc++;
            cur_argv = realloc(cur_argv, sizeof(char *) * (cur_argc));
            cur_argv[cur_argc - 1] = orig_argv[i];
        }
    }

    //NULL-terminate the array
    cur_argv = realloc(cur_argv, sizeof(char *) * (cur_argc + 1));
    cur_argv[cur_argc] = NULL;

    return cur_argv;
}