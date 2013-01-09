#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ARRAY_LENGTH(x) (sizeof(x)/sizeof(*(x)))

typedef struct {
  unsigned char name[4];
  int32_t unknownint;
  int32_t datatype;
  unsigned char unknown4[4];
  uint32_t samples_count;
  uint32_t samples_file;
  uint32_t samples3;
  uint32_t timediv;
  int32_t offsety;
  uint32_t voltsdiv;
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
  float unknownvalue1;
  unsigned char unknownvalue2;
  unsigned char unknown3[8];
  size_t channels_count;
  CHANNEL_st **channels;
} HEADER_st;

// Tables are from the Levi Larsen app
float _attenuation_table[] = {1.0e0, 1.0e1, 1.0e2, 1.0e3};
float _volt_table[] = {
  2.0e-3, 5.0e-3, // 1 mV
  1.0e-2, 2.0e-2, 5.0e-2, // 10 mV
  1.0e-1, 2.0e-1, 5.0e-1, // 100 mV
  1.0e+0, 2.0e+0, 5.0e+0, // 1 V
  1.0e+1, 2.0e+1, 5.0e+1, // 10 V
  1.0e+2, 2.0e+2, 5.0e+2, // 100 V
  1.0e+3, 2.0e+3, 5.0e+3, // 1 kV
  1.0e+4                  // 10 kV
};

float get_real_attenuation(uint32_t attenuation) {
  if (attenuation > ARRAY_LENGTH(_attenuation_table)) {
    return(_attenuation_table[ARRAY_LENGTH(_attenuation_table)]);
  } else {
    return(_attenuation_table[attenuation]);
  }
}

float get_real_voltscale(uint32_t volt) {
  if (volt > ARRAY_LENGTH(_volt_table)) {
    return(_volt_table[ARRAY_LENGTH(_volt_table)]);
  } else {
    return(_volt_table[volt]);
  }
}

void show_hex_uchar(unsigned char *data,unsigned int length) {
  int i;
  for(i=0;i<length;i++) {
    if ((i % 0x10)==0) printf("\n%08X: ",i);
    printf("%02X ",data[i]);
  }  
  printf("\n");
}

void show_hex_int16t(int16_t *data,unsigned int size) {
  int i;
  for(i=0;i<size;i++) {
    if ((i % 0x10)==0) printf("\n%08X: ",i);
    printf("%d ",(int16_t) data[i]);
  }  
  printf("\n");
}

void debug_channel(CHANNEL_st *chan) {
#ifdef DEBUG_KNOWN
  printf("Channel name: %s\n",(*chan).name);
#endif
#ifdef DEBUG_UNKNOWN
  printf("unknownint: %d\n",(*chan).unknownint);
#endif
#ifdef DEBUG_KNOWN
  printf("datatype: %d\n",(*chan).datatype);
#endif
#ifdef DEBUG_UNKNOWN
  printf("Unknown4: \n");
  show_hex_uchar((*chan).unknown4,4);
#endif
#ifdef DEBUG_KNOWN
  printf("Samples (count): %d\n",(*chan).samples_count);
  printf("Samples (file): %d\n",(*chan).samples_file);
  printf("Samples (slow-scan): %d\n",(*chan).samples3);
  printf("Time/div: %d\n",(*chan).timediv);
  printf("OffsetY: %d\n",(*chan).offsety);
  printf("Volts/div: %d\n",(*chan).voltsdiv);
  printf("Attenuation: %d\n",(*chan).attenuation);
  printf("Time_mul: %f\n",(*chan).time_mul);
  printf("Frequency: %f\n",(*chan).frequency);
  printf("Period: %f\n",(*chan).period);
  printf("Volts_mul: %f\n",(*chan).volts_mul);
#endif
  //  printf("Data (%d blocks): \n",(*chan).samples_file);
  //  show_hex_int16t((*chan).data,(*chan).samples_file);
}

