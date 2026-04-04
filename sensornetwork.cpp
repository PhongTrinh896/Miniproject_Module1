#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <strings.h>
#include <windows.h>
#include <time.h>

#define MAX_SENSORS 30
#define BUFFER_SIZE 10
#define NUMBER_OF_STATS 3

enum condition{
	DISCONNECTED = 0,
	CONNECTED = 1
};
typedef struct Sensor_stats{
	uint8_t ID;
	char sensor_name[15];
	uint16_t frequency;
	
	float data, maximum_value;
	enum condition current_state;
	char data_file_name[12];
	Sensor_stats *next;
	
	uint16_t over_counter, error_counter, overflow_counter;
	
	float buffer[BUFFER_SIZE];
	uint8_t head, buffer_count;
} Sensor_stats;

// Cau truc FILE sensor: ID Ten_sensor Frequency(in Hz) Max_value

float receive_data_sensor(Sensor_stats *S, FILE* reportfptr);
void add_data_to_buffer (Sensor_stats* S, float data, FILE* errorfptr);
void add_Node(Sensor_stats *S1, Sensor_stats *S2);
void delete_Node(Sensor_stats *S1, Sensor_stats *S2, Sensor_stats *S3);
Sensor_stats* init_sensor(uint8_t sensor_counter, FILE* fptr);
void update_data_to_file(Sensor_stats *S, float data);
void check_invalid_data (float data, Sensor_stats* S, FILE* errorfptr);
void apply_average_filter (Sensor_stats* S,float data, FILE* reportfptr);
void send_actuator (Sensor_stats* S);
void report(Sensor_stats* S, FILE* reportfptr);

// ---------- MAIN FUNCTION -----------
int main(){
	time_t now = time(NULL);
	uint8_t sensor_counter = 0;
	FILE *sensorFileptr = fopen("SENSOR_FILE.txt", "rt");
	if (sensorFileptr == NULL) {
        printf("Khong the mo tep cau hinh SENSOR_FILE. Vui long kiem tra lai.\n");
        return 0;
    }
	FILE *errorFileptr = fopen("ERROR_FILE.txt", "w+");
	
	Sensor_stats* collection[30];
	//Sensor_stats *S1 = init_sensor(sensor_counter, sensorFileptr);
	while (!feof(sensorFileptr) && sensor_counter < 30){
		// Tim cach khai bao array chua nhieu sensor theo file de noi cac nodes
		collection[sensor_counter] = init_sensor(sensor_counter, sensorFileptr);
    	sensor_counter++;
    	if (sensor_counter >0){
    		add_Node(collection[sensor_counter - 1], collection[sensor_counter]);
		}
	}
	fclose(sensorFileptr);
	
	while(1){
		float input = receive_data_sensor(collection[0], errorFileptr);
		if(input){
		}
		
	}
	
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
	FILE *finalFileptr = fopen("REPORT_FILE.txt", "a+");
	fseek(finalFileptr, 0, SEEK_END);
	fprintf(finalFileptr, "FINAL REPORT ---\n");
	report(collection[0], finalFileptr);
	
	
	fclose(errorFileptr);
	fclose(finalFileptr);
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
	S->error_counter = 0;
	S->over_counter = 0;
	S->overflow_counter = 0;
	S->next = NULL;
	
	S->buffer_count = 0;
	S->head = 0;
	S->buffer = {0};
	
	uint8_t verifier = fscanf(fptr, " %s %hu %f", S->sensor_name, S->frequency, S->maximum_value);
	if (verifier != NUMBER_OF_STATS)    return NULL;
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
// Update data vao trong file ghi du lieu rieng tung sensor
void update_data_to_file(Sensor_stats *S, float data){ 
	FILE *fptr = fopen(S->data_file_name, "a+t");
	fprintf(fptr, "%hu\n", data);
	fclose(fptr);
}

void add_data_to_buffer (Sensor_stats* S, float data, FILE* errorfptr){
	if (S->buffer_count == BUFFER_SIZE) {
        S->overflow_counter++;
        fprintf(errorfptr, "[OVERFLOW] ID %hu: Bo dem day, dang ghi de mau cu\n", S->ID);
    } 
	else {
        S->buffer_count++;
    }

    S->buffer[S->head] = data;
    S->head = (S->head + 1) % BUFFER_SIZE;
    update_data_to_file(S, data);
}

// Ham check vuot nguong
uint8_t check_invalid_data (float data, Sensor_stats* S, FILE* errorfptr){
	if (data > (S->maximum_value)){
		time_t now = time(NULL);
		printf("Sensor ID number %hu reached overload value %f at %s\n", S->ID, data, ctime(&now));
		fprintf(errorfptr, "Sensor ID number %hu reached overload value %f at %s\n", S->ID, data, ctime(&now));
		S->over_counter ++;
		
		apply_average_filter(S, data, errorfptr);
		return 1;
	}
	return 0;
}


void apply_average_filter (Sensor_stats* S, float data, FILE* errorfptr){
	S->data = 0;
	for (uint8_t i = 0; i < BUFFER_SIZE; i++){
		(S->data) += S->buffer[i];
	}
	S->data /= BUFFER_SIZE;
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

// --------- Cac ham ghi report -----------

void report(Sensor_stats* S, FILE* reportfptr){
	FILE *sensorfptr = fopen(S->data_file_name, "r");
	rewind(sensorfptr);
	uint16_t counter = 0;
	double summing_data;
	float max = 0, min= S->maximum_value;
	while (!feof(sensorfptr)){
		float x;
		fscanf(sensorfptr, "%f", &x);
		summing_data += x;
		if (max < x)   { max = x; }
		if (min > x)   { min = x; }
		counter++;
	}
	float avg_value = summing_data/counter;
	fprintf(reportfptr, "Sensor ID %hu:\n  So luong ban tin loi: %hu\n  So lan vuot nguong: %hu\n  GTLN/GTNN/GTTB: %f/%f/%f\n", S->error_counter, S->over_counter, max, min, avg_value);
	fclose(sensorfptr);
}
