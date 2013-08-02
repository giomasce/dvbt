
#ifndef _TPS_H
#define _TPS_H

#include <stdint.h>
#include <stdbool.h>

#include "data.h"

typedef struct {
  bool parity;
  uint8_t len;
  FrameNum frame_num;
  Constellation constellation;
  Hierarchy hierarchy;
  CodeRate hp_code_rate;
  CodeRate lp_code_rate;
  GuardInt guard_int;
  TransMode trans_mode;
  uint8_t cell_id_byte;
  uint8_t error_protection;
} TPSData;

typedef struct {
  bool last_bit;
  uint64_t buf;
  uint8_t len;
  bool synced;
  bool parity;
  TPSData data;
} TPSDecoder;

TPSDecoder *tps_decoder_new();
void tps_decoder_destroy(TPSDecoder *dec);
bool tps_decoder_push_bit(TPSDecoder *dec, bool bit);
void tps_decoder_interpret(TPSDecoder *dec);

TPSData *tps_data_new();
void tps_data_destroy(TPSData *data);

#endif
