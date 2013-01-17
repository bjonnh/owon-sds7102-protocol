#ifndef _PARSE_H_
#define _PARSE_H_

#include <stdint.h>

typedef struct {
  const unsigned char *data;
  const unsigned char *data_p;
  size_t len;
} DATA_st;

typedef struct {
  unsigned char name[4];
  int32_t unknownint;
  int32_t datatype;
  unsigned char unknown4[4];
  uint32_t samples_count;
  uint32_t samples_file;
  uint32_t samples3;
  double timediv;
  int32_t offsety;
  float voltsdiv;
  uint32_t attenuation;
  float time_mul;
  float frequency;
  float period;
  float volts_mul;
  double *data;
} CHANNEL_st;

typedef struct {
  uint32_t length;
  int32_t unknown1;
  int32_t type;
  char model[7];
  int32_t intsize;
  char serial[30];
  unsigned char triggerstatus;
  unsigned char unknownstatus;
  uint32_t unknownvalue1;
  unsigned char unknownvalue2;
  unsigned char unknown3[8];
  size_t channels_count;
  CHANNEL_st **channels;
} HEADER_st;


int owon_parse(const char * const buf, size_t len, HEADER_st *header);
int owon_output_csv(HEADER_st *header, FILE *file);
void owon_free_header(HEADER_st *header);

#endif
