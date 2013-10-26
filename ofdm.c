
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "ofdm.h"

#include "prbs.h"

OFDMContext *ofdm_context_new(double samp_freq, double mod_freq, TransMode trans_mode, GuardInt guard_int, SlidingWindow *sw) {

  build_pilot_sequence();

  OFDMContext *ctx = malloc(sizeof(OFDMContext));

  ctx->samp_freq = samp_freq;
  ctx->mod_freq = mod_freq;
  ctx->trans_mode = trans_mode;
  ctx->guard_int = guard_int;
  ctx->constellation = 0;
  ctx->hierarchy = 0;

  ctx->packet_time = PACKET_TIME[trans_mode];
  ctx->guard_time = ctx->packet_time * GUARD_INT_RATIO[guard_int];
  ctx->carrier_num = CARRIER_NUM[trans_mode];
  ctx->central_carrier = (ctx->carrier_num - 1) / 2;
  ctx->packet_len = (int) round(ctx->samp_freq * ctx->packet_time);
  ctx->full_len = (int) round(ctx->samp_freq * (ctx->packet_time + ctx->guard_time));
  ctx->central_idx = (int) round(ctx->mod_freq * ctx->packet_time);
  ctx->lower_idx = ctx->central_idx - ctx->central_carrier;
  ctx->higher_idx = ctx->central_idx + ctx->central_carrier;

  ctx->frame_offset = 0;

  ctx->signal = fftw_malloc(sizeof(double) * ctx->packet_len);
  ctx->freqs = fftw_malloc(sizeof(double complex) * ((ctx->packet_len + 1) / 2));
  ctx->bits = malloc(sizeof(uint8_t) * USEFUL_CARRIERS_NUM[ctx->trans_mode]);

  unsigned int flags = FFTW_MEASURE | FFTW_DESTROY_INPUT;
  flags = FFTW_ESTIMATE | FFTW_DESTROY_INPUT;
  ctx->fft_forward_plan = fftw_plan_dft_r2c_1d(ctx->packet_len, ctx->signal, ctx->freqs, flags);
  ctx->fft_backward_plan = fftw_plan_dft_c2r_1d(ctx->packet_len, ctx->freqs, ctx->signal, flags);

  ctx->sw = sw;

  return ctx;

}

void ofdm_context_destroy(OFDMContext *ctx) {

  fftw_free(ctx->signal);
  fftw_free(ctx->freqs);
  free(ctx->bits);
  fftw_destroy_plan(ctx->fft_forward_plan);
  fftw_destroy_plan(ctx->fft_backward_plan);
  free(ctx);

}

void ofdm_context_decode_symbol(OFDMContext *ctx, size_t offset) {

  int i;
  for (i = 0; i < ctx->packet_len; i++) {
    uint8_t sample = ctx->sw->ref[i+offset];
    ctx->signal[i] = ((double) sample) / 255.0;
  }
  fftw_execute(ctx->fft_forward_plan);

}

double last_before(double start, double step, double ref) {

  return start + step * floor((ref - start) / step);

}

/* Struct to keep side data when working with the heap in
   optimize_offset(). */
typedef struct {
  double step;
  int change;
} OptimizeOffsetHeapSatellite;

double ofdm_context_optimize_offset(OFDMContext *ctx, double half_width, double *min_value) {

  double value = 0.0;
  double derivative = 0.0;
  double position = -half_width;
  Heap *heap = heap_new(CONTINUAL_PILOTS_LEN[ctx->trans_mode]);

  size_t i;
  for (i = 0; i < CONTINUAL_PILOTS_LEN[ctx->trans_mode]; i++) {
    int carrier = CONTINUAL_PILOTS[ctx->trans_mode][i];
    int idx = ctx->lower_idx + carrier;
    double phase = carg(ctx->freqs[idx]);
    double target_phase = bv_get(pilot_sequence, carrier) ? 0.0 : M_PI;
    double freq = idx / ctx->packet_time;
    double points_start = (target_phase - phase) / (2 * M_PI * freq) * ctx->samp_freq;
    double step = ctx->samp_freq / freq;
    double peaks_start = points_start - step / 2;
    double last_point = last_before(points_start, step, position);
    double last_peak = last_before(peaks_start, step, position);
    if (last_point > last_peak) {
      derivative += 1.0;
      value += position - last_point;
      OptimizeOffsetHeapSatellite *sat = malloc(sizeof(OptimizeOffsetHeapSatellite));
      sat->step = step;
      sat->change = -1;
      heap_push(heap, last_point + step / 2, sat);
    } else {
      derivative -= 1.0;
      value += step / 2 - (position - last_peak);
      OptimizeOffsetHeapSatellite *sat = malloc(sizeof(OptimizeOffsetHeapSatellite));
      sat->step = step;
      sat->change = 1;
      heap_push(heap, last_peak + step / 2, sat);
    }
  }

  double min_value2 = value;
  double min_pos = position;

  while (position < half_width) {
    double next_position, step, change;
    OptimizeOffsetHeapSatellite *sat;
    heap_top(heap, &next_position, (void**) &sat);
    heap_pop(heap);
    step = sat->step;
    change = sat->change;
    next_position = fmin(next_position, half_width);
    value += (next_position - position) * derivative;
    derivative += 2 * change;
    if (value < min_value2) {
      min_value2 = value;
      min_pos = position;
    }
    position = next_position;
    sat->change = -sat->change;
    heap_push(heap, next_position + step / 2, sat);
  }

  /* Deallocate OptimizeOffsetHeapSatellite instances */
  for (i = 0; i < CONTINUAL_PILOTS_LEN[ctx->trans_mode]; i++) {
    free(heap->elements[i].ptr);
  }

  if (min_value != NULL) {
    *min_value = min_value2;
  }

  return min_pos;

}

