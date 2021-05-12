#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void reverse_copy(char *src, char *dest, size_t size) {
  char *sbeg = src;
  char *send = src + size;
  while (send != sbeg - 1) {
    *dest = *send;
    dest++;
    send--;
  }
}

int main(int argc, char *argv[]) {
  char s[20];
  reverse_copy("abcd", s, 3);
  printf("%s\n", s);
  return 0;
}