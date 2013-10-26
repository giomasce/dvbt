
#ifndef _DATA_H
#define _DATA_H

typedef enum{
  FRAME_NUM_1 = 0x0,
  FRAME_NUM_2 = 0x1,
  FRAME_NUM_3 = 0x2,
  FRAME_NUM_4 = 0x3
} FrameNum;

typedef enum {
  CONSTELLATION_QPSK = 0x0,
  CONSTELLATION_16_QAM = 0x1,
  CONSTELLATION_64_QAM = 0x2,
  CONSTELLATION_NUM
} Constellation;

typedef enum {
  HIERARCHY_NON_HIERARCHICAL = 0x0,
  HIERARCHY_ALPHA_1 = 0x1,
  HIERARCHY_ALPHA_2 = 0x2,
  HIERARCHY_ALPHA_4 = 0x3,
  HIERARCHY_NUM
} Hierarchy;

typedef enum {
  CODE_RATE_1_2 = 0x0,
  CODE_RATE_2_3 = 0x1,
  CODE_RATE_3_4 = 0x2,
  CODE_RATE_5_6 = 0x3,
  CODE_RATE_7_8 = 0x4
} CodeRate;

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

typedef enum {
  BANDWIDTH_8MHZ,
  BANDWIDTH_7MHZ,
  BANDWIDTH_6MHZ
} Bandwidth;

extern const uint16_t CONTINUAL_PILOTS_2K[];
extern const uint16_t CONTINUAL_PILOTS_8K[];
extern const uint16_t *CONTINUAL_PILOTS[];
extern const size_t CONTINUAL_PILOTS_LEN[];
extern const uint16_t TPS_CARRIERS_2K[];
extern const uint16_t TPS_CARRIERS_8K[];
extern const uint16_t *TPS_CARRIERS[];
extern const size_t TPS_CARRIERS_LEN[];
extern const size_t USEFUL_CARRIERS_NUM[];
extern const double PACKET_TIME[];
extern const uint32_t CARRIER_NUM[];
extern const uint32_t MAX_CARRIER_NUM;
extern const double GUARD_INT_RATIO[];
extern const double ENERGY_NORMALIZATION[CONSTELLATION_NUM][HIERARCHY_NUM];

extern const uint8_t POINTS_PER_SEMIAXIS[CONSTELLATION_NUM];
#define MAX_POINTS_PER_SEMIAXIS 4
extern const uint8_t START_COORDINATE[HIERARCHY_NUM];
extern const uint8_t BIT_LENGTH[CONSTELLATION_NUM];
extern const uint8_t GRAY_MAP[CONSTELLATION_NUM][MAX_POINTS_PER_SEMIAXIS];

extern const uint64_t TPS_SYNC_WORD_ODD;
extern const uint64_t TPS_SYNC_WORD_EVEN;

#endif
