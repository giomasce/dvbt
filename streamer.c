
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

int main(int argc, char **argv) {

  if (argc < 2) {
    fprintf(stderr, "Please provide a file name.\n");
    exit(1);
  }

  while (true) {
    FILE *fin = fopen(argv[1], "r");
    int i;
    for (i = 0; i < 4; i++) {
      while (fgetc(fin) != '\n') {}
    }
    int res;
    unsigned char tmp;
    while (true) {
      res = fscanf(fin, "%hhu", &tmp);
      if (res == 0 || res == EOF) {
        break;
      }
      fputc(tmp, stdout);
    }
    fclose(fin);
  }

  return 0;

}
