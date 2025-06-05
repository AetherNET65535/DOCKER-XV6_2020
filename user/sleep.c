#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main (int argc, char *argv[])
{
    // if the argument is not 2 (sleep <ticks>)
    if (argc != 2)
    {
        fprintf (2, "usage: sleep <ticks>\n");
        exit(1);
    }
    
    // change ascii to integer
    int ticks = atoi(argv[1]);

    // turn back time: disabled
    if (ticks < 0)
    {
        fprintf (2, "ticks must be a positive integer\n");
        exit(1);
    }

    sleep(ticks);

    exit(0);
}