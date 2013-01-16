/*
* owon-parse - a program to parse binary files
* Copyright (c) 2013 Jonathan BISSON >bjonnh on bjonnh.net<
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "parse.h"

int main(int argc, char **argv) {
  FILE *fp,*fp2;
  int fd,fd2;

  int i;

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
  

  if (fread((void *)buffer,sizeof(char),stbuf.st_size,fp) != stbuf.st_size) {
    printf("Error: can't read file %s\n",argv[1]);
    return(125);
  }


  owon_parse(buffer,stbuf.st_size,&file_header);

  fp2=fopen("output.csv","w+");
  
  owon_output_csv(&file_header,fp2);
  fclose(fp2);
  free((char *)buffer);
  owon_free_header(&file_header);
  return(0);
}
