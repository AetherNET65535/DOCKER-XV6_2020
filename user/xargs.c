#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
#define stdin 0
#define stdout 1
#define stderr 2
#define MAX_ARG_LEN 1024

int main (int argc, char* argv[])
{
  int pid;
  int readReturn;
  int index = 0;
  char temp;
  char argTemp[MAX_ARG_LEN];
  char *argFull[MAXARG];
  int escapeMode = 0;

  // init
  for (int i = 1; i < argc; i++)
  {
    argFull[i-1] = argv[i];
  }
  argFull[argc-1] = argTemp; // Temp that always change, like a pipe
  argFull[argc] = 0;

  while ((readReturn = read(stdin, &temp, 1)) > 0)
  {
    if (escapeMode)
    {
      escapeMode = 0;

      if (temp == 'n')
      {
        argTemp[index++] = '\n';
      }
      else if (temp == 't')
      {
        argTemp[index++] = '\t';
      }
      else if (temp == '\\')
      {
        argTemp[index++] = '\\';
      }
      else
      {
        argTemp[index++] = '\\';
        argTemp[index++] = temp;
      }
    }
    else if (temp == '\\')
    {
      escapeMode = 1;
    }
    else if (temp == '\n')
    {
      argTemp[index] = 0;
      pid = fork();
      if (pid == 0)
      {
        exec(argFull[0], argFull);
        fprintf(stderr, "EXEC ERROR\n");
        exit(0);
      }
      else if (pid > 0)
      {
        wait(0);
        index = 0;
        escapeMode = 0;
      }
      else
      {
        fprintf(stderr, "FORK ERROR\n");
        exit(1);
      }
    }
    else
    {
      argTemp[index++] = temp;
    }

    if (readReturn < 0)
    {
      fprintf(stderr, "READ ERROR\n");
    }
  }
  exit(0);
}