void debug_file(HEADER_st *file_header) {
  //printf("File length: %d\n",file_header->length);
#ifdef DEBUG_UNKNOWN
  printf("Unknown1: %d\n",file_header->unknown1);
#endif
#ifdef DEBUG_KNOWN
  printf("Type: %d\n",file_header->type);
  printf("Model: %s\n",file_header->model);
#endif
#ifdef DEBUG_UNKNOWN
  printf("Intsize: %d\n",file_header->intsize);

#endif
#ifdef DEBUG_KNOWN
  printf("Serial: %s\n",file_header->serial);
#endif
#ifdef DEBUG_UNKNOWN
  printf("TRIGGERSTATUS: %d\n",file_header->triggerstatus);
  printf("UNKNOWNSTATUS: %d\n",file_header->unknownstatus);
  printf("UnknownValue1: %f\n",file_header->unknownvalue1);
  printf("UnknownValue2: %c ",file_header->unknownvalue2);
  switch(file_header->unknownvalue2) {
  case 'M':
    printf("Slow-Scan\n");
    break;
  case 'F':
    printf("Unknown\n");
    break;
  case 'G':
    printf("Unknown\n");
    break;
  default:
    printf("Never seen\n");
  }

  printf("Unknown3\n");	      
  show_hex_uchar(file_header->unknown3,8);
#endif
}

// Reading an unsigned 32 and increment the data_p position accordingly

uint32_t read_u32(const unsigned char **data_p) {
  uint32_t temp;
  temp = (uint32_t)(*data_p)[3] << 24 |
    (uint32_t)(*data_p)[2] << 16 |
    (uint32_t)(*data_p)[1] << 8  |
    (uint32_t)(*data_p)[0];
  *data_p += sizeof(uint32_t);
  return temp;
}

// Reading a signed 32 and increment the data_p position accordingly

int32_t read_32(const unsigned char **data_p) {
  int32_t temp;
  temp = (int32_t)(*data_p)[3] << 24 |
    (int32_t)(*data_p)[2] << 16 |
    (int32_t)(*data_p)[1] << 8  |
    (int32_t)(*data_p)[0];
  *data_p += sizeof(int32_t);
  return temp;
}

// Reading a signed 16 and increment the data_p position accordingly

uint16_t read_16(const unsigned char **data_p) {
  uint16_t temp;
  temp = (uint16_t)(*data_p)[1] << 8  |
    (uint16_t)(*data_p)[0];
  *data_p += sizeof(uint16_t);
  return temp;
}

// Reading a float and increment the data_p position accordingly

float read_f(const unsigned char **data_p) {
  float temp;
  temp = (float)(**data_p);
  *data_p += sizeof(float);
  return temp;
}

// Reading a char and increment the data_p position accordingly

unsigned char read_char(const unsigned char **data_p) {
  unsigned char temp;
  temp = (unsigned char)(**data_p);
  *data_p += sizeof(unsigned char);
  return temp;
}

// Reading a string from *data_p of length len-1, and copying in the memory area at destination.
// add a \0 at the end of destination and increment data_p position
void read_string_nullify(const unsigned char **data_p, char *destination, size_t len) {
  memcpy(destination,*data_p,len-1);
  destination[len]=0;
  *data_p += len-1;
}

// Reading a string from *data_p of length len, and copying in the memory area at destination.
// then increment data_p position

void read_string(const unsigned char **data_p, char *destination, size_t len) {
  memcpy(destination,*data_p,len);
  *data_p += len;
}

// Parse a channel from data, len will be used to check if there is enough data

