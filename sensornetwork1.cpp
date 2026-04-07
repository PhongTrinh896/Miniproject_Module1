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
#define MAXIMUM_ERROR 5
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
while(1) {

    printf("\n===== SAMPLING ROUND =====\n" );
    
    for (uint8_t i = 0; i < total_sensor; i++) {
        
        Sensor_stats *S = collection[i];
        printf("Processing sensor %d\n", i);
        if (S->current_state == DISCONNECTED) {
            delete_Node(collection, &total_sensor, i, errorFileptr);
            i--;
            continue;
        }

        float input = receive_data_sensor(S, errorFileptr);

        if (input == -1000) {
            add_data_to_buffer(S, INVALID_DATA, errorFileptr);
            continue;
        }

        add_data_to_buffer(S, input, errorFileptr);

        if (check_invalid_data(input, S, errorFileptr)) {
            apply_average_filter(S, input, errorFileptr);
            send_actuator(S);
        }
        else {
            send_actuator(S);
        }

        if (S->error_counter > MAXIMUM_ERROR || S->current_state == DISCONNECTED) {

            delete_Node(collection, &total_sensor, i, errorFileptr);
            i--;
        }
    }


    printf("\n===== FULL REPORT =====\n");

printf("--------------------------------------------------------------\n");
printf("| ID | Error | Over | Overflow |   Max   |   Min   |   Avg   |\n");
printf("--------------------------------------------------------------\n");

long total_error = 0, valid_counter = 0;

// report theo loại
report_per_type(collection, total_sensor, "TEMP", stdout);
report_per_type(collection, total_sensor, "HUMID", stdout);
report_per_type(collection, total_sensor, "GAS", stdout);
report_per_type(collection, total_sensor, "LIGHT", stdout);

// report từng sensor
printf("---- Report for each sensors ------\n");
for (int i = 0; i < total_sensor; i++) {
    total_error += collection[i]->error_counter;
    valid_counter += collection[i]->valid;
    report(collection[i], stdout);
}

// sensor lỗi nhiều nhất
most_error(collection, &total_sensor, stdout);

// tổng kết
printf("\nTotal error: %ld\nTotal valid: %ld\n",total_error, valid_counter);
       sleep(10);

}
    return 0;

} 