void ofdm_context_shift_freqs(OFDMContext *ctx, double samples) {

  int idx;
  for (idx = ctx->lower_idx; idx <= ctx->higher_idx; idx++) {
    double real_freq = idx / ctx->packet_time;
    double time = samples / ctx->samp_freq;
    ctx->freqs[idx] *= cexp(2 * M_PI * I * real_freq * time);
  }

}

bool ofdm_context_read_tps_bit(OFDMContext *ctx) {

  uint16_t votes[2];
  votes[0] = 0;
  votes[1] = 0;
  int i;
  for (i = 0; i < TPS_CARRIERS_LEN[ctx->trans_mode]; i++) {
    uint16_t carrier = TPS_CARRIERS[ctx->trans_mode][i];
    uint8_t read_bit = !!(creal(ctx->freqs[ctx->lower_idx+carrier]) > 0.0);
    uint8_t inverted_carrier = !!bv_get(pilot_sequence, carrier);
    votes[read_bit ^ inverted_carrier]++;
  }

  return votes[1] > votes[0];

}

void ofdm_context_dump_freqs(OFDMContext *ctx, char *filename) {

  FILE *fout = fopen(filename, "w");
  int idx;
  for (idx = ctx->lower_idx; idx <= ctx->higher_idx; idx++) {
    fprintf(fout, "%f %f\n", creal(ctx->freqs[idx]), cimag(ctx->freqs[idx]));
  }
  fclose(fout);

}

/* A better approach would be normalize every subcarrier only taking
   care of the pilots near it, in order to compensate for different
   attenuation on different parts of the spectrum. */
void ofdm_context_normalize_energy(OFDMContext *ctx) {

  /* Compute average energy */
  size_t i;
  double energy = 0.0;
  for (i = 0; i < CONTINUAL_PILOTS_LEN[ctx->trans_mode]; i++) {
    int carrier = CONTINUAL_PILOTS[ctx->trans_mode][i];
    int idx = ctx->lower_idx + carrier;
    energy += csqabs(ctx->freqs[idx]);
  }
  energy /= CONTINUAL_PILOTS_LEN[ctx->trans_mode];
  double factor = (3.0 / 4.0) / sqrt(energy);

  /* Normalize everything */
  int idx;
  for (idx = ctx->lower_idx; idx <= ctx->higher_idx; idx++) {
    ctx->freqs[idx] *= factor;
  }

}

void build_pilot_sequence() {

  if (pilot_sequence != NULL) {
    return;
  }

  uint32_t taps = (1 << 10) | (1 << 8);
  uint32_t state = 0x7ff;
  uint8_t length = 11;

  size_t pilot_sequence_len = (MAX_CARRIER_NUM / 8) + 1;
  pilot_sequence = malloc(sizeof(uint8_t) * pilot_sequence_len);

  int i;
  for (i = 0; i < MAX_CARRIER_NUM; i++) {
    uint32_t bit = prbs_gen(&state, taps, length);
    bv_set(pilot_sequence, i, bit);
  }

}

inline uint8_t decode_semiaxis(double x, Constellation c, Hierarchy h) {

  /* Decode the offset */
  uint8_t res = ((uint8_t) floor((x - (START_COORDINATE[h] - 1)) / 2.0));

  /* Handle overflow */
  if (res >= POINTS_PER_SEMIAXIS[c]) res = POINTS_PER_SEMIAXIS[c] - 1;

  /* Gray remap */
  res = GRAY_MAP[c][res];

  /* Only use even bit offsets */
  res |= (res >> 1) << 2;
  res &= ~(1 << 1);

  return res;

}

inline uint8_t decode_bits(complex x, Constellation c, Hierarchy h) {

  x /= ENERGY_NORMALIZATION[c][h];

  double r = creal(x);
  double i = cimag(x);

  /* Decode the first two, high priority bits */
  uint8_t res = (((r < 0) << 1) + (i < 0)) << (BIT_LENGTH[c] - 2);

  /* Decode the low priority bits (if they exist) */
  if (c != CONSTELLATION_QPSK) {
    res |= decode_semiaxis(abs(r), c, h) << 1;
    res |= decode_semiaxis(abs(i), c, h);
  }

  return res;

}

/* TODO */
void ofdm_context_decode_bits(OFDMContext *ctx) {

  int carrier;
  size_t j = 0, k = 0, l = 0;
  for (carrier = 0; carrier < ctx->carrier_num; carrier++) {
    bool good = true;

    /* Check whether the carrier is being used as continual pilot */
    if ((j < CONTINUAL_PILOTS_LEN[ctx->trans_mode]) && (CONTINUAL_PILOTS[ctx->trans_mode][j] == carrier)) {
      good = false;
      j++;
    }

    /* Check whether the carrier is being used as scattered pilot */
    if ((((carrier % 12) % 3) == 0) && (((carrier % 12) / 3) == (ctx->frame_offset % 4))) {
      good = false;
    }

    /* Check whether the carrier is being used for TPS encoding */
    if ((k < TPS_CARRIERS_LEN[ctx->trans_mode]) && (TPS_CARRIERS[ctx->trans_mode][k] == carrier)) {
      good = false;
      k++;
    }

    if (good) {
      int idx = ctx->lower_idx + carrier;
      ctx->bits[l] = decode_bits(ctx->freqs[idx], ctx->constellation, ctx->hierarchy);
      l++;
    }
  }

  assert(l == USEFUL_CARRIERS_NUM[ctx->trans_mode]);

}
