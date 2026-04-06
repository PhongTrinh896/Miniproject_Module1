#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <time.h>

#define MAX_SENSORS 30
#define BUFFER_SIZE 10
#define NUMBER_OF_STATS 4
#define MAXIMUM_ERROR 20
#define INVALID_DATA -9999

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

// ===== FUNCTION PROTOTYPES =====
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

// ---------- MAIN FUNCTION -----------//
int main(){
    uint8_t sensor_counter = 0, total_sensor = 0;

    FILE *sensorFileptr = fopen("SENSOR_FILE.txt", "rt");
    if (sensorFileptr == NULL) {
        printf("Khong the mo file sensor\n");
        return 0;
    }

    FILE *errorFileptr = fopen("ERROR_FILE.txt", "w+");
    if (errorFileptr == NULL){
        printf("Khong the mo file error\n");
        fclose(sensorFileptr);
        return 0;
    }

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
    for (int loop = 0; loop < 5; loop++) {

        for (uint8_t i = 0; i < total_sensor; i++) {

            printf("Dang xu ly sensor %d\n", i);

            float input = receive_data_sensor(collection[i], errorFileptr);

            if (input == -1000) {
                add_data_to_buffer(collection[i], INVALID_DATA, errorFileptr);
                continue;
            }

            add_data_to_buffer(collection[i], input, errorFileptr);

            if (check_invalid_data(input, collection[i], errorFileptr)) {
                // có thể xử lý thêm nếu cần
            }

            if (collection[i]->error_counter > MAXIMUM_ERROR ||
                collection[i]->current_state == DISCONNECTED) {

                delete_Node(collection, &total_sensor, i, errorFileptr);
                i--; // tránh skip phần tử sau khi xóa
            }
        }
    }

    // ===== REPORT =====
    FILE *finalFileptr = stdout;

    fprintf(finalFileptr, "FINAL REPORT ---\n");

    long total_error = 0, valid_counter = 0;

    report_per_type(collection, total_sensor, "TEMP", finalFileptr);
    report_per_type(collection, total_sensor, "HUMID", finalFileptr);
    report_per_type(collection, total_sensor, "GAS", finalFileptr);
    report_per_type(collection, total_sensor, "LIGHT", finalFileptr);

    for (int i = 0; i < total_sensor; i++) {
        total_error += collection[i]->error_counter;
        valid_counter += collection[i]->valid;
        report(collection[i], finalFileptr);
    }

    most_error(collection, &total_sensor, errorFileptr);

    printf("DEBUG NEW CODE\n");

    fprintf(finalFileptr,
            "\nTotal error: %ld\nTotal valid: %ld\n",
            total_error, valid_counter);

    fclose(errorFileptr);

    return 0;
}
