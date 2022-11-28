/* C Standard library */
#include <stdio.h>
#include <inttypes.h>

/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/UART.h>

/* Board Header files */
#include "Board.h"
#include "buzzer.h"
#include "wireless/comm_lib.h"
#include "sensors/opt3001.h"
#include "sensors/mpu9250.h"

/* Own libraries*/
#include "music.h"
#include <functions.h>

/* Prototypes */
void sendBrightnessMsg();
void writeUartMsg(UART_Handle uart);
void sendData();
void playMusic(PIN_Handle buzzerPin, int *note, int tempo);
void clearAllData();

/* Task */
#define STACKSIZE 2048
Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];
Char mainTaskStack[STACKSIZE];

/* State variables */
enum state
{
    MENU = 1,
    DATA_READY,
    MUSIC,
    SEND_DATATODEBUG,
    MOVE_DETECTION,
    MOVE_DETECTION_ALGORITHM,
    MOVE_DETECTION_WAITING,
    GAME,
    GAME_END
};
enum stateUart
{
    WAITING = 1,
    MSG_RECEIVED,
    SEND_MSG,
    SEND_DATA
};
enum stateMenu
{
    MOVE,
    PLAY_GAME,
    PLAY_MUSIC,
    NOT_IN_MENU
};
enum stateBrightness
{
    DARK,
    BRIGHT
};

enum state programState = MENU;
enum stateUart uartState = WAITING;
enum stateMenu menuState = MOVE;
enum stateBrightness brightnessState = DARK;
//
enum state nextState;
enum stateMenu oldMenuState;
// Chosen Music
int *music;

/* Global variables */
//uart and communication
int uartBufferSize = 80;
char uartBuffer[80];
char uartMsg[40];
char msgOne[50];
char msgTwo[50];
char id[] = "id:2064,";
int dataSentUart = 0; //Saves if data is sent to uart

//led game
int getPoint = 0;
int totalPoints = 0;

// Data for move detection
float ambientLight = -1001.0;
float ax, ay, az, gx, gy, gz, time;
int dataIndex;
int dataSize = 85;
float dataAx[85];
float dataAy[85];
float dataAz[85];
float dataGx[85];
float dataGy[85];
float dataGz[85];
float dataTime[85];

// Time variables in (ticks).
Uint32 clockTicks = 0;
Uint32 mpuStartTicks = 0;
Uint32 gameStartTicks = 0;
Uint32 lastTimeTicks = 0;

// RTOS pin handles
static PIN_Handle button0Handle;
static PIN_State button0State;
static PIN_Handle button1Handle;
static PIN_State button1State;
static PIN_Handle led0Handle;
static PIN_Handle led1Handle;
static PIN_State ledState;
static PIN_Handle buzzerHandle;
static PIN_State buzzerState;
// RTOS-variables MPU9250-pins
static PIN_Handle mpuHandle;
// static PIN_State mpuState;

// RTOS:n clock variables
Clock_Handle clkHandle;
Clock_Params clkParams;

/* Pin configs */
PIN_Config button0Config[] = {
    Board_BUTTON0 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE};

PIN_Config button1Config[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE};

PIN_Config led1Config[] = {
    Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE};

PIN_Config led0Config[] = {
    Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE};

PIN_Config buzzerConfig[] = {
    Board_BUZZER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE};

uint_t pinValue_0 = 0; // green LED pinValue
uint_t pinValue_1 = 0; // red LED pinValue

// MPU uses its own I2C interface
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1};

/* this function is called when button0 is pressed
 * MENU:
 * select function
 * MUSIC:
 * stop music and back to menu
 * MOVE:
 * stop move detection and back to menu
 * GAME:
 * when green LED is on, player have to push button0 to get point
 */
