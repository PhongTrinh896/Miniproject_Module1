#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <time.h>
#include "SENSORLIB1.h"

#define MAX_SENSORS 30
#define BUFFER_SIZE 10
#define NUMBER_OF_STATS 4
#define MAXIMUM_ERROR 20
#define INVALID_DATA -9999

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
                apply_average_filter(collection[i], input, errorFileptr);
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
    fclose(finalFileptr);

    return 0;
}
