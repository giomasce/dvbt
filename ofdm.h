
#ifndef _OFDM_H
#define _OFDM_H

#include <stdint.h>

#include <complex.h>
#include <fftw3.h>

#include "util.h"
#include "data.h"

typedef struct {
  double samp_freq;
  double mod_freq;
  TransMode trans_mode;
  GuardInt guard_int;
  Constellation constellation;
  Hierarchy hierarchy;

  double packet_time;
  double guard_time;
  uint32_t carrier_num;
  uint32_t central_carrier;
  uint32_t packet_len;
  uint32_t full_len;
  uint32_t central_idx;
  uint32_t lower_idx;
  uint32_t higher_idx;

  uint8_t frame_offset;

  double complex *signal;
  double complex *freqs;
  uint8_t *bits;

  int32_t *carrier_map;
  int32_t *carrier_rev_map;

  fftw_plan fft_forward_plan;
  fftw_plan fft_backward_plan;

  SlidingWindow *sw;
} OFDMContext;

OFDMContext *ofdm_context_new(double samp_freq, double mod_freq, TransMode trans_mode, GuardInt guard_int, SlidingWindow *sw);
void ofdm_context_destroy(OFDMContext *ctx);
void ofdm_context_decode_symbol(OFDMContext *ctx, size_t offset);
double ofdm_context_optimize_offset(OFDMContext *ctx, double half_width, double *min_value);
void ofdm_context_shift_freqs(OFDMContext *ctx, double samples);
bool ofdm_context_read_tps_bit(OFDMContext *ctx);
void ofdm_context_dump_debug(OFDMContext *ctx, char *filename);
void ofdm_context_dump_freqs(OFDMContext *ctx, char *filename);
void ofdm_context_dump_gnuplot(OFDMContext *ctx, char *filename);
void ofdm_context_normalize_energy(OFDMContext *ctx);
void ofdm_context_decode_bits(OFDMContext *ctx);

uint8_t *pilot_sequence;

void build_pilot_sequence();

#endif