void button0Fxn(PIN_Handle handle, PIN_Id pinId)
{

    System_printf("button0 pressed!\n");
    System_flush();

    if (programState == MENU)
    {

        if(menuState == NOT_IN_MENU)
        {
            menuState = oldMenuState;
        }

        switch (menuState)
        {
        //Select function depending of menu state
        case MOVE:
            programState = MUSIC;
            nextState = MOVE_DETECTION;
            music = chooseMusic;
            System_printf("move detection\n");
            System_flush();
            mpuStartTicks = clockTicks;
            sprintf(msgOne,"Tamagotchi is collecting data!");
            uartState = SEND_MSG;
            break;

        case PLAY_GAME:
            PIN_setOutputValue(led0Handle, Board_LED0, 1);
            PIN_setOutputValue(led1Handle, Board_LED1, 0);
            pinValue_0 = 1;
            programState = MUSIC;
            nextState = GAME;
            System_printf("Push button0 when green led is on.\nPush button1 when red led is on.\n");
            System_flush();
            music = chooseMusic;
            gameStartTicks = clockTicks;
            break;

        case PLAY_MUSIC:
            programState = MUSIC;
            music = hedwigsThemeMusic;
            nextState = MENU;
            break;

        default:
            System_printf("ERROR, invalid menuState\n");
            System_flush();
        }
        //Saves menustate
        oldMenuState = menuState;
        menuState = NOT_IN_MENU;
    }
    else if (programState == MUSIC && nextState != GAME) // Led game needs both buttons
    {
        programState = MENU;
    }
    else if (programState == MOVE_DETECTION || programState == MOVE_DETECTION_ALGORITHM || programState == MOVE_DETECTION_WAITING)
    {
        //Back to menu
        clearAllData();
        programState = MUSIC;
        nextState = MENU;
        music = backMusic;
        System_printf("MOVE_DETECTION stopped!\n");
        System_flush();
    }
    else if (programState == GAME)
    {
        if ((pinValue_0 == 1) && (getPoint == 0))
        {
            char msg[30];
            programState = MUSIC;
            nextState = GAME;
            music = gamePointMusic;
            sprintf(msg, "You get a point! pinValue_0 : %d\n", pinValue_0);
            System_printf(msg);
            getPoint += 1;
            totalPoints += 1;
        }
        else
        {
            programState = GAME_END;
            System_printf("Wrong button!\n");
            System_flush();
            gameStartTicks = clockTicks;
        }
    }
}

/* this funktion is called when button 1 is pressed
 * MENU:
 * roll menu with this button
 * GAME:
 * when red LED is on, player have to push button1 to get point
 *
 */

void button1Fxn(PIN_Handle handle, PIN_Id pinId)
{
    System_printf("button1 pressed!\n");
    System_flush();

    if (programState == MENU)
    {
        switch (menuState)
        {
        case MOVE:
            menuState = PLAY_GAME;
            break;

        case PLAY_GAME:
            menuState = PLAY_MUSIC;
            break;

        case PLAY_MUSIC:
            menuState = MOVE;
            break;

        default:
            System_printf("ERROR, invalid menuState\n");
            System_flush();
        }
    }
    else if (programState == GAME)
    {
        if (pinValue_1 == 1)
        {
            if (getPoint == 0)
            {
                char msg[30];
                programState = MUSIC;
                nextState = GAME;
                music = gamePointMusic;
                sprintf(msg, "You get a point! pinValue_0 : %d\n", pinValue_1);
                System_printf(msg);
                System_flush();
                getPoint = getPoint + 1;
                totalPoints = totalPoints + 1;
            }
        }
        else
        {
            programState = GAME_END;
            System_printf("Wrong button!\n");
            System_flush();
            gameStartTicks = clockTicks;
        }
    }
}

static void uartFxn(UART_Handle uart, void *rxBuf, size_t len)
{
    uartState = MSG_RECEIVED;

    //Starts waiting next message
    UART_read(uart, rxBuf, uartBufferSize);
}

Void clkFxn(UArg arg0)
{
    clockTicks = Clock_getTicks(); // saves time from start
}

