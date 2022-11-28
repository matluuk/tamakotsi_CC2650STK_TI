/*
 * funktions.h
 *
 *  Created on: 14.11.2022
 *      Author: matti
 */



#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <inttypes.h>

/// @brief calculates peak count in data. Made for calculating acceleration peaks from ti sensorTag mpu 9250 sensor.
/// @param time time data
/// @param data acceleration data
/// @param dataSize time and acceleration data size. Both sould be same
/// @param treshold Treshold for data value to be counted as peak
/// @param tresholdOffset Value from treshold is calculated
/// @param direction if < 0 function calculates negative peaks. 
///     if > 0 funktion calculates positive peaks
///     if = 0 fuction calculates both peaks
/// @param peakTime Time what data value must be over treshold
/// @return count of peaks. Always positive. 0 if no peaks found.
int peakCount(float *time, float *data, float dataSize, float treshold, float tresholdOffset, int direction, float peakTime);

/// @brief Calculates peak count of chosen axis. calculates averages for treshold offsets.
/// @param time time data
/// @param testAxis data of test axis.
/// @param errorAxis1 data of first secondary axis
/// @param errorAxis2 data of second secondary axis
/// @param dataSize size of time, testAxis, errorAxis1 and errorAxis2.
/// @param treshold Treshold for data value to be counted as peak
/// @param peakTime time which testAxis value must be over treshold
/// @param errorMargin peak treshold for error test
/// @param errorTime time for error to be counted as error
/// @return returns peak count of testAxis if error margin is not exeed.
///     -1 if errorAxis1 exeed errorMargin
///     -2 if errorAxis2 exeed errorMargin
int peakCountMargin(float *time, float *testAxis, float *errorAxis1, float *errorAxis2, int dataSize, float treshold, float peakTime, float errorMargin, float errorTime);

/// @brief Sts all data list values to 0
/// @param data float list of data
/// @param size size of data list
void clearData(float*data, int size);

/// @brief Calculate average of list of floats
/// @param data pointer to float data list
/// @param dataSize size of list
/// @return sum of all data / dataSize
float average(float *data, int dataSize);

/// @brief Function changes array values to moving averages of original values
/// @param array Array for data
/// @param array_size size of array
/// @param window_size count of how many values ar considered for each average
/// @param start_value persumable start value
void movavg(float *array, uint8_t array_size, uint8_t window_size, float start_value);



#endif /* FUNKTIONS_H_ */
