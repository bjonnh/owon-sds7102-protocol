#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define CHANNELS 4

struct header {
  int length;
  int unknown1;
  int type;
  char model[7];
  int unknown2;
  char serial[18];
  unsigned char unknown3[27];
};

struct channel {
  char name[4];
  unsigned char unknown4[12];
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
  printf("Channel name: %s\n",(*chan).name);
  printf("Unknown4: \n");
  show_hex_uchar((*chan).unknown4,12);
  //  printf("Samples (count): %d\n",(*chan).samples_count);
  //printf("Samples (file): %d\n",(*chan).samples_file);
  //printf("Samples (slow-scan): %d\n",(*chan).samples3);
  //printf("Time/div: %d\n",(*chan).timediv);
  //printf("OffsetY: %d\n",(*chan).offsety);
  //printf("Volts/div: %d\n",(*chan).voltsdiv);
  //printf("Attenuation: %d\n",(*chan).attenuation);
  //printf("Time_mul: %f\n",(*chan).time_mul);
  //printf("Frequency: %f\n",(*chan).frequency);
  //printf("Period: %f\n",(*chan).period);
  //printf("Volts_mul: %f\n",(*chan).volts_mul);

  //  printf("Data (%d blocks): \n",(*chan).samples_file);
  //  show_hex_int16t((*chan).data,(*chan).samples_file);
}

void debug_file(struct header *file_header) {
  //printf("File length: %d\n",(*file_header).length);
  printf("Unknown1: %d\n",(*file_header).unknown1);
  //printf("Type: %d\n",(*file_header).type);
  //printf("Model: %s\n",(*file_header).model);
  printf("Unknown2: %X\n",(*file_header).unknown2);
  //printf("Serial: %s\n",(*file_header).serial);
  printf("Unknown3\n");	      
  show_hex_uchar((*file_header).unknown3,27);

}

int main(int argc, char **argv) {
  FILE *fptr;
  char buffer[4096];
  int i;
  struct header file_header;
  struct channel *channels;
  unsigned short int current_channel=0;
  struct channel *channelptr;

  channels = calloc(CHANNELS,sizeof(struct channel));
  
  if (argc<2) {
    printf("Give me the food !\n");
    return 1;
  }

  fptr=fopen(argv[1],"rb");

  fread(&file_header.length,sizeof(int),1,fptr);
  fread(&file_header.unknown1,sizeof(int),1,fptr);
  fread(&file_header.type,sizeof(int),1,fptr);
  fread(&file_header.model,sizeof(char),sizeof(file_header.model)-1,fptr);
  file_header.model[6]=0;

  fread(&file_header.unknown2,sizeof(int),1,fptr);
  fread(&file_header.serial,sizeof(char),sizeof(file_header.serial)-1,fptr);
  file_header.serial[17]=0;
  fread(&file_header.unknown3,sizeof(char),sizeof(file_header.unknown3),fptr);
  //  printf("File name: %s\n",argv[1]);
  debug_file(&file_header);
  // Reading channels
  channelptr=channels;
  while (ftell(fptr) < file_header.length+12 && current_channel < CHANNELS) {

    fread(&(*channelptr).name,sizeof(char),sizeof((*channelptr).name)-1,fptr);
    (*channelptr).name[3]=0;

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
      fread(&(*channelptr).data[i],sizeof(int16_t),1,fptr);
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