/* Task Functions */
static void uartTaskFxn(UArg arg0, UArg arg1)
{

    char msg[85];
    const char sep[] = ":";

    // UART-library settings
    UART_Handle uart;
    UART_Params uartParams;

    // intialise serial monitoring
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_TEXT;
    uartParams.readDataMode = UART_DATA_TEXT;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.readMode = UART_MODE_CALLBACK;
    uartParams.readCallback = &uartFxn;
    uartParams.baudRate = 9600;            // speed 9600baud
    uartParams.dataLength = UART_LEN_8;    // 8
    uartParams.parityType = UART_PAR_NONE; // n
    uartParams.stopBits = UART_STOP_ONE;   // 1

    uart = UART_open(Board_UART0, &uartParams);
    if (uart == NULL)
    {
        System_abort("Error opening the UART");
    }

    UART_read(uart, &uartBuffer, uartBufferSize);

    while (1)
    {
        switch (uartState)
        {
        case MSG_RECEIVED:
            uartState = WAITING;
            // sprintf(msg, "Uartmsg: %s\n", uartBuffer);

            char *token;
            token = strtok(uartBuffer, sep);

            if (strcmp(token, "2064,BEEP") == 0)
            {
                // System_printf("beepTest\n");
                // System_flush();
                if (programState != MUSIC)
                {
                    nextState = programState;
                    programState = MUSIC;
                }
                music = lowFeaturesMusic;
            }
            break;

        case SEND_MSG:
            writeUartMsg(uart);
            System_printf("uartTask,SEND_MSG: msg sent\n");
            System_flush();
            uartState = WAITING;
            break;

        case SEND_DATA:
            writeUartMsg(uart);
            System_printf("uartTask,SEND_DATA: msg sent\n");
            System_flush();
            // sends data currently not in use
            sprintf(msg, "%ssession:start", id);
            UART_write(uart, msg, strlen(msg) + 1);


            int i;
            for (i = 0; i < dataIndex; i++)
            {
                sprintf(msg, "%stime:%.3f,ax:%.3f,ay:%.3f,az:%.3f,gx:%.3f,gy:%.3f,gz:%.3f",
                        id,
                        (dataTime[i] - dataTime[0]),
                        dataAx[i],
                        dataAy[i],
                        dataAz[i],
                        dataGx[i],
                        dataGy[i],
                        dataGz[i]);
                UART_write(uart, msg, strlen(msg) + 1);
            }
            sprintf(msg, "%ssession:end", id);
            UART_write(uart, msg, strlen(msg) + 1);

            dataSentUart = 1;
            uartState = WAITING;
            System_printf("Data sent to uart!\n");
            System_flush();

            break;

        }

        Task_sleep(50000 / Clock_tickPeriod);
    }
}

Void sensorTaskFxn(UArg arg0, UArg arg1)
{

    I2C_Handle i2c;
    I2C_Params i2cParams;
    I2C_Handle i2cMPU; // Own i2c-interface for MPU9250 sensor
    I2C_Params i2cMPUParams;
    // char msg[30];
    lastTimeTicks = 0;

    // Configure i2cMPU
    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    // Note the different configuration below
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    // MPU power on
    PIN_setOutputValue(mpuHandle, Board_MPU_POWER, Board_MPU_POWER_ON);

    // Wait 100ms for the MPU sensor to power up
    Task_sleep(100000 / Clock_tickPeriod);
    System_printf("MPU9250: Power ON\n");
    System_flush();

    // MPU open i2c
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL)
    {
        System_abort("Error Initializing I2CMPU\n");
    }

    // MPU setup and calibration
    System_printf("MPU9250: Setup and calibration...\n");
    System_flush();

    mpu9250_setup(&i2cMPU);

    System_printf("MPU9250: Setup and calibration OK\n");
    System_flush();

    // MPU close i2c
    I2C_close(i2cMPU);
    // MPU power off
    // PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);

    // Open I2C
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    // open i2c connection
    i2c = I2C_open(Board_I2C_TMP, &i2cParams);
    if (i2c == NULL)
    {
        System_abort("Error Initializing I2C\n");
    }

    Task_sleep(1000 / Clock_tickPeriod);
    opt3001_setup(&i2c);
    I2C_close(i2c);

    while (1)
    {

        if ((clockTicks - lastTimeTicks) * Clock_tickPeriod < 4000000)
        {
            // open i2c
            i2c = I2C_open(Board_I2C_TMP, &i2cParams);
            if (i2c == NULL)
            {
                System_abort("Error Initializing I2C\n");
            }

            //Get opt3001 data
            ambientLight = opt3001_get_data(&i2c);

            //Check value
            if (ambientLight <= 65 && ambientLight > 0)
            {
                brightnessState = DARK;
            }
            else if (ambientLight > 65)
            {
                brightnessState = BRIGHT;

            }

            //saves current ticks
            lastTimeTicks = clockTicks;

            // close i2c
            I2C_close(i2c);
        }

        if (programState == MOVE_DETECTION)
        {
            if ((clockTicks - mpuStartTicks) * Clock_tickPeriod < 4000000)
            {
                // MPU open i2c
                i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
                if (i2cMPU == NULL)
                {
                    System_abort("Error Initializing I2CMPU\n");
                }
                // MPU ask data
                mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
                // time
                time = Clock_getTicks() * Clock_tickPeriod / 1000;
                // MPU close i2c
                I2C_close(i2cMPU);

                // saves data to lists
                dataTime[dataIndex] = time;
                dataAx[dataIndex] = ax;
                dataAy[dataIndex] = ay;
                dataAz[dataIndex] = az;
                dataGx[dataIndex] = gx;
                dataGy[dataIndex] = gy;
                dataGz[dataIndex] = gz;
                dataIndex++;
            }
            else
            {
                //Ends data gathering
                //Plays music
                programState = MUSIC;
                nextState = MOVE_DETECTION_ALGORITHM;
                music = dataReadyMusic;
                //Message to uart
                sprintf(msgOne,"Tamagotchi is analyzing data!");
                uartState = SEND_MSG;

                System_printf("Data ready for movement algorithm.\n");
                System_flush();
            }
        }

        Task_sleep(50000 / Clock_tickPeriod);
    }
}

