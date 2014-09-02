
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "ofdm.h"
#include "tps.h"
#include "debug.h"

int main() {

  //fftw_init_threads();
  //fftw_plan_with_nthreads(8);

  FILE *fin;
  //fin = fopen("real_data.i", "r");
  fin = stdin;

  double samp_freq;
  double mod_freq;
  TransMode trans_mode;
  GuardInt guard_int;

  /*samp_freq = 76.5e6;
  mod_freq = 5760.0 / 224e-6;
  trans_mode = TRANS_MODE_2K;
  guard_int = GUARD_INT_1_32;*/

  samp_freq = 20e6;
  mod_freq = 4e6;
  trans_mode = TRANS_MODE_2K;
  guard_int = GUARD_INT_1_32;

  SlidingWindow *sw = sw_new(fin);
  OFDMContext *ctx = ofdm_context_new(samp_freq, mod_freq, trans_mode, guard_int, sw);

  sw_advance(sw, 5000);
  sw_reserve_front(sw, 3 * ctx->full_len + 5000);
  sw_reserve_back(sw, 5000);

  char filename[1024];

  int i;
  double shift, value;
  double min_value = 1e9;
  int min_offset = 0;
  for (i = 0; i < 2 * ctx->full_len; i+= 5000) {
    ofdm_context_decode_symbol(ctx, i);
    /*snprintf(filename, 1024, "dump/debug_%06d", i);
    ofdm_context_dump_debug(ctx, filename);
    snprintf(filename, 1024, "dump/symbol_%06d", i);
    ofdm_context_dump_freqs(ctx, filename);
    snprintf(filename, 1024, "dump/gnuplot_%06d", i);
    ofdm_context_dump_gnuplot(ctx, filename);*/
    shift = ofdm_context_optimize_offset(ctx, 5000.0, &value);
    if (value < min_value) {
      min_value = value;
      min_offset = (int) round(i + shift);
    }
  }

  fprintf(stderr, "%f %d\n", min_value, min_offset);
  sw_advance(sw, min_offset);

  debug_dump_phase_diagrams(ctx, sw, -2000, 2000);
  exit(0);

  i = 0;
  TPSDecoder *tps_dec = tps_decoder_new();
  bool locked = false;
  while (true) {
    sw_reserve_front(sw, 2 * ctx->full_len);
    sw_reserve_back(sw, 2 * ctx->full_len);
    ctx->frame_offset++;
    ofdm_context_decode_symbol(ctx, 0);
    shift = ofdm_context_optimize_offset(ctx, 30.0, NULL);
    sw_advance(sw, ((int) round(shift)) + ctx->full_len);
    ofdm_context_shift_freqs(ctx, shift);
    snprintf(filename, 1024, "dump/symbol_%06d", i);
    ofdm_context_dump_freqs(ctx, filename);
    if (locked) {
      /*snprintf(filename, 1024, "dump/debug_%06d", i);
        ofdm_context_dump_debug(ctx, filename);*/
      ofdm_context_normalize_energy(ctx);
      ofdm_context_decode_bits(ctx);
      //exit(1);
    }
    bool tps_bit = ofdm_context_read_tps_bit(ctx);
    bool tps_finished = tps_decoder_push_bit(tps_dec, tps_bit);

    if (i % 1 == 0) {
      fprintf(stderr, "> %d %d %f %f\n", i, sw->total_offset, sw->total_offset / samp_freq, shift);
    }
    if (tps_finished) {
      fprintf(stderr, "TPS finished!\n");
      fprintf(stderr, "Cell ID: %d\n", tps_dec->data.cell_id_byte);
      fprintf(stderr, "Transmission mode: %d\n", tps_dec->data.trans_mode);
      fprintf(stderr, "TPS length: %d\n", tps_dec->data.len);
      fprintf(stderr, "Frame num: %d\n", tps_dec->data.frame_num);
      ctx->frame_offset = -1;
      ctx->constellation = tps_dec->data.constellation;
      ctx->hierarchy = tps_dec->data.hierarchy;
      locked = true;
    }

    i++;
  }

  return 0;

}
