#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

const char DELIMS[] = " -\r\t\n./,";

int is_delimiter(char ch) {
  return strchr(DELIMS, ch) != 0;
}

void process_token(char *buf, int len, int valid_number) {
  if (len > 0 && valid_number) {
    buf[len] = '\0';
    int val = atoi(buf);
    if (val % 5 == 0 || val % 6 == 0) {
      printf("%d\n", val);
    }
  }
}

void parse_stream(int fd) {
  char buf[64];
  char numbuf[32];
  int idx = 0;
  int valid = 1;
  int n;

  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    for (int i = 0; i < n; i++) {
      char c = buf[i];

      if (is_delimiter(c)) {
        process_token(numbuf, idx, valid);
        idx = 0;
        valid = 1;
      } else if (c >= '0' && c <= '9') {
        if (idx < (int)sizeof(numbuf) - 1) {
          numbuf[idx++] = c;
        }
      } else {
        valid = 0;
      }
    }
  }

  process_token(numbuf, idx, valid);
}

int main(int argc, char *argv[]) {
  int i;
  int fd;

  if (argc <= 1) {
    parse_stream(0);
    exit(0);
  }

  for (i = 1; i < argc; i++) {
    fd = open(argv[i], 0);
    if (fd < 0) {
      fprintf(2, "sixfive: cannot open %s\n", argv[i]);
      exit(1);
    }
    parse_stream(fd);
    close(fd);
  }

  exit(0);
}
