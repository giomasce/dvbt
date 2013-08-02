
#include <stdio.h>
#include <math.h>

#include "ofdm.h"
#include "tps.h"

int main() {

  //fftw_init_threads();
  //fftw_plan_with_nthreads(8);

  //FILE *fin = fopen("data", "r");

  SlidingWindow *sw = sw_new(stdin);
  OFDMContext *ctx = ofdm_context_new(76.5e6, 5760.0 / 224e-6, TRANS_MODE_2K, GUARD_INT_1_32, sw);

  sw_advance(sw, 5000);
  sw_reserve_front(sw, 3 * ctx->full_len + 5000);
  sw_reserve_back(sw, 5000);

  int i;
  double shift, value;
  double min_value = 1e9;
  int min_offset = 0;
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

  i = 0;
  TPSDecoder *tps_dec = tps_decoder_new();
  while (true) {
    sw_reserve_front(sw, 2 * ctx->full_len);
    sw_reserve_back(sw, 2 * ctx->full_len);
    ofdm_context_decode_symbol(ctx, 0);
    shift = ofdm_context_optimize_offset(ctx, 30.0, NULL);
    sw_advance(sw, ((int) round(shift)) + ctx->full_len);
    ofdm_context_shift_freqs(ctx, shift);
    //ofdm_context_dump_freqs(ctx, "dump");
    bool tps_bit = ofdm_context_read_tps_bit(ctx);
    bool tps_finished = tps_decoder_push_bit(tps_dec, tps_bit);

    if (i % 1 == 0) {
      fprintf(stderr, "> %d %d %f\n", i, sw->total_offset, shift);
    }
    if (tps_finished) {
      fprintf(stderr, "TPS finished!\n");
      fprintf(stderr, "Cell ID: %d\n", tps_dec->data.cell_id_byte);
      fprintf(stderr, "Transmission mode: %d\n", tps_dec->data.trans_mode);
      fprintf(stderr, "TPS length: %d\n", tps_dec->data.len);
      fprintf(stderr, "Frame num: %d\n", tps_dec->data.frame_num);
    }

    i++;
  }

  return 0;

}
