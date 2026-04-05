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

enum sensor_type{
	TEMP,
	HUMID,
	GAS,
	LIGHT,
	INVALID
};
typedef struct Sensor_stats{
	uint8_t ID;
	char sensor_name[15];
	enum sensor_type type;
	uint16_t frequency;
	
	float data, maximum_value;
	enum condition current_state;
	char data_file_name[12];
	
	uint16_t over_counter, error_counter, valid;
	long overflow_counter;
	
	float buffer[BUFFER_SIZE];
	uint8_t head, buffer_count;
	
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

// ---------- MAIN FUNCTION -----------
int main(){
    uint8_t sensor_counter = 0, total_sensor = 0;

    FILE *sensorFileptr = fopen("SENSOR_FILE.txt", "rt");
    if (sensorFileptr == NULL) {
        printf("Khong the mo file sensor\n");
        return 0;
    }
    FILE *errorFileptr = fopen("ERROR_FILE.txt", "w+");
    Sensor_stats* collection[MAX_SENSORS];

    // ===== LOAD SENSOR =====
    while (sensor_counter < MAX_SENSORS) {
        Sensor_stats *S = init_sensor(sensor_counter, sensorFileptr);
        if (S == NULL) break;

        collection[sensor_counter] = S;
        sensor_counter++;
        total_sensor++;
    }
    fclose(sensorFileptr);
    printf("Loaded %d sensors\n", total_sensor);

    // ===== MAIN LOOP =====
    while (1){
        for (uint8_t i = 0; i < total_sensor; i++){

            float input = receive_data_sensor(collection[i], errorFileptr);
            if (input == -1000) continue;
            
            add_data_to_buffer(collection[i], input, errorFileptr);
            if (check_invalid_data(input, collection[i], errorFileptr)){
            	apply_average_filter(collection[i], input, errorFileptr);
			}
            if (collection[i]->error_counter > MAXIMUM_ERROR || collection[i]->current_state == DISCONNECTED){
                delete_Node(collection, &total_sensor, i, errorFileptr);
                i--; 
            }
        }
    }

    // ===== REPORT  =====
    FILE *finalFileptr = fopen("REPORT_FILE.txt", "a+");
    fprintf(finalFileptr, "FINAL REPORT ---\n");
    long total_error = 0, valid_counter = 0;
    
    report_per_type(collection, total_sensor, "TEMP", finalFileptr);
    report_per_type(collection, total_sensor, "HUMID", finalFileptr);
    report_per_type(collection, total_sensor, "GAS", finalFileptr);
    report_per_type(collection, total_sensor, "LIGHT", finalFileptr);
    
    for (int i = 0; i < total_sensor; i++){
    	total_error += collection[i]->error_counter;
    	valid_counter += collection[i]->valid;
        report(collection[i], finalFileptr);
    }
    most_error(collection, &total_sensor, errorFileptr);
    fprintf(finalFileptr, "\nTotal error: %ld\nTotal valid: %ld\n", total_error, valid_counter);
    fclose(errorFileptr);
    fclose(finalFileptr);
    return 0;
}


//-------- Ham khoi tao Sensor ----------
Sensor_stats* init_sensor(uint8_t sensor_counter, FILE *sensorfptr){
	fseek(sensorfptr, 0, SEEK_CUR);
	Sensor_stats* S = (Sensor_stats*)malloc(sizeof(Sensor_stats));
	S->ID = sensor_counter;
	char type[5];
	sensor_type s;
	uint8_t verifier = fscanf(sensorfptr, " %s %s %hu %f", S->sensor_name, type, &S->frequency, &S->maximum_value);
	s = parse_sensor_type(type);
	if (s != INVALID){
		S->type = s;
	}
	if (verifier != NUMBER_OF_STATS)    return NULL;
	S->current_state = CONNECTED;
	S->error_counter = 0;
	S->over_counter = 0;
	S->overflow_counter = 0;
	S->valid = 0;
	
	S->buffer_count = 0;
	S->head = 0;
	memset(S->buffer, 0, sizeof(S->buffer));
	
	S->max = 0;
	S->min = S->maximum_value;
	snprintf(S->data_file_name, sizeof(S->data_file_name),"Sens%d.dat", S->ID); // tao 1 file cho tung sensor de ghi data
	return S;
}

// Convert sensor type in FILE to enum data type
enum sensor_type parse_sensor_type (const char* str){
	if (strcmp(str, "TEMP") == 0) return TEMP;
    if (strcmp(str, "HUMID") == 0)  return HUMID;
    if (strcmp(str, "GAS") == 0)  return GAS;
    if (strcmp(str, "LIGHT") == 0) return LIGHT;
    
    return INVALID;
}

// -------- Cac ham thao tac Nodes ------
void add_Node(Sensor_stats *collection[], uint8_t sensor_counter, uint8_t* total_sensor, FILE* sensorfptr){
	if (sensor_counter == MAX_SENSORS){
		printf("MAXIMUM NUMBER OF SENSOR REACHED");
		return;
	}
	rewind(sensorfptr);
	for (uint8_t i = 0; i < sensor_counter; i++){
		char s[100];
		fgets(s, sizeof(s), sensorfptr);
	}
	collection[sensor_counter] = init_sensor(sensor_counter + 1, sensorfptr);
	(*total_sensor) ++;
}

void delete_Node(Sensor_stats *collection[], uint8_t *total_sensor, int position, FILE* reportfptr){
    if (position < 0 || position >= *total_sensor) return;
    time_t now = time(NULL);
    fprintf(reportfptr,
        "Sensor ID %hu named %s disconnected at %s",
        collection[position]->ID,
        collection[position]->sensor_name,
        ctime(&now));
	report(collection[position], reportfptr);
    free(collection[position]);
    for (int i = position; i < *total_sensor - 1; i++){
        collection[i] = collection[i + 1];
    }

    (*total_sensor)--;
}


//------- Cac ham thao tac du lieu -------
// Update data vao trong file ghi du lieu rieng tung sensor
void update_data_to_file(Sensor_stats *S, float data){ 
	calculate_max_min(S, data);
	FILE *fptr = fopen(S->data_file_name, "a+t");
	fprintf(fptr, "%f\n", data);
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
	update_data_to_file(S, data);
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
	double cycle_timer = 1/(S->frequency);
	Sleep(cycle_timer*1000);
	float input;
	S->valid++;
	while (getchar() != '\n');
	if (scanf("%f", &input)){
		return input;
	}
	else {
		fprintf(reportfptr, "Ivalid input to sensor ID %hu", S->ID);
		S->error_counter ++;
		return -1000;
	}
}

// --------- Cac ham ghi report -----------
void calculate_max_min(Sensor_stats* S, float data){
	if (S->max < data)   { S->max = data; }
	if (S->min > data)   { S->min = data; }
	return;
}

void most_error(Sensor_stats* collection[], uint8_t* total_sensor, FILE* errorfptr){
	fprintf(errorfptr, "\n----- Most error -----\n");
	int most_error = 0;
	for (uint8_t i = 0; i < *total_sensor; i++){
		if (most_error < collection[i]->error_counter){
			most_error = collection[i]->error_counter;
		}
	}
	for (uint8_t i = 0; i < *total_sensor; i++){
		if (most_error == collection[i]->error_counter){
			fprintf(errorfptr, "Sensor with the most error: %s, ID: %hu\n-------\n", collection[i]->sensor_name, collection[i]->ID);
		}
	}
}

void report(Sensor_stats* S, FILE* reportfptr){
	FILE *sensorfptr = fopen(S->data_file_name, "r");
	rewind(sensorfptr);
	uint16_t counter = 0;
	double summing_data = 0;
	float x;
	while (fscanf(sensorfptr, "%f", &x) == 1){
		summing_data += x;
		counter++;
	}
	float avg_value = summing_data/counter;
	fprintf(reportfptr, "\n------------\n");
	fprintf(reportfptr, "Sensor ID %hu:\n  So luong ban tin loi: %hu\n  So lan vuot nguong: %hu\n  So lan tran bo dem:%ld\n  GTLN/GTNN/GTTB: %f/%f/%f\n",S->ID, S->error_counter, S->over_counter, S->overflow_counter, S->max, S->min, avg_value);
	fprintf(reportfptr, "\n------------\n");
	fclose(sensorfptr);
}

void report_per_type(Sensor_stats* collection[], uint8_t total_sensor, const char* str, FILE* reportfptr){
	sensor_type type = parse_sensor_type(str);
    if (type == INVALID){
        fprintf(reportfptr, "Invalid sensor type: %s\n", str);
        return;
    }
    fprintf(reportfptr, "\n---- Report for %s sensors type ------\n", str);
    for (uint8_t i = 0; i < total_sensor; i++){
        if (collection[i] == NULL) continue;
        if (collection[i]->type == type){
            report(collection[i], reportfptr);
        }
    }
}

/***NOTE:
	- Xử lý mất mẫu
*/