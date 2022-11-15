/*
 * funktions.h
 *
 *  Created on: 14.11.2022
 *      Author: matti
 */



#ifndef FUNKTIONS_H_
#define FUNKTIONS_H_

//prototypes for funktions.
/*
 * Play music
 */
void playMusic(PIN_Handle buzzerPin, int *note, int tempo);
/*
 * send data to debug console.
 */
void sendData();
void clearData(float*data, int size);
int peakCount(float *data, int size, float treshold);



#endif /* FUNKTIONS_H_ */
