/*
 * funktions.c
 *
 *  Created on: 14.11.2022
 *      Author: matti
 */

int peakCount(float *time, float *data, float dataSize, float treshold, int direction, float peakTime){
    int countPos = 0;
    int peakCountPos = 0;
    int countNeg = 0;
    int peakCountNeg = 0;
    int i;
    for (i = 0; i < dataSize; i++ ){
        if (direction >= 0){
            //This is for positive peaks
            if (data[i] >= treshold) {
                countPos++;
            } else if (countPos){
                //chack if peak is long enough
                if((time[i] - time[i - countPos]) >= peakTime){
                    peakCountPos++;
                }
                countPos = 0;
            }
        }
        if (direction <= 0){
            //this is for negative peaks
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
        printf("invalid peakAxis. Only (x, y, z) are allowed.");
        return -1;
    }
    if (!peakCount(time, axis1, dataSize, errorMargin, 0, 0) && !peakCount(time, axis2, dataSize, errorMargin, 0, 0)){
        return peakCount(time, testAxis, dataSize, 0.25, 1, 0);
    }
    return -1;
}

void clearData(float *data, int size){
    int i;
    for (i = 0; i < size; i++){
        data[i] = 0;
    }
}

