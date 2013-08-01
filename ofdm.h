
#ifndef _OFDM_H
#define _OFDM_H

#include <stdint.h>

#include <complex.h>
#include <fftw3.h>

#include "util.h"

typedef enum {
  TRANS_MODE_2K = 0x0,
  TRANS_MODE_8K = 0x1
} TransMode;

typedef enum {
  GUARD_INT_1_32 = 0x0,
  GUARD_INT_1_16 = 0x1,
  GUARD_INT_1_8 = 0x2,
  GUARD_INT_1_4 = 0x3
} GuardInt;

typedef struct {
  double samp_freq;
  double mod_freq;
  TransMode trans_mode;
  GuardInt guard_int;
  double packet_time;
  double guard_time;
  uint32_t carrier_num;
  uint32_t central_carrier;
  uint32_t packet_len;
  uint32_t full_len;
  uint32_t central_idx;
  uint32_t lower_idx;
  uint32_t higher_idx;
  double *signal;
  complex *freqs;
  fftw_plan fft_forward_plan;
  fftw_plan fft_backward_plan;
  SlidingWindow *sw;
} OFDMContext;

extern const double PACKET_TIME[];
extern const uint32_t CARRIER_NUM[];
extern const uint32_t MAX_CARRIER_NUM;
extern const double GUARD_INT_RATIO[];

OFDMContext *ofdm_context_new(double samp_freq, double mod_freq, TransMode trans_mode, GuardInt guard_int, SlidingWindow *sw);
void ofdm_context_destroy(OFDMContext *ctx);
void ofdm_context_decode_symbol(OFDMContext *ctx, size_t offset);
double ofdm_context_optimize_offset(OFDMContext *ctx, double half_width, double *min_value);
void ofdm_context_shift_freqs(OFDMContext *ctx, double samples);
uint8_t ofdm_context_read_tps_bit(OFDMContext *ctx);
void ofdm_context_dump_freqs(OFDMContext *ctx, char *filename);

uint8_t *pilot_sequence;

void build_pilot_sequence();

#endif
