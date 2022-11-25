/*
 * funktions.h
 *
 *  Created on: 14.11.2022
 *      Author: matti
 */



#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <inttypes.h>

//prototypes for funktions.
int peakCount(float *time, float *data, float dataSize, float treshold, float tresholdOffset, int direction, float peakTime);
int peakCountMargin(float *time, float *testAxis, float *errorAxis1, float *errorAxis2, int dataSize, float treshold, float peakTime, float errorMargin, float errorTime);
void clearData(float*data, int size);
float average(float *data, int dataSize);
void movavg(float *array, uint8_t array_size, uint8_t window_size);



#endif /* FUNKTIONS_H_ */
