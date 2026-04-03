#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <strings.h>
#include <windows.h>
#include <time.h>

enum condition{
	DISCONNECTED = 0,
	CONNECTED = 1
};
typedef struct Sensor_stats{
	uint8_t ID, over_counter = 0, error_counter = 0;
	uint16_t frequency;
	char sensor_name[15];
	float data, maximum_value;
	enum condition current_state;
	char data_file_name[12];
	Sensor_stats *next = NULL;
} Sensor_stats;
// Cau truc FILE sensor: ID, Ten_sensor, Frequency (in Hz), Max_value

float receive_data_sensor(Sensor_stats *S, FILE* reportfptr);
void add_Node(Sensor_stats *S1, Sensor_stats *S2);
void delete_Node(Sensor_stats *S1, Sensor_stats *S2, Sensor_stats *S3);
Sensor_stats* init_sensor(uint8_t sensor_counter, FILE* fptr);
void update_data_to_file(Sensor_stats *S, float data);
int update_data (Sensor_stats *S, float new_data, FILE * errorfptr);
uint8_t check_invalid_data (float data, Sensor_stats* S, FILE* errorfptr);
void average_filter (Sensor_stats* S, FILE* reportfptr);
void send_actuator (Sensor_stats* S);

// ---------- MAIN FUNCTION -----------
int main(){
	time_t now = time(NULL);
	uint8_t sensor_counter = 0;
	FILE *sensorFileptr = fopen("SENSOR_FILE_.txt", "rt");
	FILE *errorFileptr = fopen("REPORT_FILE.txt", "w+");
	Sensor_stats* collection[20];
	Sensor_stats *S1 = init_sensor(sensor_counter, sensorFileptr);
	while (!feof(sensorFileptr)){
		
	}
	
	while(1){
		float input = receive_data_sensor(S1, errorFileptr);
		if(input){
			if(check_invalid_data(input, S1, errorFileptr)){
				average_filter(S1, errorFileptr);
			}
			else{
				update_data(S1, input, errorFileptr);
			}
		}
		
	}
	// Test lay max, min, avg data cua sensor
	FILE *fptr = fopen(S1->data_file_name, "r");
	rewind(fptr);
	uint16_t counter;
	float summing_data, max = 0, min= S1->maximum_value;
	while (!feof(fptr)){
		float x;
		fgets(x, sizeof(x), fptr);
		summing_data += x;
		if (max < x){
			max = x;
		}
		if (min > x){
			min = x;
		}
	}
	float avg_value = summing_data/counter;
	
	
	
	/*Sensor_stats *S2 = (Sensor_stats*)malloc(sizeof(Sensor_stats));
	Sensor_stats *S3 = (Sensor_stats*)malloc(sizeof(Sensor_stats));
	add_Node(S1, S2);
	add_Node(S2, S3);
	/*printf("Address of S2 (linked to S1): %p\n", (void *)S1->next);
	printf("Real address of S2: %p\n", (void *)S2);
	
	delete_Node(S1, S2, S3);
	/*printf("S1 now point to: %p\n", (void*)S1->next);
	printf("Real address of S3: %p\n", (void *)S3);
	printf("Real address of S2: %p\n", (void *)S2);*/
	
	fclose(sensorFileptr);
	fclose(errorFileptr);
}

//-------- Ham khoi tao Sensor ----------
Sensor_stats* init_sensor(uint8_t sensor_counter, FILE *fptr){
	fseek(fptr, 0, SEEK_SET);
	Sensor_stats* S = (Sensor_stats*)malloc(sizeof(Sensor_stats));
	S->ID = sensor_counter;
	uint8_t list_counter = 0;
	do {
		fseek(fptr, 0, SEEK_CUR);
		fscanf(fptr, "%hu", &list_counter);
		if (S->ID != list_counter){
			char s[100];
			fgets(s, 100, fptr);
		}
	} while (S->ID != list_counter);
	S->current_state = CONNECTED;
	fscanf(fptr, ", %s, %hu, %hu", &(S->sensor_name), &(S->frequency), &(S->maximum_value));
	snprintf(S->data_file_name, sizeof(S->data_file_name),"Sens%d.dat", S->ID); // tao 1 file cho tung sensor de ghi data
	return S;
}

// -------- Cac ham thao tac Nodes ------
void add_Node(Sensor_stats *S1, Sensor_stats *S2){
	S1->next = S2; 
}

void delete_Node(Sensor_stats *S1, Sensor_stats *S2, Sensor_stats *S3, FILE* reportfptr){
	S1->next = S3;
	time_t now = time(NULL);
	fprintf(reportfptr, "Sensor ID %hu named %s disconnected at %s", S2->ID, S2->sensor_name, ctime(&now));
	free(S2);
}

//------- Cac ham thao tac du lieu -------
void update_data_to_file(Sensor_stats *S, float data){ 
	FILE *fptr = fopen(S->data_file_name, "a+t");
	fprintf(fptr, "%hu\n", data);
	fclose(fptr);
}


uint8_t check_invalid_data (float data, Sensor_stats* S, FILE* errorfptr){
	if (data > (S->maximum_value)){
		time_t now = time(NULL);
		printf("Sensor ID number %hu reached overload value %f at %s\n", S->ID, data, ctime(&now));
		fprintf(errorfptr, "Sensor ID number %hu reached overload value %f at %s\n", S->ID, data, ctime(&now));
		return S->over_counter ++;
	}
	return 0;
}

int update_data (Sensor_stats *S, float new_data, FILE * errorfptr){
	if (!check_invalid_data(new_data, S, errorfptr)){
		S->data = new_data;
		update_data_to_file(S, new_data);
		send_actuator(S);
		return 1;
	}
	return 0;
}

void average_filter (Sensor_stats* S, FILE* reportfptr){
	uint8_t i = 0;
	while (i < 10){
		float inputs = receive_data_sensor(S, reportfptr);
		(S->data) += inputs;
		update_data_to_file(S, inputs);
	}
	S->data /= 10;
	send_actuator(S);
}

// ------ Cac ham gia lap nhan input va output du lieu ------
void send_actuator (Sensor_stats* S){
	printf("Received data from %s: %f", S->sensor_name, S->data);
	if (S->data > S->maximum_value){
		printf("ALERT");
	}
	return;
}

//Sensor nhan input theo chu ky nhan tin hieu
float receive_data_sensor(Sensor_stats *S, FILE *reportfptr){
	float cycle_timer = 1/(S->frequency);
	Sleep(cycle_timer*1000);
	float input;
	if (scanf("%f", &input)){
		return input;
	}
	else {
		fprintf(reportfptr, "Ivalid input to sensor ID %hu", S->ID);
		S->error_counter ++;
		return 0;
	}
}