Void mainTaskFxn(UArg arg0, UArg arg1)
{
    //Constants for movement algorithm
    const int moveAvgWindowSize = 3;
    const float peakTreshold = 0.15;
    const float peakTime = 120;
    const float errorMargin = 0.12;
    const float errorTime = 100;

    // Variables

    //Points
    int eatPoints;
    int petPoints;
    int exercicePoints;

    char msg[30];

    //Led game
    int blinkAccelator = 1;
    int endBlinks = 0;

    //Saved states consider moving to global
    int brightnessValue = -1;
    int menuValue = -1;

    //variables for movement detection algorithm
    //Averages
    float avgAx = 0;
    float avgAy = 0;
    float avgAz = 0;
    //Acceleration peak counts
    int peaks = 0;
    int peaksX = 0;
    int peaksY = 0;
    int peaksZ = 0;
    char direction = '\0';


    /*On Startup*/
    // red led is on at start
    System_printf("b0: Move your tamagotchi\nb1: next state\n---\n");

    while (1)
    {
        switch (programState)
        {
        case MUSIC:
            // System_printf("Playing music.\n");
            // System_flush();

            //Play mysic
            playMusic(buzzerHandle, music, 144);

            //Check if hedwigs theme
            if (music == hedwigsThemeMusic)
            {
               //Set points
               eatPoints = 2;
               petPoints = 2;
               exercicePoints = 4;

               if (brightnessState == BRIGHT)
               {
                   eatPoints = 3;
                   petPoints = 3;
                   exercicePoints = 5;
               }

                System_printf("music was Hedwigs Theme.\n");
                System_flush();
                sprintf(uartMsg, "ACTIVATE:%d;%d;%d",eatPoints, petPoints, exercicePoints);
                uartState = SEND_MSG;
            }

            //Change program State to next State
            programState = nextState;
            break;

        case GAME:
            if ((clockTicks - gameStartTicks) * Clock_tickPeriod / 1000 > 2000 - blinkAccelator) //Time from 2000ms and decreases from there
            {
                // System_printf("led changes/n");
                // System_flush();
                pinValue_0 = PIN_getOutputValue(Board_LED0);
                pinValue_0 = !pinValue_0;
                PIN_setOutputValue(led0Handle, Board_LED0, pinValue_0);
                pinValue_1 = PIN_getOutputValue(Board_LED1);
                pinValue_1 = !pinValue_1;
                PIN_setOutputValue(led1Handle, Board_LED1, pinValue_1);

                if (blinkAccelator < 1800)
                {
                    blinkAccelator = blinkAccelator + 130; //Time decreases in 130ms intervals
                }
                //Set point variable to 0. For not to be able to get multiple points in one led blink
                getPoint = 0;
                //Saves current ticks
                gameStartTicks = clockTicks;
                //Print points to uart
                sprintf(msgTwo, "Points: %d", totalPoints);
                uartState = SEND_MSG;
            }
            break;

        case GAME_END:
            // Both LEDs blinks 5 times and go back to menu
            if ((clockTicks - gameStartTicks) * Clock_tickPeriod / 1000 > 50)
            {
                if (endBlinks == 0)
                {
                    pinValue_0 = 1;
                    pinValue_1 = 1;
                    endBlinks = endBlinks + 1;
                }
                else if (endBlinks > 0 && endBlinks < 10)
                {
                    pinValue_1 = !pinValue_1;
                    pinValue_0 = !pinValue_0;
                    endBlinks = endBlinks + 1;
                }
                PIN_setOutputValue(led0Handle, Board_LED0, pinValue_1);
                PIN_setOutputValue(led1Handle, Board_LED1, pinValue_1);
                gameStartTicks = clockTicks;
                if (endBlinks >= 10) {
                    eatPoints = 1;
                    petPoints = 0;
                    exercicePoints = 0;

                    if (totalPoints >= 3)
                    {
                        if (brightnessState == BRIGHT)
                        {
                            eatPoints = 2;
                        }
                        if (totalPoints < 6)
                        {
                            petPoints = 1;
                            exercicePoints = 1;
                        }
                        else if (totalPoints >= 6 && totalPoints < 12)
                        {
                            eatPoints = eatPoints + 2;
                            petPoints = 2;
                            exercicePoints = 2;
                        }
                        else if (totalPoints >= 12)
                        {
                            eatPoints = eatPoints + 3;
                            petPoints = 3;
                            exercicePoints = 3;
                        }
                        sprintf(uartMsg, "ACTIVATE:%d;%d;%d",eatPoints, petPoints, exercicePoints);
                        uartState = SEND_MSG;
                    }
                    PIN_setOutputValue( led0Handle, Board_LED0, 1 );
                    PIN_setOutputValue( led1Handle, Board_LED1, 1 );
                    sprintf(msgTwo,"");
                    blinkAccelator = 1;
                    endBlinks = 0;
                    getPoint = 0;
                    totalPoints = 0;
                    programState = MUSIC;
                    nextState = MENU;
                    music = gameEndMusic;
                }
            }
            break;

        case MOVE_DETECTION_ALGORITHM:

            programState = MUSIC;
            nextState = MOVE_DETECTION_WAITING;
            music = menuMusic;

            dataSentUart = 0; //Saves if data is sent to uart

            //Set reward points to 0
            eatPoints = 0;
            petPoints = 0;
            exercicePoints = 0;
            direction = '\0';

            //Calculate averages from every axis
            avgAx = average(dataAx, dataIndex);
            avgAy = average(dataAy, dataIndex);
            avgAz = average(dataAz, dataIndex);

            //Calculate moving averages
            movavg(dataAx, dataSize, moveAvgWindowSize, avgAx);
            movavg(dataAy, dataSize, moveAvgWindowSize, avgAy);
            movavg(dataAz, dataSize, moveAvgWindowSize, avgAz);


            //Print averages to debug console
            sprintf(msg, "avgAx: %.2f\n", avgAx);
            System_printf(msg);
            sprintf(msg, "avgAy: %.2f\n", avgAy);
            System_printf(msg);
            sprintf(msg, "avgAz: %.2f\n", avgAz);
            System_printf(msg);
            System_flush();


            //Testing code for peaks without error margins
            /*
            // display peaks + side
            System_printf("peakCounts without error margin + side:\n");
            peaksX = peakCount(dataTime, dataAx, dataIndex, peakTreshold, avgAx, 1, peakTime);
            sprintf(msg, "Ax: = %d\n", peaks);
            System_printf(msg);
            peaksY = peakCount(dataTime, dataAy, dataIndex, peakTreshold, avgAy, 1, peakTime);
            sprintf(msg, "Ay: = %d\n", peaks);
            System_printf(msg);
            peaksZ = peakCount(dataTime, dataAz, dataIndex, peakTreshold, avgAz, 1, peakTime);
            sprintf(msg, "Az: = %d\n", peaks);
            System_printf(msg);
            System_flush();
            */

            // Calculate peak counts
            peaksX = peakCountMargin(dataTime, dataAx, dataAy, dataAz, dataIndex, peakTreshold, peakTime, errorMargin, errorTime);
            peaksY = peakCountMargin(dataTime, dataAy, dataAx, dataAz, dataIndex, peakTreshold, peakTime, errorMargin, errorTime);
            peaksZ = peakCountMargin(dataTime, dataAz, dataAx, dataAy, dataIndex, peakTreshold, peakTime, errorMargin, errorTime);



            // Print peak counts to debug
            System_printf("peakCounts with error margin + side:\n");
            sprintf(msg, "Ax: = %d\n", peaksX);
            System_printf(msg);
            sprintf(msg, "Ay: = %d\n", peaksY);
            System_printf(msg);
            sprintf(msg, "Az: = %d\n", peaksZ);
            System_printf(msg);
            System_flush();


            //Check movement direction
            if (peaksX > 0)
            {
                //Sets direction to x
                peaks = peaksX;
                direction = 'x';
            }
            else if (peaksY > 0)
            {
                //Sets direction to y
                peaks = peaksY;
                direction = 'y';
            }
            else if (peaksZ > 0)
            {
                //Sets direction to z
                peaks = peaksZ;
                direction = 'z';
            }
            else if(peaksX == 0 && peaksY == 0 && peaksZ == 0)
            {
                //no movement
                peaks = 0;
            }
            else
            {
                //Error margin strikes. Movement in multiple direction
                peaks = -1;
            }

            //Check chosen peak count
            if (peaks > 0)//Movenemt in chosen direction
            {
                //Set points depending of count
                if (peaks >= 3){
                    eatPoints = 3;
                    petPoints = 3;
                    exercicePoints = 5;
                }else if (peaks >= 1){
                    eatPoints = 2;
                    petPoints = 2;
                    exercicePoints = 4;
                }
                sprintf(msgTwo, "Tamagotchi %d times in %c direction", peaks, direction);
            }
            else if (peaks == 0)//No movement
            {
                sprintf(msgTwo, "Tamagotchi didn't move!");
            }
            else//Movement in multiple direction
            {
                sprintf(msgTwo, "Tamagotchi moved in multiple directions!");
            }

            sprintf(uartMsg, "ACTIVATE:%d;%d;%d",eatPoints, petPoints, exercicePoints);
            uartState = SEND_DATA;
            break;

        case MOVE_DETECTION_WAITING:
            //Waits for the uartTaskFxn to send data. There would be further problems if dont wait. Can be cancelled with button
            if (dataSentUart){
                programState = MENU;
                dataSentUart = 0;
                System_printf("mainTask,MOVE_DETECTION_WAITING: back to menu!\n");
                System_flush();
                clearAllData();
            }
            break;

        case SEND_DATATODEBUG:
            //Send data to debug. Currently not in use
            programState = MUSIC;
            nextState = MENU;
            music = backMusic;
            sendData();
            System_printf("data sent to debug.\n");
            break;

        case MENU:
                //Change brightness message if brightnessState is changed
                if (brightnessValue != brightnessState)
                {
                    sendBrightnessMsg();
                }
                brightnessValue = brightnessState;

                //Check if menuState have changed
                if (menuValue != menuState)
                {
                    //update brightness msg
                    sendBrightnessMsg();

                    System_printf("menuState changed!\n");
                    System_flush();

                    if (menuState == NOT_IN_MENU)
                    {
                        //Change menuState back to previous state. This is to force menuState change when going back to menu. Better way would be to save button press
                        menuState = oldMenuState;
                    }
                    switch(menuState)
                    {
                    case PLAY_GAME:
                        //Print to debug
                        System_printf("b0: Play LED-game\nb1: next state\n---\n");
                        System_flush();
                        //Update menuState to UART
                        sprintf(msgOne,"MENU: LED-game");
                        uartState = SEND_MSG;
                        //update leds
                        PIN_setOutputValue(led0Handle, Board_LED0, 1);
                        PIN_setOutputValue(led1Handle, Board_LED1, 1);
                        //Play music for menuState
                        programState = MUSIC;
                        nextState = MENU;
                        music = gameMusic;
                        break;

                    case PLAY_MUSIC:
                        //Print to debug
                        System_printf("b0: Listen music\nb1: next state\n---\n");
                        System_flush();
                        //Update menuState to UART
                        sprintf(msgOne,"MENU: Play music");
                        uartState = SEND_MSG;
                        //update leds
                        PIN_setOutputValue(led0Handle, Board_LED0, 1);
                        PIN_setOutputValue(led1Handle, Board_LED1, 0);
                        //Play music for menuState
                        programState = MUSIC;
                        nextState = MENU;
                        music = listenMusic;
                        break;

                    case MOVE:
                        //Print to debug
                        System_printf("b0: start move detection\nb1: next state\n---\n");
                        System_flush();
                        //Update menuState to UART
                        sprintf(msgOne,"MENU: Move detection");
                        uartState = SEND_MSG;
                        //update leds
                        PIN_setOutputValue(led0Handle, Board_LED0, 0);
                        PIN_setOutputValue(led1Handle, Board_LED1, 1);
                        //Play music for menuState
                        programState = MUSIC;
                        nextState = MENU;
                        music = moveMusic;
                        break;

                    case NOT_IN_MENU:
                        break;

                    default:

                        System_printf("MENU: Invalid menu state!\n");
                        System_flush();
                        sprintf(msgOne,"MENU: Invalid menu state!");
                        uartState = SEND_MSG;
                        break;
                    }
                }
                menuValue = menuState;//save menustate
            break;

        }
        System_flush();
        Task_sleep(50000 / Clock_tickPeriod);
    }
}

