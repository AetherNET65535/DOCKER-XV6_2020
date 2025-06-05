#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *
fmtname(char *path)
{
  char *p;

  // find the last '/' in the path
  for(p = path + strlen(path); p >= path && *p != '/'; --p)
    ;
  return p + 1;
}

void
find(char *path, char *filename)
{
  char buf[512], *p;
  int fd;
  struct stat st;
  struct dirent de;

  // get the file descriptor
  if((fd = open (path, 0)) < 0)
  {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  // get the file stats
  if(fstat (fd, &st) < 0)
  {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type)
  {
    case T_FILE: // if it is a file
      // check if the file name matches the given filename
      if(strcmp(fmtname(path), filename) == 0)
      {
        printf("%s\n", path);
      }
      break;

    case T_DIR: // if it is a directory
      strcpy(buf, path);
      p = buf + strlen(buf);
      *p++ = '/';
      
      // read the whole directory, until the end of the directory
      while(read(fd, &de, sizeof(de)) == sizeof(de))
      {
        if(de.inum == 0 || strcmp(de.name, ".") == 0
          || strcmp(de.name, "..") == 0)
          continue;

        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        find(buf, filename); // recursive call to find in subdirectories
      }
      break;

    default:
      break;
  }
  close (fd);
}

int
main(int argc, char *argv[])
{
  if(argc < 3)
  {
    fprintf(2, "usage: find <start_path> <file_name>\n");
    exit(0);
  }
  find(argv[1], argv[2]);
  exit(0);
}