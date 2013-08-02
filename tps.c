
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include "tps.h"

TPSDecoder *tps_decoder_new() {

  TPSDecoder *dec = malloc(sizeof(TPSDecoder));
  bzero(dec, sizeof(TPSDecoder));
  return dec;

}

void tps_decoder_destroy(TPSDecoder *dec) {

  free(dec);

}

bool tps_decoder_push_bit(TPSDecoder *dec, bool read_bit) {

  // Invert the bit according to DBPSK
  read_bit = !!read_bit;
  bool bit = read_bit ^ dec->last_bit;
  dec->last_bit = read_bit;

  // Push the bit in the buffer
  dec->len++;
  dec->buf <<= 1;
  dec->buf |= bit;

  // Detect synchronization word
  if ((dec->len == 17) || (dec->len > 17 && !dec->synced)) {
    dec->len = 17;
    if ((dec->buf & 0xffff) == TPS_SYNC_WORD_ODD) {
      dec->synced = true;
      dec->parity = true;
    } else if ((dec->buf & 0xffff) == TPS_SYNC_WORD_EVEN) {
      dec->synced = true;
      dec->parity = false;
    } else {
      dec->synced = false;
    }
  }

  // Detect packet termination
  if (dec->len == 68) {
    tps_decoder_interpret(dec);
    dec->parity = !dec->parity;
    dec->len = 0;
    return true;
  }

  return false;

}

void tps_decoder_interpret(TPSDecoder *dec) {

  TPSData *data = &dec->data;

  data->parity = dec->parity;
  data->error_protection = dec->buf & ((1 << 14) - 1);
  dec->buf >>= 14;
  // Empty bits
  dec->buf >>= 6;
  data->cell_id_byte = dec->buf & ((1 << 8) - 1);
  dec->buf >>= 8;
  data->trans_mode = dec->buf & ((1 << 2) - 1);
  dec->buf >>= 2;
  data->guard_int = dec->buf & ((1 << 2) - 1);
  dec->buf >>= 2;
  data->lp_code_rate = dec->buf & ((1 << 3) - 1);
  dec->buf >>= 3;
  data->hp_code_rate = dec->buf & ((1 << 3) - 1);
  dec->buf >>= 3;
  data->hierarchy = dec->buf & ((1 << 3) - 1);
  dec->buf >>= 3;
  data->constellation = dec->buf & ((1 << 2) - 1);
  dec->buf >>= 2;
  data->frame_num = dec->buf & ((1 << 2) - 1);
  dec->buf >>= 2;
  data->len = dec->buf & ((1 << 6) - 1);
  dec->buf >>= 6;

}

TPSData *tps_data_new() {

  TPSData *data = malloc(sizeof(TPSData));
  bzero(data, sizeof(TPSData));
  return data;

}

void tps_data_destroy(TPSData *data) {

  free(data);

}
