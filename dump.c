#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define CHANNELS 4
#define ARRAY_LENGTH(x) (sizeof(x)/sizeof(*(x)))

struct header {
  int32_t length;
  int32_t unknown1;
  int32_t type;
  char model[7];
  int32_t intsize;
  int v;
  int vid;
  int mid;
  char serial[30];
  unsigned char trgstatus;
  unsigned char chlstatus;
  float htp;
  unsigned char sna;
  unsigned char unknown3[8];
};

struct channel {
  char name[4];
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
  int16_t *data;
};

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
    return(_volt_table[ARRAY_LENGTH(_volt_table)]);
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

void debug_channel(struct channel *chan) {
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

  void debug_file(struct header *file_header) {
    int v,vid,mid;
    //printf("File length: %d\n",(*file_header).length);
#ifdef DEBUG_UNKNOWN
    printf("Unknown1: %d\n",(*file_header).unknown1);
#endif
#ifdef DEBUG_KNOWN
    printf("Type: %d\n",(*file_header).type);
    printf("Model: %s\n",(*file_header).model);
#endif
#ifdef DEBUG_UNKNOWN
    printf("Intsize: %d\n",(*file_header).intsize);
    
    printf("Intsize v: %d\n",(*file_header).v);
    printf("Intsize vid: %d\n",(*file_header).vid);
    printf("Intsize mid: %d\n",(*file_header).mid);
#endif
#ifdef DEBUG_KNOWN
    printf("Serial: %s\n",(*file_header).serial);
#endif
#ifdef DEBUG_UNKNOWN
    printf("TRGSTATUS: %d\n",(*file_header).trgstatus);
    printf("CHLSTATUS: %d\n",(*file_header).chlstatus);
    printf("HTP: %f\n",(*file_header).htp);
    printf("SNA: %d\n",(*file_header).sna);

    printf("Unknown3\n");	      
    show_hex_uchar((*file_header).unknown3,8);
#endif
}

int main(int argc, char **argv) {
  FILE *fptr;
  char buffer[4096];
  int i;
  struct header file_header;
  struct channel *channels;
  unsigned short int current_channel=0;
  struct channel *channelptr;
  unsigned short int temp;
  char test_header[4];
  
  channels = calloc(CHANNELS,sizeof(struct channel));
  
  if (argc<2) {
    printf("Give me the food !\n");
    return 1;
  }

  fptr=fopen(argv[1],"rb");
  if (fptr==NULL) {
    printf("Error: can't open file %s\n",argv[1]);
    return(128);
  }

  // The three following fields are only found on LAN-coming files so we are checking if it's the case
  // Maybe we should simply dump them when doing the LAN grabber
  fseek(fptr,12,SEEK_SET);
  fread(&test_header,sizeof(char),3,fptr);
  test_header[3]=0;
  fseek(fptr,0,SEEK_SET);
  if (strncmp(test_header,"SPB",3)==0) {  
    fread(&file_header.length,sizeof(int),1,fptr);
    fread(&file_header.unknown1,sizeof(int),1,fptr);      
    fread(&file_header.type,sizeof(int),1,fptr);
  }

  fread(&file_header.model,sizeof(char),sizeof(file_header.model)-1,fptr);
  file_header.model[6]=0;

  fread(&file_header.intsize,sizeof(file_header.intsize),1,fptr);

  if (file_header.intsize < 1) {
    file_header.v = file_header.intsize & 0x7fffffff;
    file_header.vid = file_header.v & 0xffff;
    file_header.mid = file_header.v >> 16 & 0xffff;
    if (file_header.vid == 101) {
    }
  } 


  fread(&file_header.serial,sizeof(char),sizeof(file_header.serial)-1,fptr);
  file_header.serial[17]=0;
  fread(&file_header.trgstatus,sizeof(file_header.trgstatus),1,fptr);
  fread(&file_header.chlstatus,sizeof(file_header.chlstatus),1,fptr);
  fread(&file_header.htp,sizeof(file_header.htp),1,fptr);
  fread(&file_header.sna,sizeof(file_header.sna),1,fptr);
  fread(&file_header.unknown3,sizeof(char),sizeof(file_header.unknown3),fptr);

  debug_file(&file_header);


  // Reading channels
  channelptr=channels;
  while (ftell(fptr) < file_header.length+12 && current_channel < CHANNELS) {

    fread(&(*channelptr).name,sizeof(char),sizeof((*channelptr).name)-1,fptr);
    (*channelptr).name[3]=0;
    fread(&(*channelptr).unknownint,sizeof((*channelptr).unknownint),1,fptr);  
    fread(&(*channelptr).datatype,sizeof((*channelptr).datatype),1,fptr);  
    fread(&(*channelptr).unknown4,sizeof(char),sizeof((*channelptr).unknown4),fptr);  
    fread(&(*channelptr).samples_count,sizeof((*channelptr).samples_count),1,fptr);  
    fread(&(*channelptr).samples_file,sizeof((*channelptr).samples_file),1,fptr);  

    fread(&(*channelptr).samples3,sizeof((*channelptr).samples3),1,fptr);  

    fread(&(*channelptr).timediv,sizeof((*channelptr).timediv),1,fptr);  
    
    fread(&(*channelptr).offsety,sizeof((*channelptr).offsety),1,fptr);  
    
    fread(&(*channelptr).voltsdiv,sizeof((*channelptr).voltsdiv),1,fptr);  

    fread(&(*channelptr).attenuation,sizeof((*channelptr).attenuation),1,fptr);  
    
    fread(&(*channelptr).time_mul,sizeof(float),1,fptr);  

    fread(&(*channelptr).frequency,sizeof(float),1,fptr);  

    fread(&(*channelptr).period,sizeof(float),1,fptr);  

    fread(&(*channelptr).volts_mul,sizeof(float),1,fptr);      
  
    (*channelptr).data = (int16_t *) calloc((*channelptr).samples_file,sizeof(int16_t));
  
    if ((*channelptr).data == NULL) {
      printf("Error: Can't allocate %d bytes of memory.\n",(*channelptr).samples_file*sizeof(int16_t));
      return 2;
    }

    for(i=0;i<(*channelptr).samples_file;i++) {
      if ((*channelptr).datatype == 2) {
	fread(&(*channelptr).data[i],sizeof(int16_t),1,fptr);
      } else {
	fread(&temp,sizeof(int8_t),1,fptr);
	(*channelptr).data[i]=temp;
      }
      if (feof(fptr) || ferror(fptr)) {
	printf("Error: EOF before end of channel.\n");
	return 3;
      }
    }
    debug_channel(channelptr);
    current_channel++;
  }
  return 0;
}
