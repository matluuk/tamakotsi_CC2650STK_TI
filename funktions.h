/*
 * funktions.h
 *
 *  Created on: 14.11.2022
 *      Author: matti
 */



#ifndef FUNKTIONS_H_
#define FUNKTIONS_H_

#include <inttypes.h>

//prototypes for funktions.
int peakCount(float *time, float *data, float dataSize, float treshold, int direction, float peakTime);
int peakCountMargin(float *time, float *ax, float *ay, float *az, int dataSize, char peakAxis, float treshold, float errorMargin, float peakTime);
void clearData(float*data, int size);
float average(float *data, int dataSize);
void movavg(float *array, uint8_t array_size, uint8_t window_size, float *output);



#endif /* FUNKTIONS_H_ */