// Own functions
/*
 * Checks brightnessState
 * Sets message to uart msgTwo buffer according to brightnessState
*/
void sendBrightnessMsg()
{
    //Check brightnessState and send uarMsg
    if(brightnessState == DARK)
    {
        sprintf(msgTwo,"It is too dark in there");
        uartState = SEND_MSG;
    }
    else
    {
        sprintf(msgTwo,"Nice! It's so bright in there");
        uartState = SEND_MSG;
    }
}
/*
 * Sends UART message using UART handle.
 * Always sends msgOne and msgTwo buffer messages.
 * Sends uartMsg if not \0
 */
void writeUartMsg(UART_Handle uart)
{
    char msg[100];
    sprintf(msg, "%sMSG1:%s,MSG2:%s\0", id, msgOne, msgTwo);
    UART_write(uart, msg, strlen(msg) + 1);
    if (uartMsg[0] != '\0') {
        sprintf(msg, "%s%s\0", id,uartMsg);
        sprintf(uartMsg,"");
        UART_write(uart, msg, strlen(msg) + 1);
    }
}

/*
 * Go through all data and sends to debug.
 */
void sendData()
{
    // sends data
    char msgg[30];
    int i;
    sprintf(msgg, "[time, ax, ay, az, gx, gy, gz]\n");
    System_printf(msgg);
    for (i = 0; i < dataIndex; i++)
    {
        sprintf(msgg, "%05f,%05f,%05f,%05f,%05f,%05f,%05f\n",
                dataTime[i] - dataTime[0],
                dataAx[i],
                dataAy[i],
                dataAz[i],
                dataGx[i],
                dataGy[i],
                dataGz[i]);
        System_printf(msgg);
        System_flush();
    }

    clearAllData();
}
/*
 * Function: playMusic
 * ---------------------------
 * plays music with sensor tag buzzer. Music gets interrupted if programState changes.
 *
 * buzzerPin: Pin handle for used buzzer.
 *
 * *music: list of frequencies (Hz) and note durations (1 whole note, 4 is quarter note). Durations with minus(-) are dotted notes. Ends when frequency is -1. For example: 144(Hz), 2(half note), 200(Hz), -1(dotted whole note), -1
 *
 * tempo: Tells how many quarter notes are in minute.
 *
 * code modified from: https://github.com/robsoncouto/arduino-songs
 *
 */
