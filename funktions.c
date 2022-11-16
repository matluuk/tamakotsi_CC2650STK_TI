/*
 * funktions.c
 *
 *  Created on: 14.11.2022
 *      Author: matti
 */

int peakCount(float *time, float *data, int dataSize, float treshold, float peakTime){
    int count = 0;
    int peakCount = 0;
    int i;
    for (i = 0; i < dataSize; i++ ){
        if (data[i] > treshold) {
            count++;
        } else if (count){
            if((time[i] - time[i - count]) >= peakTime){
                peakCount++;
            }
            count = 0;
        } else {
            count = 0;
        }
    }
    return peakCount;
}

void clearData(float *data, int size){
    int i;
    for (i = 0; i < size; i++){
        data[i] = 0;
    }
}
