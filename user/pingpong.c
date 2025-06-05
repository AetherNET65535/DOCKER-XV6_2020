#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define READ 0
#define WRITE 1

int main (int argc, char *argv[])
{
    if (argc != 1)
    {
        fprintf(2, "usage: pingpong\n");
        exit(1);
    }

    int pid;

    // one-way pipes for communication
    // p2c: parent to child
    // c2p: child to parent
    int p2c[2], c2p[2]; 
    char signal = 0;
    
    // create pipes
    pipe(p2c); 
    pipe(c2p);

    pid = fork();

    if (pid > 0) // parent process
    {
        // parent process just needs to write[p2c] and read[c2p]
        close(p2c[READ]);
        close(c2p[WRITE]);

        // tee off: send a byte signal to child
        write(p2c[WRITE], &signal, 1);
        close(p2c[WRITE]);

        // waiting for catch: blocking until child sends a byte signal
        read(c2p[READ], &signal, 1);
        close(c2p[READ]);

        printf("%d: Received Pong\n", getpid());
        
        exit(0);
    }
    else if (pid == 0) // child process
    {
        // child process just needs to read[p2c] and write[c2p]
        close(c2p[READ]);
        close(p2c[WRITE]);

        // waiting for catch: blocking until parent sends a byte signal
        read(p2c[READ], &signal, 1);
        close(p2c[READ]);

        printf("%d: Received Ping\n", getpid());
        
        // tee off: send a byte signal to parent
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