
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DATA float
//#define DATA double

int main(int argc, char **argv) {

  if (argc != 3) {
    fprintf(stderr, "Usage: %s OUTPUT_SAMP_FREQ GEN_FREQ\n", argv[0]);
    exit(1);
  }

  char *endptr;
  double output_freq = strtod(argv[1], &endptr);
  if (endptr == argv[1]) {
    fprintf(stderr, "Invalid output sampling frequency\n");
    exit(1);
  }
  double gen_freq = strtod(argv[2], &endptr);
  if (endptr == argv[2]) {
    fprintf(stderr, "Invalid generated frequency\n");
    exit(1);
  }

  fprintf(stderr, "sizeof(DATA) = %lu\n", sizeof(DATA));

  unsigned long long int out_time = 0;
  for (;; out_time++) {
    DATA out = cos(2 * M_PI * ((double) out_time) / output_freq * gen_freq);
    size_t res = fwrite(&out, sizeof(DATA), 1, stdout);
    if (res == 0) goto end;
  }

 end:

  return 0;

}
