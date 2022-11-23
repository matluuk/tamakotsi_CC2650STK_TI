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
            if ((data[i] - tresholdOffset) >= treshold) {
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
            //this is for negative peaks
            if ((data[i] - tresholdOffset) >= treshold) {
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


int peakCountMargin(float *time, float *testAxis, float *errorAxis1, float *errorAxis2, int dataSize, float treshold, float peakTime, float errorMargin, float errorTime){
    int result = -1;
    //Calculate averages
    float testAxisAvg = average(*testAxis, dataSize);
    float errorAxis1Avg = average(*errorAxis1, dataSize);
    float errorAxis2Avg = average(*errorAxis2, dataSize);


    //Check if other axis values are in error Margin
    if (peakCount(time, errorAxis1, dataSize, errorMargin, errorAxis1Avg, 0, errorTime) != 0){
        result = -1;
    }
    else if (peakCount(time, errorAxis2, dataSize, errorMargin, errorAxis2Avg, 0, errorTime) != 0){
        result = -2;
    } else {
        //Calculate peak count for desired axis
        result = peakCount(time, testAxis, dataSize, treshold, testAxisAvg, 1, peakTime);
    }
    return result;
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
