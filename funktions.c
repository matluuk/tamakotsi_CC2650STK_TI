/*
 * funktions.c
 *
 *  Created on: 14.11.2022
 *      Author: matti
 */

#include <inttypes.h>

int peakCount(float *time, float *data, float dataSize, float treshold, float tresholdOffset, int direction, float peakTime){
    int countPos = 0;
    int peakCountPos = 0;
    int countNeg = 0;
    int peakCountNeg = 0;
    int i;
    for (i = 0; i < dataSize; i++ ){
        if (direction >= 0){
            //This is for positive peaks
            if (data[i] - tresholdOffset >= treshold) {
                countPos++;
            } else if (countPos){
                //check if peak is long enough
                if((time[i] - time[i - countPos]) >= peakTime){
                    peakCountPos++;
                }
                countPos = 0;
            }
        }
        if (direction <= 0){
            //This is for negative peaks
            if (data[i] <= -treshold) {
                countNeg++;
            } else if (countNeg){
                //check if peak is long enough
                if((time[i] - time[i - countNeg]) >= peakTime){
                    peakCountNeg++;
                }
                countNeg = 0;
            }
        }
    }
    return peakCountPos + peakCountNeg;
}


int peakCountMargin(float *time, float *ax, float *ay, float *az, int dataSize, char peakAxis, float treshold, float errorMargin, float peakTime){
    float *testAxis;
    float *axis1;
    float *axis2;
    switch (peakAxis)
    {
    case 'x':
        testAxis = ax;
        axis1 = ay;
        axis2 = az;
        break;
    case 'y':
        testAxis = ay;
        axis1 = ax;
        axis2 = az;
        break;
    case 'z':
        testAxis = az;
        axis1 = ax;
        axis2 = ay;
        break;
    default:
        //printf("invalid peakAxis. Only (x, y, z) are allowed.");
        return -2;
    }

    //Calculate averages
    float axis1Avg = average(axis1, dataSize);
    float axis2Avg = average(axis2, dataSize);

    //Check if other axis values are in error Margin
    if (!peakCount(time, axis1, dataSize, errorMargin, axis1Avg, 0, 0) && !peakCount(time, axis2, axis2Avg, dataSize, errorMargin, 0, 0)){
        //Calculate peak count for desired axis
        return peakCount(time, testAxis, dataSize, treshold, 0, 1, 0);
    }
    return -1;
}

void clearData(float *data, int size){
    int i;
    for (i = 0; i < size; i++){
        data[i] = 0;
    }
}

float average(float *data, int dataSize){
    float sum = 0;
    int i;
    for(i = 0; i < dataSize; i++){
        sum += *(data + i);
    }
    return (sum / dataSize);
}

void movavg(float *array, uint8_t array_size, uint8_t window_size, float *output){
    uint8_t new_array_size = array_size - window_size + 1;
    float sum = 0;
    int i;
    for (i = 0; i < array_size; i++){

        sum += array[i];
        if (i >= window_size){
            sum -= array[i - window_size];
        }
        output[i] = sum / window_size;
    }
}
