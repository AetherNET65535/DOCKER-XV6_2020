#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define READ 0
#define WRITE 1

__attribute__((noreturn)) // to avoid warning about not returning
void sieve_algo (int left[2], int depth)
{
    close(left[WRITE]);

    int prime, temp, pid, right[2];

    // read a prime number from the pipe
    if (read(left[READ], &prime, sizeof(int)) == 0)
    {
        close(left[READ]);
        exit(0);
    }
    
    // if the depth is too deep, we have a stack overflow
    if (depth > 15)
    {
        fprintf(2, "stackoverflow\n");
        close(right[READ]);
        close(right[WRITE]);
        close(left[READ]);
        exit(1);
    }

    printf("prime: %d\n", prime);
    pipe(right);

    pid = fork();
    if (pid > 0) // parent process
    {
        close(right[READ]);

        while(read(left[READ], &temp, sizeof(int)))
        {
            if (temp % prime != 0)
            {
                if (write(right[WRITE], &temp, sizeof(int)) != sizeof(int))
                {
                    fprintf(2, "write error\n");
                }
            }
        }
        close(right[WRITE]);
        wait(0);
        exit(0);
    }
    else if (pid == 0) // child process
    {
        close(left[READ]);
        close(right[WRITE]);
        sieve_algo(right, depth + 1);
        exit(0);
    }
    else // fork failed
    {
        fprintf(2, "fork error\n");
        close(right[READ]);
        close(right[WRITE]);
        close(left[READ]);
        exit(1);
    }
}

int main(int argc, char* argv[])
{
    int pid, p[2];
    pipe(p);

    pid = fork();
    if (pid > 0) // parent process
    {
        close(p[READ]);
        
        // write 2 to 35 to the pipe
        for (int i = 2; i <= 35; i++)
        {
            write(p[WRITE], &i, sizeof(int));
        }
        
        // p finish working
        close(p[WRITE]);
        wait(0); 
        exit(0); 
    }
    else if (pid == 0) // child process
    {
        sieve_algo(p, 1);
        exit(0);
    }
    else // fork failed
    {
        fprintf(2, "fork error\n");
        exit(1);
    }
}