#include "SENSORLIB1.h"
#include <unistd.h>

Sensor_stats* init_sensor(uint8_t sensor_counter, FILE *sensorfptr){
	fseek(sensorfptr, 0, SEEK_CUR);
	Sensor_stats* S = (Sensor_stats*)malloc(sizeof(Sensor_stats));
	S->ID = sensor_counter;
	char type[10];
	sensor_type s;
	uint8_t verifier = fscanf(sensorfptr, " %s %s %hu %f", S->sensor_name, type, &S->frequency, &S->maximum_value);
	s = parse_sensor_type(type);
	if (s != INVALID){
		S->type = s;
	}
	else{
		free(S);
		return NULL;
	}
	if (verifier != NUMBER_OF_STATS)    return NULL;
	S->current_state = CONNECTED;
	S->error_counter = 0;
	S->over_counter = 0;
	S->overflow_counter = 0;
	S->valid = 0;
	
	S->buffer_count = 0;
	S->head = S->tail = 0;
	memset(S->buffer, 0, sizeof(S->buffer));
	
	S->max = 0;
	S->min = S->maximum_value;
	snprintf(S->data_file_name, sizeof(S->data_file_name),"Sens%d.dat", S->ID); // tao 1 file cho tung sensor de ghi data
	return S;
}

// Convert sensor type in FILE to enum data type
sensor_type parse_sensor_type (const char* str){
	if (!strcmp(str, "TEMP")) return TEMP;
    if (!strcmp(str, "HUMID"))  return HUMID;
    if (!strcmp(str, "GAS"))  return GAS;
    if (!strcmp(str, "LIGHT")) return LIGHT;
    
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

void add_data_to_buffer(Sensor_stats* S, float data, FILE* ef) {
    if (S->buffer_count == BUFFER_SIZE) {
        S->overflow_counter++;
        S->tail = (S->tail + 1) % BUFFER_SIZE;
    } else {
        S->buffer_count++;
    }
    S->buffer[S->head] = data;
    S->head = (S->head + 1) % BUFFER_SIZE;

    // CHỈ GHI FILE TẠI ĐÂY THÔI NHÉ
    update_data_to_file(S, data); 
}

uint8_t check_invalid_data(float data, Sensor_stats* S, FILE* ef) {
    if (data > S->maximum_value) {
        S->over_counter++;
        fprintf(ef, "[OVERLOAD] ID %d: %.2f\n", S->ID, data);
        return 1;
    }
    return 0; 
   
}


void apply_average_filter (Sensor_stats* S, float data, FILE* errorfptr){
    if (S->buffer_count == 0) return;

    double sum = 0;
    int count = 0;

    for (uint8_t i = 0; i < S->buffer_count; i++) {
        if (S->buffer[i] != INVALID_DATA){
            sum += S->buffer[i];
            count++;
        }
    }

    if (count == 0) return; // tránh chia 0

    S->data = (float)(sum / count);

}

// ------ Cac ham gia lap nhan input va output du lieu ------
void send_actuator (Sensor_stats* S){
	printf("Received data from S%hu: %.2f\n", S->ID, S->data);
	printf("DEBUG: actuator called\n");
	if (S->data > S->maximum_value){
		printf("ALERT\n");
	}
}

//Sensor nhan input theo chu ky nhan tin hieu
//float receive_data_sensor(Sensor_stats *S, FILE* reportfptr){

    //float input;

   // input = (rand() % 1000) / 10.0;

   // printf("AUTO DATA sensor %d: %.2f\n", S->ID, input);

   // S->valid++;
  // return input;
//}
float receive_data_sensor(Sensor_stats *S, FILE* reportfptr){
    char line[100];
    float input;

    usleep(1000 / (S->frequency > 0 ? S->frequency : 1));

    printf("Nhap du lieu cho sensor %d: ", S->ID);

    while (1) {
        if (!fgets(line, sizeof(line), stdin)) {
            fprintf(reportfptr, "Input stream error at sensor ID %hu\n", S->ID);
            S->error_counter++;
            return -1000;
        }

        if (line[0] == '\n') {
    fprintf(reportfptr, "MISS sensor ID %hu\n", S->ID);
    S->error_counter++;
    return -1000;
}

char extra;

if (sscanf(line, " %f %c", &input, &extra) == 1){
    printf("READ: %.2f\n", input);
    S->valid++;
    return input;
} 
else {
    fprintf(reportfptr, "INVALID FORMAT sensor ID %hu: %s", S->ID, line);
    S->error_counter++;
    return -1000;
}
}
}
// --------- Cac ham ghi report -----------
void calculate_max_min(Sensor_stats* S, float data){
	if (S->max < data)   { S->max = data; }
	if (S->min > data)   { 
		if(data != INVALID_DATA){
		S->min = data; 
		}
	}
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
	float x, avg_value;
	while (fscanf(sensorfptr, "%f", &x) == 1){
		if(x != INVALID_DATA){
		summing_data += x;
		counter++;
		}
	}
	if(counter == 0){
    fprintf(reportfptr, "\n------------\n");
    fprintf(reportfptr,
        "Sensor ID %hu:\n"
        "  So luong ban tin loi: %hu\n"
        "  So lan vuot nguong: %hu\n"
        "  So lan tran bo dem:%ld\n"
        "  GTLN/GTNN/GTTB: NaN/NaN/NaN (No valid data)\n",
        S->ID, S->error_counter, S->over_counter,S->overflow_counter);
    fprintf(reportfptr, "\n------------\n");
    fclose(sensorfptr);
    return;

	}	else{
	avg_value = summing_data/counter;
	fprintf(reportfptr, "\n------------\n");
	fprintf(reportfptr, "Sensor ID %hu:\n  So luong ban tin loi: %hu\n  So lan vuot nguong: %hu\n  So lan tran bo dem:%ld\n  GTLN/GTNN/GTTB: %f/%f/%f\n",S->ID, S->error_counter, S->over_counter, S->overflow_counter, S->max, S->min, avg_value);
	fprintf(reportfptr, "\n------------\n");
	fclose(sensorfptr);
	}
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
