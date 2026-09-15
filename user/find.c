#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

char *
fmtname(char *path)
{
  static char buf[DIRSIZ + 1];
  char *p;
  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;
  if (strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)] = 0;
  return buf;
}

void
run_exec(char **execargv, int execargc, char *path)
{
  int pid = fork();
  if (pid < 0) {
    fprintf(2, "find: fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    execargv[execargc] = path;
    execargv[execargc + 1] = 0;
    exec(execargv[0], execargv);
    fprintf(2, "find: exec %s failed\n", execargv[0]);
    exit(1);
  }
  int st;
  wait(&st);
}

void
find(char *path, char *name, char **execargv, int execargc)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if (strcmp(fmtname(path), name) == 0) {
    if (execargv)
      run_exec(execargv, execargc, path);
    else
      printf("%s\n", path);
  }

  if (st.type == T_DIR) {
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
      printf("find: path too long\n");
      close(fd);
      return;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;
      if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf, name, execargv, execargc);
    }
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if (argc < 3) {
    fprintf(2, "Usage: find <path> <name> [-exec cmd args...]\n");
    exit(1);
  }

  char *path = argv[1];
  char *name = argv[2];
  char *execargv[MAXARG];
  int execargc = 0;
  int has_exec = 0;

  if (argc > 3) {
    if (strcmp(argv[3], "-exec") != 0) {
      fprintf(2, "find: unknown option %s\n", argv[3]);
      exit(1);
    }
    has_exec = 1;
    for (int i = 4; i < argc && execargc < MAXARG - 2; i++)
      execargv[execargc++] = argv[i];
  }

  find(path, name, has_exec ? execargv : 0, execargc);
  exit(0);
}
