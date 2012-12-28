#include <stdio.h>
#include <stdint.h>

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
  uint32_t samples;
  uint32_t samples2;
  uint32_t samples3;
  uint32_t timediv;
  int32_t offsety;
  unsigned char unknown5[4];
  char attenuation;
  unsigned char unknown6[9];
  uint32_t frequency; // ??
  uint32_t frequency2; // ??
  unsigned char unknown7[2];
};

int main(int argc, char **argv) {
  FILE *fptr;
  char buffer[4096];
  int i;
  struct header file_header;
  struct channel ch1;

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
  printf("File name: %s\n",argv[1]);
  //  printf("File length: %d\n",file_header.length);
  printf("Unknown1: %d\n",file_header.unknown1);
  //printf("Type: %d\n",file_header.type);
  //printf("Model: %s\n",file_header.model);
  printf("Unknown2: %X\n",file_header.unknown2);
  //printf("Serial: %s\n",file_header.serial);
  for(i=0;i<sizeof(file_header.unknown3);i++) {
    printf("%02X ",file_header.unknown3[i]);
  }
  printf("\n");
  fread(&ch1.name,sizeof(char),sizeof(ch1.name)-1,fptr);
  ch1.name[3]=0;
  printf("Channel name: %s\n",ch1.name);
  fread(&ch1.unknown4,sizeof(char),sizeof(ch1.unknown4),fptr);  
  for(i=0;i<sizeof(ch1.unknown4);i++) {
    if ((i % 0x10)==0) printf("\n%08X: ",i);
    printf("%02X ",ch1.unknown4[i]);
  }  
  printf("\n");
  fread(&ch1.samples,sizeof(ch1.samples),1,fptr);  
  //printf("Samples (count): %d\n",ch1.samples);
  fread(&ch1.samples2,sizeof(ch1.samples2),1,fptr);  
  //printf("Samples (screen): %d\n",ch1.samples3);
  fread(&ch1.samples3,sizeof(ch1.samples3),1,fptr);  
  //printf("Samples (slow-scan): %d\n",ch1.samples3);
  fread(&ch1.timediv,sizeof(ch1.timediv),1,fptr);  
  //printf("Time/div: %d\n",ch1.timediv);


  fread(&ch1.offsety,sizeof(ch1.offsety),1,fptr);  
  //printf("OffsetY: %d\n",ch1.offsety);

  fread(&ch1.unknown5,sizeof(char),sizeof(ch1.unknown5),fptr);  
  for(i=0;i<sizeof(ch1.unknown5);i++) {
    if ((i % 0x10)==0) printf("\n%08X: ",i);
    printf("%02X ",ch1.unknown5[i]);
  }  
  printf("\n");

  fread(&ch1.attenuation,sizeof(ch1.attenuation),1,fptr);  
  printf("Attenuation: %d\n",ch1.attenuation);

  fread(&ch1.unknown6,sizeof(char),sizeof(ch1.unknown6),fptr);  
  for(i=0;i<sizeof(ch1.unknown6);i++) {
    if ((i % 0x10)==0) printf("\n%08X: ",i);
    printf("%02X ",ch1.unknown6[i]);
  }  
  printf("\n");
  fread(&ch1.frequency,sizeof(ch1.frequency),1,fptr);  
  printf("Frequency: %d\n",ch1.frequency);

  fread(&ch1.frequency2,sizeof(ch1.frequency2),1,fptr);  
  printf("Frequency2: %d\n",ch1.frequency2);

  fread(&ch1.unknown7,sizeof(char),sizeof(ch1.unknown7),fptr);  
  for(i=0;i<sizeof(ch1.unknown7);i++) {
    if ((i % 0x10)==0) printf("\n%08X: ",i);
    printf("%02X ",ch1.unknown7[i]);
  }  


  printf("\n");
  return 0;
}