void playMusic(PIN_Handle buzzerPin, int *music, int tempo)
{

    // this calculates the duration of a whole note in ticks (60s/tempo)*4 beats
    int wholenote = (60000000 * 4) / tempo;

    int divider = 0, noteDuration = 0;

    // iterate over the notes of the melody.
    int *note;
    for (note = music;*note != -1; note = note + 2)
    {
        if (programState == MUSIC)
        {
            // calculates the duration of each note
            divider = *(note + 1);
            if (divider > 0)
            {
                // regular note, just proceed
                noteDuration = (wholenote) / divider;
            }
            else if (divider < 0)
            {
                // dotted notes are represented with negative durations!!
                noteDuration = (wholenote) / abs(divider);
                noteDuration *= 1.5; // increases the duration in half for dotted notes
            }

            // we only play the note for 90% of the duration, leaving 10% as a pause
            buzzerOpen(buzzerPin);
            buzzerSetFrequency(*note);
            Task_sleep(noteDuration * 0.9 / Clock_tickPeriod); //Is not good idea to use in the task this way

            // stop buzzer before the next note.
            buzzerClose();

            // Wait for the specified duration before playing the next note.
            Task_sleep(noteDuration * 0.1 / Clock_tickPeriod); //Is not good idea to use in the task this way
        }
        else
        {
            nextState = MENU;
            System_printf("Music stop!\n");
            System_flush();
            break;
        }
    }
}

