#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define READ 0
#define WRITE 1

int main (int argc, char *argv[])
{
    // CHECK: is user enter argument samw with 1?
    if (argc != 1)
    {
        fprintf(2, "USAGE: pingpong\n");
        exit(1);
    }

    int pid, p2c[2], c2p[2];
    char signal = 0;
    
    pipe(p2c);
    pipe(c2p);

    if ((pid = fork()) > 0)
    {
        close(p2c[READ]);
        close(c2p[WRITE]);

        write(p2c[WRITE], &signal, 1);
        close(p2c[WRITE]);

        read(c2p[READ], &signal, 1);
        close(c2p[READ]);

        printf("%d: Received Pong\n", getpid());
        
        exit(0);
    }
    else if (pid == 0)
    {
        close(c2p[READ]);
        close(p2c[WRITE]);

        read(p2c[READ], &signal, 1);
        close(p2c[READ]);

        printf("%d: Received Ping\n", getpid());
        
        write(c2p[WRITE], &signal, 1);
        close(c2p[WRITE]);

        exit(0);
    }
    else
    {
        fprintf(2, "fork error\n");
        exit(1);
    }
}

