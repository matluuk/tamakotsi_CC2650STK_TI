

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
//#include "funktions.h"

int convertLineToLists(char *line, int index);
void calcSpeed(float *speed, float *array, uint8_t array_size, float *time);

// prototypes for funktions.
int peakCount(float *time, float *data, float dataSize, float treshold, float tresholdOffset, int direction, float peakTime);
int peakCountMargin(float *time, float *ax, float *ay, float *az, int dataSize, char peakAxis, float treshold, float errorMargin, float peakTime);
void movavg(float *array, uint8_t array_size, uint8_t window_size, float *output);
float average(float *data, int dataSize);
void clearData(float *data, int size);

int size = 0;
float time[110];
float ax[110];
float ay[110];
float az[110];
float gx[110];
float gy[110];
float gz[110];

float avgTime[110];
float avgAx[110];
float avgAy[110];
float avgAz[110];
float avgGx[110];
float avgGy[110];
float avgGz[110];

float averageTime;
float averageAx;
float averageAy;
float averageAz;
float averageGx;
float averageGy;
float averageGz;

float velx[110];
float vely[110];
float velz[110];

int main(int argc, char *argv[])
{
    // read any text file from currect directory
    char const *const fileName = "data/liuutus.txt";

    FILE *file = fopen(fileName, "r");

    if (!file)
    {
        printf("\n Unable to open : %s ", fileName);
        return -1;
    }

    char line[500];
    int index;

    while (fgets(line, sizeof(line), file))
    {
        if (convertLineToLists(line, index))
        {
            printf("invalid number of items in list!");
        }
        size++;
        index++;
    }
    fclose(file);

    printf("size: %d\n", size);

    movavg(time, size, 3, avgTime);
    movavg(ax, size, 3, avgAx);
    movavg(ay, size, 3, avgAy);
    movavg(az, size, 3, avgAz);
    movavg(gx, size, 3, avgGx);
    movavg(gy, size, 3, avgGy);
    movavg(gz, size, 3, avgGz);

    averageTime = average(time, size);
    averageAx = average(ax, size);
    averageAy = average(ay, size);
    averageAz = average(az, size);
    averageGx = average(gx, size);
    averageGy = average(gy, size);
    averageGz = average(gz, size);

    printf("averageTime: %.4f\n", averageTime);
    printf("averageAx: %.4f\n", averageAx);
    printf("averageAy: %.4f\n", averageAy);
    printf("averageAz: %.4f\n", averageAz);
    printf("averageGx: %.4f\n", averageGx);
    printf("averageGy: %.4f\n", averageGy);
    printf("averageGz: %.4f\n", averageGz);

    // AX average test
    float sum = 0;
    int i;
    for (i = 0; i < size; i++)
    {
        sum += ax[i];
    }
    float result = (sum / size);
    printf("axAverage : = %.4f\n", result);

    for (int i = 0; i < size; i++)
    {
        printf("%.2f", ay[i]);

        if (i < size - 1)
            printf(",");
    }
    printf("\n");

    printf("algoritmit alkaa!\n");
    float value = 0.25;
    int peaks = peakCount(time, ax, size, 0.25, 0, 1, 0);
    printf("peakCount: %d\n", peaks);

    peaks = peakCountMargin(time, ax, ay, az, size, 'x', 0.25, 0.1, 0);
    if (peaks < 0)
    {
        printf("error margin strikes.");
    }
    else
    {
        printf("peakOneAxis: %d\n", peaks);
    }
    return 0;
}
/// @brief Converts String of mpu 9250 data to lists of ax,ay,az,gx,gy,gz
/// @param line line from file
/// @param index index of data
/// @return return 0. Tried to do different return states.
int convertLineToLists(char *line, int index)
{
    const char sep[] = ","; // Erotin pilkku
    char *token;            // paikkamerkki-osoitin
    int count = 0;
    float t;

    // Erota listasta ensimmäinen luku
    token = strtok(line, sep);
    t = (float)atof(token);

    // erotetaan loput luvut
    if (t >= 0)
    {
        while (token != NULL)
        {
            switch (count)
            {
            case 0:
                time[index] = (float)atof(token);
                break;
            case 1:
                ax[index] = (float)atof(token);
                break;
            case 2:
                ay[index] = (float)atof(token);
                break;
            case 3:
                az[index] = (float)atof(token);
                break;
            case 4:
                gx[index] = (float)atof(token);
                break;
            case 5:
                gy[index] = (float)atof(token);
                break;
            case 6:
                gz[index] = (float)atof(token);
                break;
            default:
                return -1;
                break;
            }
            count++;
            token = strtok(NULL, sep);
        }
    }
    return 0;
}

void calcSpeed(float *speed, float *acc, uint8_t array_size, float *time)
{
    speed[0] = 0;
    for (int i = 1; i < array_size; i++)
    {
        speed[i] = speed[i - 1] + (acc[i] - acc[i - 1]) * (time[i] - time[i - 1]);
    }
}

// These are in functions.h library
// This are not up to date

int peakCount(float *time, float *data, float dataSize, float treshold, float tresholdOffset, int direction, float peakTime)
{
    int countPos = 0;
    int peakCountPos = 0;
    int countNeg = 0;
    int peakCountNeg = 0;
    int i;
    for (i = 0; i < dataSize; i++)
    {
        if (direction >= 0)
        {
            // This is for positive peaks
            if (data[i] - tresholdOffset >= treshold)
            {
                countPos++;
            }
            else if (countPos)
            {
                // chack if peak is long enough
                if ((time[i] - time[i - countPos]) >= peakTime)
                {
                    peakCountPos++;
                }
                countPos = 0;
            }
        }
        if (direction <= 0)
        {
            // This is for negative peaks
            if (data[i] <= -treshold)
            {
                countNeg++;
            }
            else if (countNeg)
            {
                // chack if peak is long enough
                if ((time[i] - time[i - countNeg]) >= peakTime)
                {
                    peakCountNeg++;
                }
                countNeg = 0;
            }
        }
    }
    return peakCountPos + peakCountNeg;
}

int peakCountMargin(float *time, float *ax, float *ay, float *az, int dataSize, char peakAxis, float treshold, float errorMargin, float peakTime)
{
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
        // printf("invalid peakAxis. Only (x, y, z) are allowed.");
        return -2;
    }

    // Calculate averages
    float axis1Avg = average(axis1, dataSize);
    float axis2Avg = average(axis2, dataSize);

    // Check if other axises are in error Margin
    if (!peakCount(time, axis1, dataSize, errorMargin, axis1Avg, 0, 0) && !peakCount(time, axis2, axis2Avg, dataSize, errorMargin, 0, 0))
    {
        // Calkulate peak count for desired axis
        return peakCount(time, testAxis, dataSize, treshold, 0, 1, 0);
    }
    return -1;
}

void movavg(float *array, uint8_t array_size, uint8_t window_size, float *output)
{
    uint8_t new_array_size = array_size - window_size + 1;
    float sum = 0;

    for (int i = 0; i < array_size; i++)
    {

        sum += array[i];
        if (i >= window_size)
        {
            sum -= array[i - window_size];
        }
        output[i] = sum / window_size;
    }
}

float average(float *data, int dataSize)
{
    float sum = 0;
    int i;
    printf("item: \n");
    for (i = 0; i < dataSize; i++)
    {
        sum += *(data + i);
    }
    return (sum / dataSize);
}

void clearData(float *data, int size)
{
    int i;
    for (i = 0; i < size; i++)
    {
        data[i] = 0;
    }
}
