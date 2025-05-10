#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define READ 0
#define WRITE 1

__attribute__((noreturn))
void sieve_algo (int left[2], int depth)
{
    close(left[WRITE]);

    int prime, temp, pid, right[2];

    // if cant read any number...QAQ
    if (read(left[READ], &prime, sizeof(int)) == 0)
    {
        close(left[READ]);
        exit(0);
    }
    
    // if too deep (prob will stackoverflow, kinda useless func, but just add, 保险起见)
    if (depth > 16) // 15 is enough, but 16 is more romance
    {
        fprintf(2, "my_stackoverflow(not real, but have error, cuz too deep)\n");
        close(right[READ]);
        close(right[WRITE]);
        close(left[READ]);
        exit(1);
    }

    printf("prime: %d\n", prime);
    pipe(right);

    if ((pid = fork()) > 0)
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
    else if (pid == 0)
    {
        close(left[READ]);
        close(right[WRITE]);
        sieve_algo(right, depth + 1);
        exit(0);
    }
    else
    {
        fprintf(2, "fork error...\n");
        close(right[READ]);
        close(right[WRITE]);
        close(left[READ]);
        exit(1);
    }
}

int main(int argc, char* argv[])
{
    int pid, p[2];
    pipe(p); // give 'p' a acess to this p (prob kernel will give him, idk)

    if ((pid = fork()) > 0) // parent
    {
        close(p[READ]);

        for (int i = 2; i <= 35; i++)
        {
            write(p[WRITE], &i, sizeof(int));
        }

        close(p[WRITE]);
        wait(0); // wait child kill himself
        exit(0); // kys
    }
    else if (pid == 0) // child
    {
        sieve_algo(p, 1);
        exit(0);
    }
    else // 5000% have error
    {
        fprintf(2, "fork error\n");
        exit(1);
    }
}
