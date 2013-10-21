
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DATA float
//#define DATA double

int main(int argc, char **argv) {

  if (argc != 4) {
    fprintf(stderr, "Usage: %s INPUT_SAMP_FREQ OUTPUT_SAMP_FREQ UP_FREQ\n", argv[0]);
    exit(1);
  }

  char *endptr;
  double input_freq = strtod(argv[1], &endptr);
  if (endptr == argv[1]) {
    fprintf(stderr, "Invalid input sampling frequency\n");
    exit(1);
  }
  double output_freq = strtod(argv[2], &endptr);
  if (endptr == argv[2]) {
    fprintf(stderr, "Invalid output sampling frequency\n");
    exit(1);
  }
  double up_freq = strtod(argv[3], &endptr);
  if (endptr == argv[3]) {
    fprintf(stderr, "Invalid upconverting frequency\n");
    exit(1);
  }

  fprintf(stderr, "sizeof(DATA) = %lu\n", sizeof(DATA));

  unsigned long long int out_time = 0;
  unsigned long long int in_time = 0;
  DATA in1 = 0.0, in2;
  size_t res = fread(&in2, sizeof(DATA), 1, stdin);
  if (res == 0) goto end;
  for (;; out_time++) {
    double target_in_time = ((double) out_time) / output_freq * input_freq;
    while (target_in_time > ((double) in_time)) {
      in1 = in2;
      res = fread(&in2, sizeof(DATA), 1, stdin);
      if (res == 0) goto end;
      in_time++;
    }
    double offset = ((double) in_time) - target_in_time;
    DATA out = in1 * offset + in2 * (1.0 - offset);
    out *= cos(2 * M_PI * ((double) out_time) / output_freq * up_freq);
    res = fwrite(&out, sizeof(DATA), 1, stdout);
    if (res == 0) goto end;
  }

 end:

  return 0;

}
