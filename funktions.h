/*
 * funktions.h
 *
 *  Created on: 14.11.2022
 *      Author: matti
 */



#ifndef FUNKTIONS_H_
#define FUNKTIONS_H_

//prototypes for funktions.
void clearData(float*data, int size);
int peakCount(float *time, float *data, float dataSize, float treshold, int direction, float peakTime);

int peakCountMargin(float *time, float *ax, float *ay, float *az, int dataSize, char peakAxis, float treshold, float errorMargin, float peakTime);



#endif /* FUNKTIONS_H_ */