/*
 * Clears all data from data variables
 */
void clearAllData()
{
    clearData(dataAx, dataSize);
    clearData(dataAy, dataSize);
    clearData(dataAz, dataSize);
    clearData(dataGx, dataSize);
    clearData(dataGy, dataSize);
    clearData(dataGz, dataSize);
    clearData(dataTime, dataSize);
    dataIndex = 0;
    System_printf("All data cleared!\n");
    System_flush();
}

Int main(void)
{

    // Task variables
    Task_Handle sensorTaskHandle;
    Task_Params sensorTaskParams;
    Task_Handle uartTaskHandle;
    Task_Params uartTaskParams;
    Task_Handle mainTaskHandle;
    Task_Params mainTaskParams;

    // Initialize board
    Board_initGeneral();
    Init6LoWPAN();

    // Initialize i2c bus
    Board_initI2C();
    // Initialize UART
    Board_initUART();

    // buttons and LEDs
    button0Handle = PIN_open(&button0State, button0Config);
    if (!button0Handle)
    {
        System_abort("Error initializing button0 pins\n");
    }
    button1Handle = PIN_open(&button1State, button1Config);
    if (!button1Handle)
    {
        System_abort("Error initializing button1 pins\n");
    }
    led0Handle = PIN_open(&ledState, led0Config);
    if (!led0Handle)
    {
        System_abort("Error initializing LED pins\n");
    }
    led1Handle = PIN_open(&ledState, led1Config);
    if (!led1Handle)
    {
        System_abort("Error initializing LED pins\n");
    }
    if (PIN_registerIntCb(button0Handle, &button0Fxn) != 0)
    {
        System_abort("Error registering button0 callback function");
    }
    if (PIN_registerIntCb(button1Handle, &button1Fxn) != 0)
    {
        System_abort("Error registering button1 callback function");
    }

    // Initialize clock
    Clock_Params_init(&clkParams);
    clkParams.period = 50000 / Clock_tickPeriod; // 50ms
    clkParams.startFlag = TRUE;

    clkHandle = Clock_create((Clock_FuncPtr)clkFxn, 50000 / Clock_tickPeriod, &clkParams, NULL);
    if (clkHandle == NULL)
    {
        System_abort("Clock create failed");
    }

    // Buzzer
    buzzerHandle = PIN_open(&buzzerState, buzzerConfig);
    if (buzzerHandle == NULL)
    {
        System_abort("Pin open failed!");
    }

    /* Task */
    Task_Params_init(&sensorTaskParams);
    sensorTaskParams.stackSize = STACKSIZE;
    sensorTaskParams.stack = &sensorTaskStack;
    sensorTaskParams.priority = 2;
    sensorTaskHandle = Task_create(sensorTaskFxn, &sensorTaskParams, NULL);
    if (sensorTaskHandle == NULL)
    {
        System_abort("sensorTask create failed!");
    }

    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority = 2;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL)
    {
        System_abort("uartTask create failed!");
    }

    Task_Params_init(&mainTaskParams);
    mainTaskParams.stackSize = STACKSIZE;
    mainTaskParams.stack = &mainTaskStack;
    mainTaskParams.priority = 2;
    mainTaskHandle = Task_create(mainTaskFxn, &mainTaskParams, NULL);
    if (mainTaskHandle == NULL)
    {
        System_abort("mainTask create failed!");
    }

    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