int parse_channel(const unsigned char **data, size_t len, CHANNEL_st *channel) 
{
  size_t i;

  read_string_nullify(data,channel->name,4);
  channel->unknownint = read_32(data);
  channel->datatype = read_32(data);
  read_string(data,channel->unknown4,4);
  channel->samples_count = read_u32(data);
  channel->samples_file = read_u32(data);
  channel->samples3 = read_u32(data);
  channel->timediv = read_u32(data);
  channel->offsety = read_32(data);
  channel->voltsdiv = read_u32(data);
  channel->attenuation = read_u32(data);
  channel->time_mul = read_f(data);
  channel->frequency = read_f(data);
  channel->period = read_f(data);
  channel->volts_mul = read_f(data);

  channel->data = (double *) calloc(channel->samples_file,sizeof(double));
  if (channel->data == NULL) {
    printf("Error: Can't allocate %d bytes of memory.\n",channel->samples_file*sizeof(int16_t));
    return 2;
  }

  if (channel->datatype == 2) {
    memcpy(&(channel->data),data,channel->samples_file*sizeof(int16_t));
  } else {
    memcpy(&(channel->data),data,channel->samples_file*sizeof(char));
  }

  debug_channel(channel);
}

int parse(const char *data, size_t len, HEADER_st *header)
{
unsigned int i;
const unsigned char *data_p;

memset(header,0,sizeof(HEADER_st)); // Putting NULL in the structure for fields not present
 
data_p = data;

if (strncmp(data+12,"SPB",3) == 0) {
header->length = read_32(&data_p); // This is the length without the LAN header
printf("Length: %d\n",header->length);
header->unknown1 = read_32(&data_p);
header->type = read_32(&data_p); // This seems to be related to the number of parts in the file
}

read_string_nullify(&data_p,header->model,sizeof(header->model));

header->intsize=read_32(&data_p); // Not really the size ? It was in some models.

if (header->intsize != 0xFFFFFF) { // For short files coming from official app
read_string_nullify(&data_p,header->serial,sizeof(header->serial));

header->triggerstatus = read_char(&data_p);

header->unknownstatus = read_char(&data_p);
header->unknownvalue1 = read_f(&data_p);
header->unknownvalue2 = read_char(&data_p);
read_string(&data_p,header->unknown3,sizeof(header->unknown3));

debug_file(header);    

}

header->channels_count = 0;
  
while ((void *)data_p < (void *)data + len) {
if (strncmp(data_p,"CH",2) == 0) {
  header->channels_count++;
  header->channels = realloc(header->channels,sizeof(void *) * header->channels_count);
  header->channels[header->channels_count] = malloc(sizeof(CHANNEL_st));
  if (header->channels==NULL) {
    printf("Can't allocate %d bytes of memory for channel data.\n",sizeof(CHANNEL_st) * header->channels_count);
    return(126);
  }
  parse_channel(&data_p,len,header->channels[header->channels_count]);
 } else if (strncmp(data_p,"INFO",4) == 0) {

 }
 data_p++;
 }
 return(0);
}

int main(int argc, char **argv) {
  FILE *fp;
  int fd;

  HEADER_st file_header;
  struct stat stbuf;

  char *buffer;
  
  if (argc<2) {
    printf("Give me the food !\n");
    return 1;
  }

  fd=open(argv[1],O_RDONLY);
  if (fd==-1) {
    printf("Error: can't open file %s\n",argv[1]);
    return(128);
  }
  fp=fdopen(fd,"rb");
  if (fp==NULL) {
    printf("Error: can't open file %s\n",argv[1]);
    return(128);
  }

  if (fstat(fd, &stbuf) == -1) {
    printf("Error: %s may not be a regular file\n",argv[1]);
    return(127);
  }

  buffer = calloc(stbuf.st_size,sizeof(char));
  if (buffer==NULL) {
    printf("Can't allocate %d bytes of memory.\n",stbuf.st_size);
    return(126);
  }
  
  if (fread(buffer,sizeof(char),stbuf.st_size,fp) != stbuf.st_size) {
    printf("Error: can't read file %s\n",argv[1]);
    return(125);
  }

  parse(buffer,stbuf.st_size,&file_header);
  return(0);
}

