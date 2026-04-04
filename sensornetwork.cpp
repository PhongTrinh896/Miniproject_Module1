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
	uint8_t ID, over_counter, error_counter;
	uint16_t frequency;
	char sensor_name[15];
	float data, maximum_value;
	enum condition current_state;
	char data_file_name[12];
	Sensor_stats *next;
} Sensor_stats;
// Cau truc FILE sensor: ID Ten_sensor Frequency(in Hz) Max_value

float receive_data_sensor(Sensor_stats *S, FILE* reportfptr);
void add_Node(Sensor_stats *S1, Sensor_stats *S2);
void delete_Node(Sensor_stats *S1, Sensor_stats *S2, Sensor_stats *S3);
Sensor_stats* init_sensor(uint8_t sensor_counter, FILE* fptr);
void update_data_to_file(Sensor_stats *S, float data);
int update_data (Sensor_stats *S, float new_data, FILE * errorfptr);
uint8_t check_invalid_data (float data, Sensor_stats* S, FILE* errorfptr);
void average_filter (Sensor_stats* S, FILE* reportfptr);
void send_actuator (Sensor_stats* S);
void report(Sensor_stats* S, FILE* reportfptr);

// ---------- MAIN FUNCTION -----------
int main(){
	time_t now = time(NULL);
	uint8_t sensor_counter = 0;
	FILE *sensorFileptr = fopen("SENSOR_FILE_.txt", "rt");
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
	
	
	while(1){
		float input = receive_data_sensor(collection[0], errorFileptr);
		if(input){
			if(check_invalid_data(input, collection[0], errorFileptr)){
				average_filter(collection[0], errorFileptr);
			}
			else{
				update_data(collection[0], input, errorFileptr);
			}
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
	
	fclose(sensorFileptr);
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
	S->next = NULL;
	fscanf(fptr, " %s %hu %f", S->sensor_name, S->frequency, S->maximum_value);
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

// --------- Cac ham ghi report -----------

void report(Sensor_stats* S, FILE* reportfptr){
	FILE *sensorfptr = fopen(S->data_file_name, "r");
	rewind(sensorfptr);
	uint16_t counter;
	double summing_data;
	float max = 0, min= S->maximum_value;
	while (!feof(sensorfptr)){
		float x;
		fscanf(sensorfptr, "%f", &x);
		summing_data += x;
		if (max < x)   { max = x; }
		if (min > x)   { min = x; }
	}
	float avg_value = summing_data/counter;
	fprintf(reportfptr, "Sensor ID %hu:\n  So luong ban tin loi: %hu\n  So lan vuot nguong: %hu\n  GTLN/GTNN/GTTB: %f/%f/%f\n", S->error_counter, S->over_counter, max, min, avg_value);
	fclose(sensorfptr);
}
