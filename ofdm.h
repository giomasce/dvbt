
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

double PACKET_TIME[] = { 224e-6, 896e-6 };
uint32_t CARRIER_NUM[] = { 1705, 6817 };
double GUARD_INT_RATIO[] = { 1.0 / 32, 1.0 / 16, 1.0 / 8, 1.0 / 4 };

OFDMContext *ofdm_context_new(double samp_freq, double mod_freq, TransMode trans_mode, GuardInt guard_int, SlidingWindow *sw);
void ofdm_context_destroy(OFDMContext *ctx);

#endif
