
#include <stdio.h>
#include <math.h>

#include "ofdm.h"

int main() {

  //FILE *fin = fopen("data", "r");

  SlidingWindow *sw = sw_new(stdin);
  OFDMContext *ctx = ofdm_context_new(76.5e6, 5760.0 / 224e-6, TRANS_MODE_2K, GUARD_INT_1_32, sw);

  sw_advance(sw, 5000);
  sw_reserve_front(sw, 3 * ctx->full_len + 5000);

  int i;
  double shift, value;
  double min_value = 1e9;
  int min_offset;
  for (i = 0; i < 2 * ctx->full_len; i+= 5000) {
    ofdm_context_decode_symbol(ctx, i);
    shift = ofdm_context_optimize_offset(ctx, 5000.0, &value);
    if (value < min_value) {
      min_value = value;
      min_offset = (int) round(i + shift);
    }
  }

  fprintf(stderr, "%f %d\n", min_value, min_offset);
  sw_advance(sw, min_offset);

  while (true) {
    sw_reserve_front(sw, 2 * ctx->full_len);
    sw_reserve_back(sw, 2 * ctx->full_len);
    ofdm_context_decode_symbol(ctx, 0);
    shift = ofdm_context_optimize_offset(ctx, 30.0, NULL);
    sw_advance(sw, ((int) round(shift)) + ctx->full_len);
    ofdm_context_shift_freqs(ctx, shift);
    fprintf(stderr, "%d\n", sw->total_offset);
  }

  return 0;

}
