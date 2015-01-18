#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_LENGTH 4096

int main()
{
    char user_input[MAX_INPUT_LENGTH];
    char *saveptr;
    char *current_token;

    //main loop
    while (1)
    {

        printf("zash$ ");
        fgets(user_input, MAX_INPUT_LENGTH, stdin);

        //Parse the string into tokens - first the program name
        if (current_token = strtok_r(user_input, " ", &saveptr))
        {
            printf("program name: %s \n", current_token);
            //TODO: Save to first element of an input array
        }
        else continue;

        //Then each subsequent token is an argument
        while (current_token = strtok_r(NULL, " ", &saveptr))
        {
            printf("input: %s \n", current_token);
        }


    }

    return 0;
}