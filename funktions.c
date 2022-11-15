/*
 * funktions.c
 *
 *  Created on: 14.11.2022
 *      Author: matti
 */


int peakCount(float *data, int size, float treshold){
    int count = 0;
    int peakCount = 0;
    int i;
    for (i = 0; i < size; i++ ){
        if (data[i] > treshold) {
            count++;
        } else if (count){
            peakCount++;
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
