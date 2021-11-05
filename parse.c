/*
 * parse - functions for parsing owon binary files
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
#include <sys/stat.h>
#include <fcntl.h>
#include "parse.h"

#define ARRAY_LENGTH(x) (sizeof(x)/sizeof(*(x)))

// _attenuation_table is from the Levi Larsen app
static float _attenuation_table[] = { 1.0e0, 1.0e1, 1.0e2, 1.0e3 }; // We are only sure for these
static float _volt_table[] = {
        2.0e-3, 5.0e-3, 1.0e-2,            // T.S. 05NOV2021     SDS-7102 also has 2 mV .. 10 mV / Div
	2.0e-2, 5.0e-2, // 10 mV
	1.0e-1, 2.0e-1, 5.0e-1, // 100 mV
	1.0e+0, 2.0e+0, 5.0e+0, // 1 V
	1.0e+1, 2.0e+1, 5.0e+1, // 10 V
	1.0e+2 // 100 V
};

// Only for SDS7102 need to allow different models
static double _timescale_table[] = {
	2.0e-9, 5.0e-9,  // 2 ns
	1.0e-8, 2.0e-8, 5.0e-8, // 10 ns
	1.0e-7, 2.0e-7, 5.0e-7, // 100 ns
	1.0e-6, 2.0e-6, 5.0e-6, // 1 us
	1.0e-5, 2.0e-5, 5.0e-5, // 10 us
	1.0e-4, 2.0e-4, 5.0e-4, // 100 us
	1.0e-3, 2.0e-3, 5.0e-3, // 1 ms
	1.0e-2, 2.0e-2, 5.0e-2, // 10 ms
	1.0e-1, 2.0e-1, 5.0e-1, // 100 ms
	1.0e+0, 2.0e+0, 5.0e+0, // 1 s
	1.0e+1, 2.0e+1, 5.0e+1, // 10 s
	1.0e+2 // 100 s
};

static double get_real_timescale(uint32_t timescale) {
	if (timescale >= ARRAY_LENGTH(_timescale_table)) {
		return(_timescale_table[ARRAY_LENGTH(_timescale_table) - 1]);
	} else {
		return(_timescale_table[timescale]);
	}
}

static float get_real_attenuation(uint32_t attenuation) {
	if (attenuation >= ARRAY_LENGTH(_attenuation_table)) {
		return(_attenuation_table[ARRAY_LENGTH(_attenuation_table) - 1]);
	} else {
		return(_attenuation_table[attenuation]);
	}
}

static float get_real_voltscale(uint32_t volt) {
	if (volt >= ARRAY_LENGTH(_volt_table)) {
		return(_volt_table[ARRAY_LENGTH(_volt_table) - 1]);
	} else {
		return(_volt_table[volt]);
	}
}

static void show_hex_uchar(unsigned char *data,unsigned int length) {
	int i;
	for(i=0;i<length;i++) {
		if ((i % 0x10)==0) printf("\n%08X: ",i);
		printf("%02X ",data[i]);
	}
	printf("\n");
}

static void show_hex_int16t(int16_t *data,unsigned int size) {
	int i;
	for(i=0;i<size;i++) {
		if ((i % 0x10)==0) printf("\n%08X: ",i);
		printf("%d ",(int16_t) data[i]);
	}
	printf("\n");
}

static void debug_channel(CHANNEL_st *chan) {
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
	printf("Time/div: %.10f\n",(*chan).timediv);
	printf("OffsetY: %d\n",(*chan).offsety);
	printf("Volts/div: %f\n",(*chan).voltsdiv);
	printf("Attenuation: %u\n",(*chan).attenuation);
	printf("Time_mul: %f\n",(*chan).time_mul);
	printf("Frequency: %f\n",(*chan).frequency);
	printf("Period: %f\n",(*chan).period);
	printf("Volts_mul: %f\n",(*chan).volts_mul);
#endif
	//  printf("Data (%d blocks): \n",(*chan).samples_file);
	//  show_hex_int16t((*chan).data,(*chan).samples_file);
}

static void debug_file(HEADER_st *file_header) {
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
	printf("UnknownValue1: %u\n",file_header->unknownvalue1);
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

static uint32_t read_u32(DATA_st *data) {
	uint32_t temp;
	temp = (uint32_t)(data->data_p[3]) << 24 |
		(uint32_t)(data->data_p[2]) << 16 |
		(uint32_t)(data->data_p[1]) << 8  |
		(uint32_t)(data->data_p[0]);
	data->data_p += sizeof(uint32_t);
	return temp;
}

// Reading a signed 32 and increment the data_p position accordingly

static int32_t read_32(DATA_st *data) {
	int32_t temp;
	temp = (int32_t)(data->data_p[3]) << 24 |
		(int32_t)(data->data_p[2]) << 16 |
		(int32_t)(data->data_p[1]) << 8  |
		(int32_t)(data->data_p[0]);
	data->data_p += sizeof(int32_t);
	return temp;
}

// Reading a signed 16 and increment the data_p position accordingly

static int16_t read_16(DATA_st *data) {
	int16_t temp;
	temp = (int16_t)(data->data_p[1]) << 8  |
		(int16_t)(data->data_p[0]);
	data->data_p += sizeof(uint16_t);
	return temp;
}

// Reading a float and increment the data_p position accordingly

static float read_f(DATA_st *data) {
	float temp;

	memcpy(&temp,&(*data->data_p),4);

	data->data_p += sizeof(float);
	return temp;
}

// Reading a char and increment the data_p position accordingly

static unsigned char read_char(DATA_st *data) {
	unsigned char temp;
	temp = (unsigned char)(*data->data_p);
	data->data_p += sizeof(unsigned char);
	return temp;
}

// Reading a string from *data_p of length len-1, and copying in the memory area at destination.
// add a \0 at the end of destination and increment data_p position
void read_string_nullify(DATA_st *data, char *destination, size_t len) {
	memcpy(destination,data->data_p,len-1);
	destination[len]=0;
	data->data_p += len-1;
}

// Reading a string from *data_p of length len, and copying in the memory area at destination.
// then increment data_p position

static void read_string(DATA_st *data, char *destination, size_t len) {
	memcpy(destination,data->data_p,len);
	data->data_p += len;
}

// Parse a channel from data, len will be used to check if there is enough data

static int parse_channel(DATA_st *data_s, CHANNEL_st *channel)
{
	size_t i;

	read_string_nullify(data_s,channel->name,4);
	channel->unknownint = read_32(data_s);
	channel->datatype = read_32(data_s);
	read_string(data_s,channel->unknown4,4);
	channel->samples_count = read_u32(data_s);
	channel->samples_file = read_u32(data_s);
	channel->samples3 = read_u32(data_s);
	channel->timediv = get_real_timescale(read_u32(data_s));
	channel->offsety = read_32(data_s);
	channel->voltsdiv = get_real_voltscale(read_u32(data_s));
	channel->attenuation = get_real_attenuation(read_u32(data_s));
	channel->time_mul = read_f(data_s);
	channel->frequency = read_f(data_s);
	channel->period = read_f(data_s);
	channel->volts_mul = read_f(data_s);
	channel->data = (double *) calloc(channel->samples_file,sizeof(double));
	if (channel->data == NULL) {
		printf("Error: Can't allocate %d bytes of memory.\n",channel->samples_file*sizeof(int16_t));
		return 2;
	}
	for (i=0;i<channel->samples_file-1;i++) {

		if (channel->datatype == 2) {
			channel->data[i] = read_16(data_s);
		} else {
			channel->data[i] = read_char(data_s);
		}
	}

	debug_channel(channel);
}

int owon_parse(const char * const buf, size_t len, HEADER_st *header)
{
	unsigned int i;
	DATA_st data;
	DATA_st *data_s = &data;
	CHANNEL_st **channel_p;

	data_s->data = buf;
	data_s->data_p = buf;
	data_s->len = len;

	memset(header,0,sizeof(HEADER_st)); // Putting NULL in the structure for fields not present
  // This is used only with data coming over usb, we jump the first 12 bytes.
	if (strncmp((data_s->data_p)+12,"SPB",3) == 0) {
		header->length = read_32(data_s); // This is the length without the LAN header
		header->unknown1 = read_32(data_s);
		header->type = read_32(data_s); // This seems to be related to the number of parts in the file
#ifdef DEBUG_KNOWN
    printf("Debug Known: found length %d\n", header->length);
    printf("Debug Known: found unknown1 %d\n", header->unknown1);
    printf("Debug Known: found type %d\n", header->type);
#endif

	}

	read_string_nullify(data_s,header->model,sizeof(header->model));
  #ifdef DEBUG_KNOWN
  printf("Debug Known, model: %s\n", header->model);
  #endif

	header->intsize=read_32(data_s); // Not really the size ? It was in some models, maybe…
#ifdef DEBUG_KNOWN
  printf("Debug Known, size: %u\n", header->intsize);
#endif
	if (header->intsize != 0xFFFFFF && header->intsize !=0 ) { // For short files coming from official app
		read_string_nullify(data_s,header->serial,sizeof(header->serial));
#ifdef DEBUG_KNOWN
    printf("Debug Known, serial: %s\n", header->serial);
#endif

		header->triggerstatus = read_char(data_s);

		header->unknownstatus = read_char(data_s);
		header->unknownvalue1 = read_u32(data_s);
		header->unknownvalue2 = read_char(data_s);
		read_string(data_s,header->unknown3,sizeof(header->unknown3));

		//    debug_file(header);

	}

	header->channels_count = 0;

	while ((void *)data_s->data_p < (void *) data_s->data + data_s->len) {
		if (strncmp(data_s->data_p,"CH",2) == 0) {

			header->channels_count++;

			channel_p = realloc(header->channels,header->channels_count*sizeof(CHANNEL_st*));
			if (channel_p==NULL) {
				printf("Can't allocate %d bytes of memory for channel data.\n",sizeof(CHANNEL_st*) * header->channels_count);
				return(126);
			}

			header->channels = channel_p;
			header->channels[header->channels_count-1] = malloc(sizeof(CHANNEL_st));
			memset(header->channels[header->channels_count-1],0,sizeof(CHANNEL_st)); // Putting NULL in the structure for fields not present
			parse_channel(data_s,header->channels[header->channels_count-1]);
		}  else if (strncmp(data_s->data_p,"INFO",4) == 0) {

		}
		data_s->data_p++;
	}
	return(0);
}

static float sample_id_to_time(const HEADER_st *header, uint32_t sample)
{
	if (!header->channels_count)
		return -1.0;

	// return header->channels[0]->timediv * 10.0 * sample / header->channels[0]->samples_count;

	// T.S. 05NOV2021    SDS-7102:        20 DIVs    span over      all samples in the file:
	return header->channels[0]->timediv * 20.0 * sample / header->channels[0]->samples_file;


}

static float sample_to_volt(HEADER_st *header, uint8_t channel, uint32_t sample)
{
	int8_t val;

	if (channel > header->channels_count - 1)
		return -1.0;

	val = header->channels[channel]->data[sample];
	// return val * 2.0 * header->channels[channel]->voltsdiv / 5.0;
	
        // T.S. 05NOV2021    SDS-7102:       10 DIVs span over 1byte=256      and subtract offset first: 
	return (val-header->channels[channel]->offsety)*10.0/256.0  * header->channels[channel]->voltsdiv;

}

static void volt_scale_to_string(float volt_scale, uint32_t *val, const char **unit)
{
	if (volt_scale < 1) {
		*val = volt_scale * 1000;
		*unit = "mV";
	} else if (volt_scale < 1000) {
		*val = volt_scale;
		*unit = "V";
	} else {
		*val = volt_scale / 1000;
		*unit = "kV";
	}
}

static void time_scale_to_string(double time_scale, uint32_t *val, const char **unit)
{
	if (time_scale < 0.000001) {
		*val = time_scale * 1.0e9;
		*unit = "ns";
	} else if (time_scale < 0.001) {
		*val = time_scale * 1000000;
		*unit = "us";
	} else if (time_scale < 1) {
		*val = time_scale * 1000;
		*unit = "ms";
	} else {
		*val = time_scale;
		*unit = "s";
	}
}

int owon_output_csv(HEADER_st *header, FILE *file ) {
	size_t sample, channel, channels_count, samples_count;

#ifdef DEBUG_UNKNOWN
	printf("Debug Unknown activated\n");
#endif
#ifdef DEBUG_KNOWN
	printf("Debug Known activated\n");
#endif

 
	channels_count = header->channels_count;
	if (channels_count < 1) {
#ifdef DEBUG_UNKNOWN || DEBUG_KNOWN
    printf("No channels found\n");
#endif

		return 1;
  }
	samples_count = header->channels[0]->samples_file;


	/* add the header to the CSV */
	fprintf(file, "time");
	for (channel = 0; channel < channels_count; channel++) {
		const char *time_unit, *volt_unit;
		uint32_t time_val, volt_val;

		time_scale_to_string(header->channels[channel]->timediv, &time_val, &time_unit);
		volt_scale_to_string(header->channels[channel]->voltsdiv, &volt_val, &volt_unit);
		fprintf(file, ",channel %i (Att %u, %u %s/div, %u %s/div)",
			channel + 1, header->channels[channel]->attenuation,
			volt_val, volt_unit, time_val, time_unit);
	}
	fprintf(file, "\n");

	/* add the actual data */
	for (sample = 0; sample < samples_count; sample++) {
		fprintf(file, "%f", sample_id_to_time(header, sample));

		for(channel=0; channel<channels_count; channel++)
			fprintf(file, ",%f", sample_to_volt(header, channel, sample));

		fprintf(file,"\n");
	}
}

void owon_free_header(HEADER_st *header) {
	int i;
	for (i=0;i<header->channels_count;i++) {
		free(header->channels[i]->data);
		free(header->channels[i]);
	}
	free(header->channels);
}
