#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

#define READ 0

#define MAX_ARG_LEN 512
// #define MAXARG 32

int main(int argc, char *argv[])
{
    if (argc < 2) 
    {
        fprintf(2, "missing command\n");
        exit(1);
    }

    char line[MAX_ARG_LEN];
    int index = 0;
    char ch;
    
    // read a char
    while (read(READ, &ch, 1) > 0) 
    {
        // end of a line
        if (ch == '\n') 
        {
            line[index] = '\0'; // add null terminator
            
            // a memory per argument
            char *cmd_argv[MAXARG];
            for (int i = 0; i < argc - 1; i++) 
            {
                cmd_argv[i] = malloc(strlen(argv[i+1]) + 1);
                strcpy(cmd_argv[i], argv[i+1]); 
            }

            // a block for input line
            cmd_argv[argc-1] = malloc(strlen(line) + 1);
            strcpy(cmd_argv[argc-1], line);
            cmd_argv[argc] = 0; // add end of array

            int pid = fork();
            if (pid == 0) // child process
            {
                exec(cmd_argv[0], cmd_argv); // exec(path, argv)
                fprintf(2, "exec %s failed\n", cmd_argv[0]); // if exec fails
                exit(1);
            } 
            else if (pid > 0) // parent process
            {
                wait(0);
                
                // free memory allocated
                for (int i = 0; i < argc; i++) 
                {
                    free(cmd_argv[i]);
                }
            }
            else // fork failed 
            {
                fprintf(2, "fork failed\n");
                exit(1);
            }
            index = 0;    
        } 
        else // common char
        {
            // check if the argument length is out of limit
            if (index < MAX_ARG_LEN - 1) 
            {
                line[index++] = ch;
            } 
            else // argument too long
            {
                fprintf(2, "argument too long\n");
                while (read(READ, &ch, 1) > 0 && ch != '\n');
                index = 0;
            }
        }
    }
    exit(0);
}