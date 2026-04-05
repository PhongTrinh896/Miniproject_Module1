#ifndef SENSORLIB
#define SENSORLIB

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>
#include <time.h>

#define MAX_SENSORS 30
#define BUFFER_SIZE 10
#define NUMBER_OF_STATS 4
#define MAXIMUM_ERROR 20

enum condition{
	DISCONNECTED = 0,
	CONNECTED = 1
};

typedef enum {
	TEMP,
	HUMID,
	GAS,
	LIGHT,
	INVALID
} sensor_type;
typedef struct Sensor_stats{
	uint8_t ID;
	char sensor_name[15];
	sensor_type type;
	uint16_t frequency;
	
	float data, maximum_value;
	enum condition current_state;
	char data_file_name[12];
	
	uint16_t over_counter, error_counter, valid;
	long overflow_counter;
	
	float buffer[BUFFER_SIZE];
	uint8_t head, tail, buffer_count;
	
	float max, min;
} Sensor_stats;

// Cau truc FILE sensor: STT() Ten_sensor Loai_sensor Frequency(in Hz) Max_value
void calculate_max_min(Sensor_stats* S, float data);
float receive_data_sensor(Sensor_stats *S, FILE* reportfptr);
void send_actuator (Sensor_stats* S);
void add_data_to_buffer (Sensor_stats* S, float data, FILE* errorfptr);

sensor_type parse_sensor_type (const char* str);
Sensor_stats* init_sensor(uint8_t sensor_counter, FILE* fptr);
void add_Node(Sensor_stats *collection[], uint8_t sensor_counter, uint8_t* total_sensor, FILE* sensorfptr);
void delete_Node(Sensor_stats *collection[], uint8_t *total_sensor, int position, FILE* reportfptr);

void update_data_to_file(Sensor_stats *S, float data);
uint8_t check_invalid_data (float data, Sensor_stats* S, FILE* errorfptr);
void apply_average_filter (Sensor_stats* S, float data, FILE* errorfptr);

void most_error(Sensor_stats* collection[], uint8_t* total_sensor, FILE* errorfptr);
void report(Sensor_stats* S, FILE* reportfptr);
void report_per_type(Sensor_stats* collection[], uint8_t total_sensor, const char* str, FILE* reportfptr);

#endif