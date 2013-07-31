
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ofdm.h"

#include "data.h"

OFDMContext *ofdm_context_new(double samp_freq, double mod_freq, TransMode trans_mode, GuardInt guard_int, SlidingWindow *sw) {

  OFDMContext *ctx = malloc(sizeof(OFDMContext));

  ctx->samp_freq = samp_freq;
  ctx->mod_freq = mod_freq;
  ctx->trans_mode = trans_mode;
  ctx->guard_int = guard_int;

  ctx->packet_time = PACKET_TIME[trans_mode];
  ctx->guard_time = ctx->packet_time * GUARD_INT_RATIO[guard_int];
  ctx->carrier_num = CARRIER_NUM[trans_mode];
  ctx->central_carrier = (ctx->carrier_num - 1) / 2;
  ctx->packet_len = (int) round(ctx->samp_freq * ctx->packet_time);
  ctx->full_len = (int) round(ctx->samp_freq * (ctx->packet_time + ctx->guard_time));
  ctx->central_idx = (int) round(ctx->mod_freq * ctx->packet_time);
  ctx->lower_idx = ctx->central_idx - ctx->central_carrier;
  ctx->higher_idx = ctx->central_idx + ctx->central_carrier;

  ctx->signal = fftw_malloc(sizeof(double) * ctx->packet_len);
  ctx->freqs = fftw_malloc(sizeof(complex) * ((ctx->packet_len + 1) / 2));

  ctx->fft_forward_plan = fftw_plan_dft_r2c_1d(ctx->packet_len, ctx->signal, ctx->freqs, 0);
  ctx->fft_backward_plan = fftw_plan_dft_c2r_1d(ctx->packet_len, ctx->freqs, ctx->signal, 0);

  ctx->sw = sw;

  return ctx;

}

void ofdm_context_destroy(OFDMContext *ctx) {

  fftw_free(ctx->signal);
  fftw_free(ctx->freqs);
  fftw_destroy_plan(ctx->fft_forward_plan);
  fftw_destroy_plan(ctx->fft_backward_plan);
  free(ctx);

}

void ofdm_context_decode_symbol(OFDMContext *ctx) {

  int i;
  for (i = 0; i < ctx->packet_len; i++) {
    ctx->signal[i] = 255.0 / ((double) ctx->sw->ref[i]);
  }
  fftw_execute(ctx->fft_forward_plan);

}

double optimize_offset(OFDMContext *ctx, double half_width) {

  double value = 0.0;
  double derivative = 0.0;
  double position = -half_width;
  Heap *heap = heap_new(CONTINUAL_PILOTS_LEN[ctx->trans_mode]);

  size_t i;
  for (i = 0; i < CONTINUAL_PILOTS_LEN[ctx->trans_mode]; i++) {
    int carrier = CONTINUAL_PILOTS[ctx->trans_mode][i];
    int idx = ctx->lower_idx + carrier;
    double phase = carg(ctx->freqs[idx]);
    // unfinished
  }

}
