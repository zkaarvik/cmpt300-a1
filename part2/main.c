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

typedef struct
{
    pid_t pid;
    char *cmd_string;
} command;

char **getCurrentArgv(char **, int, int);
void close_pipes(int *, int);
void handle_SIGINT();
void print_job_list(command *, int);
void free_cmd_array(command *, int);

int main()
{
    char user_input[MAX_INPUT_LENGTH];
    char cur_dir[MAX_PATH_LENGTH];
    char *cur_cmd_string;
    char *saveptr;
    char *cur_token;
    char **cur_argv;
    char **orig_argv;
    char **temp_argv;
    int orig_argc;
    int pipe_count;
    int i;
    int child_status;
    int isBackground;
    int *pipefd;
    int cmd_array_length = 0;
    pid_t pid;
    command *cmd_array = NULL;

    //Ignore control-c and control-z signal in the main process
    signal(SIGINT, SIG_IGN); //ctrl-c
    signal(SIGTSTP, SIG_IGN); //ctrl-z

    //main loop
    while (1)
    {
        orig_argc = 0;
        isBackground = 0;
        pipe_count = 0;

        getcwd(cur_dir, MAX_PATH_LENGTH);

        printf("%s$ ", cur_dir);
        if ( fgets(user_input, MAX_INPUT_LENGTH, stdin) == NULL )
        {
            //EOF encountered, ctrl d pressed
            free_cmd_array(cmd_array, cmd_array_length);
            printf("\n");
            exit(0);
        }

        //Keep the original command string - needs to be saved into cmd_array if process is backgrounded
        cur_cmd_string = strdup(user_input);

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
            temp_argv = realloc(orig_argv, sizeof(char *) * (orig_argc));
            if (temp_argv == NULL)
            {
                perror("Error allocating memory\n");
                free(orig_argv);
                break;
            }
            else orig_argv = temp_argv;
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
            free(orig_argv);
            free_cmd_array(cmd_array, cmd_array_length);
            exit(0);
            continue;
        }
        else if (strcmp(orig_argv[0], "jobs") == 0)
        {
            print_job_list(cmd_array, cmd_array_length);
            continue;
        }


        //Need a pipefd array as large as pipe_count * 2
        if (pipe_count > 0)
        {
            pipefd = malloc(2 * pipe_count * sizeof(int *));
            for (i = 0; i < pipe_count; i++)
            {
                if (pipe(pipefd + (i * 2)) == -1)
                {
                    perror("Pipe error");
                }
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
                    free(orig_argv);
                    free_cmd_array(cmd_array, cmd_array_length);
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
                    free(pipefd);
                    free(orig_argv);
                    free_cmd_array(cmd_array, cmd_array_length);
                    execvp(cur_argv[0], cur_argv);
                    perror("Error");
                    exit(1);
                }
                else
                {
                    free(cur_argv);
                    continue;
                }
            }

            //Last command
            //Input comes from previous command
            else if (i == pipe_count)
            {
                if (pid == 0)
                {
                    dup2(pipefd[((i - 1) * 2) + PIPE_READ], STDIN_FILENO);

                    close_pipes(pipefd, pipe_count);
                    free(pipefd);
                    free(orig_argv);
                    free_cmd_array(cmd_array, cmd_array_length);
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
                    free(pipefd);
                    free(orig_argv);
                    free_cmd_array(cmd_array, cmd_array_length);
                    execvp(cur_argv[0], cur_argv);
                    perror("Error");
                    exit(1);
                }
                else
                {
                    free(cur_argv);
                    continue;
                }
            }
        }

        //Parent process, wait for child to complete if not running in background
        if (pid > 0)
        {
            close_pipes(pipefd, pipe_count);

            if (isBackground == 0) waitpid(pid, &child_status, 0);
            else
            {
                //Add to command array
                cmd_array_length++;
                int size = cmd_array_length * sizeof(command);
                cmd_array = realloc(cmd_array, size);
                cmd_array[cmd_array_length - 1].pid = pid;
                cmd_array[cmd_array_length - 1].cmd_string = cur_cmd_string;
            }
        }
        else if (pid < 0)
        {
            perror("Error: Fork failure");
            continue;
        }

        //Free necessary variables
        if (pipe_count > 0) free(pipefd);
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
    char **temp_argv = NULL;
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
            temp_argv = realloc(cur_argv, sizeof(char *) * (cur_argc));
            if (temp_argv == NULL)
            {
                perror("Error allocating memory\n");
                free(cur_argv);
                return NULL;
            }
            cur_argv = temp_argv;
            cur_argv[cur_argc - 1] = orig_argv[i];
        }
    }

    //NULL-terminate the array
    temp_argv = realloc(cur_argv, sizeof(char *) * (cur_argc + 1));
    if (temp_argv == NULL)
    {
        perror("Error allocating memory\n");
        free(cur_argv);
        return NULL;
    }
    cur_argv = temp_argv;
    cur_argv[cur_argc] = NULL;

    return cur_argv;
}

void close_pipes(int *pipefd, int pipe_count)
{
    int i;

    for (i = 0; i < pipe_count * 2; i++)
    {
        close(pipefd[i]);
    }
}

void print_job_list(command *cmd_array, int cmd_array_length)
{
    int i, child_status;

    if (cmd_array == NULL) return;

    for (i = 0; i < cmd_array_length; i++)
    {
        if (waitpid(cmd_array[i].pid, &child_status, WNOHANG) == 0)
        {
            printf("[%d] %d \t %s", i, cmd_array[i].pid, cmd_array[i].cmd_string);
        }
    }

}

void free_cmd_array(command *cmd_array, int cmd_array_length)
{
    int i;

    if (cmd_array == NULL) return;

    for (i = 0; i < cmd_array_length; i++)
    {
        free(cmd_array[i].cmd_string);
    }

    free(cmd_array);
}