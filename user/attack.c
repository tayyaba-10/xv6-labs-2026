#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  char *p;
  char *marker = "very very very secret pw is:";
  int i, j, match;

  for(i = 0; i < 40; i++){
    p = sbrk(4096);
    match = 1;
    for(j = 0; j < 10; j++){
      if(p[8 + j] != marker[j]){
        match = 0;
        break;
      }
    }
    if(match){
      write(2, p + 32, 8);
      exit(0);
    }
  }
  exit(1);
}
