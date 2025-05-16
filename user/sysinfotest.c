#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

#define ERROR_CODE ((uint64)-1)

void sinfo(struct sysinfo* info)
{
    if(sysinfo(info) < 0)
    {
        printf("FAIL: SYSINFO FAILED\n");
        exit(1);
    }
}

uint64 countfree()
{
    uint sz0 = (uint64)sbrk(0);
    struct sysinfo info; // like a rice box, kernel will put rice in here
    
    uint count = 0;

    while(1)
    {
        if((uint64)sbrk(PGSIZE) == ERROR_CODE) // infinite loop, until kernel say no
        {
            break;
        }
        count++;
    }
    
    sinfo(&info);
    
    if(info.freemem != 0)
    {
        printf("FAIL: SHOULD BE NO FREE MEMORY, BUT NOW STILL HAVE %ld\n", info.freemem);
        exit(1);
    }

    sbrk(-((uint64)sbrk(0) - sz0)); // clear memory
    return count * PGSIZE;
}

void testmem()
{
    struct sysinfo info; // rice box
    uint64 expect_free = countfree();

    sinfo(&info); // ask cafeteria how much memory & process left

    if(info.freemem != expect_free) // 
    {
        printf("FAIL: EXPECT FREE = %ld, BUT NOW FREE = %ld\n", expect_free, info.freemem);
        exit(1);
    }

    if((uint64)sbrk(PGSIZE) == ERROR_CODE)
    {
        printf("FAIL: SBRK RELEASE JUST NOW FAILED\n");
        exit(1);
    }

    sinfo(&info); // ask again

    if(info.freemem != expect_free - PGSIZE)
    {
        printf("FAIL: EXPECT FREE = %ld, BUT NOW FREE = %ld\n", expect_free, info.freemem);
        exit(1);
    }

    if((uint64)sbrk(-PGSIZE) == ERROR_CODE)
    {
        printf("FAIL: 5000%% WONT PRINT THIS, TOO HARD");
        exit(1);
    }

    sinfo(&info);

    if(info.freemem != expect_free)
    {
        printf("FAIL: EXPECT FREE = %ld, BUT NOW FREE = %ld\n", expect_free, info.freemem);
        exit(1);
    }
}

void testcall()
{
    struct sysinfo info;

    if(sysinfo(&info) < 0)
    {
        printf("FAIL: SYSINFO FAILED\n");
        exit(1);
    }
    
    // imposible address for user to visit
    if(sysinfo((struct sysinfo*)0xeaeb0b5b00002f5e) != ERROR_CODE) 
    {
        printf("FAIL: BUT PRO, HOW BRO VISIT THIS ADDRESS :0\n");
        exit(1);
    }
}

void testproc()
{
    struct sysinfo info;
    uint64 nproc;
    int pid;

    sinfo(&info);
    nproc = info.nproc;

    pid = fork();
    if(pid == 0)
    {
        sinfo(&info);
        
        if(info.nproc != nproc + 1)
        {
            printf("CHILD FAIL: SHOULD BE %ld, NOT %ld\n", nproc, info.nproc);
            exit(1);
        }
        exit(0);
    }

    if(pid < 0)
    {
        printf("FAIL: FORK FAILED\n");
        exit(1);
    }

    wait(0);
    sinfo(&info);
    if(info.nproc != nproc)
    {
        printf("CHILD FAIL: SHOULD BE %ld, NOT %ld\n", nproc, info.nproc);
        exit(1);
    }
}

int main(int argc, char* argv[])
{
    printf("SYSINFOTEST: START\n");
    testcall();
    testmem();
    testproc();
    printf("SYSINFOTEST: OK\n");
    exit(0);
}
