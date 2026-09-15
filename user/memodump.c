#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data, int len);

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    printf("Example 1:\n");
    int a[2] = {61810, 2026};
    memdump("ii", (char *)a, sizeof(a));

    printf("Example 2:\n");
    memdump("S", "a string", sizeof("a string"));

    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *)&s, sizeof(s));

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;

    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");

    printf("Example 4:\n");
    memdump("pihcS", (char *)&example, sizeof(example));

    printf("Example 5:\n");
    memdump("sccccc", (char *)&example, sizeof(example));
  } else if (argc == 2) {
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while (n < sizeof(data)) {
      int nn = read(0, data + n, sizeof(data) - n);
      if (nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data, n);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

void
memdump(char *fmt, char *data, int len)
{
  int off = 0;
  for (char *f = fmt; *f; f++) {
    char c = *f;

    if (c == 'S') {
      int i = off;
      while (i < len && data[i] != 0) {
        write(1, data + i, 1);
        i++;
      }
      write(1, "\n", 1);
      return; // S consumes the rest of the data
    }

    int need = (c == 'i') ? 4 : (c == 'p' || c == 's') ? 8 :
               (c == 'h') ? 2 : (c == 'c') ? 1 : 0;

    if (need == 0) {
      printf("memdump: unknown format '%c'\n", c);
      return;
    }
    if (off + need > len) {
      printf("memdump: not enough data for '%c'\n", c);
      return;
    }

    if (c == 'i') {
      int v;
      memmove(&v, data + off, 4);
      printf("%d\n", v);
    } else if (c == 'p') {
      uint64 v;
      memmove(&v, data + off, 8);
      printf("%lx\n", v);
    } else if (c == 'h') {
      short v;
      memmove(&v, data + off, 2);
      printf("%d\n", v);
    } else if (c == 'c') {
      printf("%c\n", data[off]);
    } else if (c == 's') {
      char *p;
      memmove(&p, data + off, 8);
      printf("%s\n", p);
    }
    off += need;
  }
}
