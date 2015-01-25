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
#define MAX_PATH_LENGTH 1024
#define DELIMS " \t\n\r"
#define PIPE_READ 0
#define PIPE_WRITE 1

char **getCurrentArgv(char **, int, int);
void close_pipes(int *, int);

int main()
{
    char user_input[MAX_INPUT_LENGTH];
    char *saveptr;
    char *cur_token;
    char cur_dir[MAX_PATH_LENGTH];
    char **cur_argv;
    char **orig_argv;
    int orig_argc;
    int pipe_count;
    int i;
    pid_t pid;
    int child_status;
    int isBackground;
    int *pipefd;

    //Ignore control-c and control-z signal
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    //main loop
    while (1)
    {
        orig_argc = 0;
        isBackground = 0;
        pipe_count = 0;

        getcwd(cur_dir, MAX_PATH_LENGTH);

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
            if (strcmp(cur_token, "&") == 0)
            {
                isBackground = 1;
                break;
            }

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
        orig_argv = realloc(orig_argv, sizeof(char *) * (orig_argc + 1));
        orig_argv[orig_argc] = NULL;

        //Handle internal commands - cd, exit, jobs
        if (strcmp(orig_argv[0], "cd") == 0)
        {
            if (orig_argv[1] == NULL) printf("Error: No directory specified\n");
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


        //Need a pipefd array as large as pipe_count * 2
        if (pipe_count > 0)
        {
            pipefd = malloc(2 * pipe_count * sizeof(int *));
            for (i = 0; i < pipe_count; i++)
            {
                pipe(pipefd + (i * 2));
            }
        }
        //Execute commands with respect to piping
        for (i = 0; i <= pipe_count; i++)
        {
            cur_argv = getCurrentArgv(orig_argv, orig_argc, i);

            pid = fork();

            //Single command, no piping
            if (pipe_count == 0)
            {
                if (pid == 0)
                {
                    execvp(cur_argv[0], cur_argv);
                    perror("Error");
                    exit(1);
                }
                else continue;
            }

            //First command
            //Output goes to next command
            else if (i == 0)
            {
                if (pid == 0)
                {
                    dup2(pipefd[(i * 2) + PIPE_WRITE], STDOUT_FILENO);
                    
                    close_pipes(pipefd, pipe_count);
                    execvp(cur_argv[0], cur_argv);
                    perror("Error");
                    exit(1);
                }
                else continue;
            }

            //Last command
            //Input comes from previous command
            else if (i == pipe_count)
            {
                if (pid == 0)
                {
                    dup2(pipefd[((i - 1) * 2) + PIPE_READ], STDIN_FILENO);

                    close_pipes(pipefd, pipe_count);
                    execvp(cur_argv[0], cur_argv);
                    perror("Error");
                    exit(1);
                }
                else continue;
            }

            //Middle commands
            //Input comes from previous command and output goes to next command
            else
            {
                if (pid == 0)
                {
                    dup2(pipefd[((i - 1) * 2) + PIPE_READ], STDIN_FILENO);
                    dup2(pipefd[(i * 2) + PIPE_WRITE], STDOUT_FILENO);
                    
                    close_pipes(pipefd, pipe_count);
                    execvp(cur_argv[0], cur_argv);
                    perror("Error");
                    exit(1);
                }
                else continue;
            }

        }

        //Parent process, wait for child to complete if not running in background
        if (pid > 0)
        {
            close_pipes(pipefd, pipe_count);

            if (isBackground == 0) waitpid(pid, &child_status, 0);
            //if (isBackground == 0) wait(NULL);
        }
        else if (pid < 0)
        {
            perror("Error: Fork failure");
            continue;
        }

        //Free necessary variables
        if(pipe_count > 0) free(pipefd);
        free(cur_argv);
        free(orig_argv);
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

void close_pipes(int *pipefd, int pipe_count)
{
    int i;

    for (i = 0; i < pipe_count*2; i++)
    {
        close(pipefd[i]);
    }
}