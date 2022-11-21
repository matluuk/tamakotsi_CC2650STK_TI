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
#include "funktions.h"

/* Prototypes */
void sendData();
void playMusic(PIN_Handle buzzerPin, int *note, int tempo);
void clearAllData();

/* Task */
#define STACKSIZE 2048
Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];
Char dataTaskStack[STACKSIZE];
Char mainTaskStack[STACKSIZE];

//Tilamuuttujat
enum state { MENU=1, DATA_READY, MUSIC, SEND_DATA, MOVE_DETECTION, MOVE_DETECTION_DATA_READY, MOVE_DETECTION_ALGORITHM, GAME};
enum stateUart {WAITING=1, MSG_RECEIVED, SEND_MSG};
enum stateMenu {MOVE, PLAY_GAME, PLAY_MUSIC};
enum state programState = MENU;
enum stateUart uartState = WAITING;
enum stateMenu menuState =  MOVE;
//seuraava tilamuutos
enum state nextState;
//chosen music
int *music;
//Menu


//Globaalit muutujat
float ambientLight = -1000.0;
float ax, ay, az, gx, gy, gz, time;
int uartBufferSize = 80;
char uartBuffer[80];
char uartMsg[80];

//Data for move detection
int dataIndex;
int dataSize = 85;
float lightData[85];
float axData[85];
float ayData[85];
float azData[85];
float gxData[85];
float gyData[85];
float gzData[85];
float timeData[85];

//time variables in (ticks).
Uint32 clockTicks = 0;
Uint32 mpuStartTicks = 0;
Uint32 gameStartTicks = 0;


// RTOS pin handles
static PIN_Handle button0Handle;
static PIN_State button0State;
static PIN_Handle button1Handle;
static PIN_State button1State;
static PIN_Handle ledHandle;
static PIN_State ledState;
static PIN_Handle buzzerHandle;
static PIN_State buzzerState;
// RTOS-muuttujat MPU9250-pinneille
static PIN_Handle mpuHandle;
//static PIN_State mpuState;

// RTOS:n kellomuuttujat
Clock_Handle clkmasaHandle;
Clock_Params clkmasaParams;
Clock_Handle clkHandle;
Clock_Params clkParams;



PIN_Config button0Config[] = {
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE
};

PIN_Config button1Config[] = {
   Board_BUTTON1  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE
};

PIN_Config led1Config[] = {
   Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE
};

PIN_Config led0Config[] = {
   Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE
};

PIN_Config buzzerConfig[] = {
  Board_BUZZER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
  PIN_TERMINATE
};

uint_t pinValue_0 = 0;
uint_t pinValue_1 = 0;

// MPU9250-pin settings
/*
static PIN_Config mpuConfig[] = {
    Board_MPU_INT | PIN_INPUT_EN | PIN_PULLDOWN | PIN_IRQ_DIS | PIN_HYSTERESIS,
    PIN_TERMINATE
};
*/

// MPU uses its own I2C interface
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};



/*This funktion is called when button 0 is pressed.
 * MENU:
 * select funktion.
 */
void button0Fxn(PIN_Handle handle, PIN_Id pinId) {

    System_printf("button0 pressed!\n");
    System_flush();

    if(programState == MENU) {
        switch(menuState){
            case MOVE:
                programState = MUSIC;
                nextState = MOVE_DETECTION;
                music = chooseMusic;
                mpuStartTicks = clockTicks;
                sprintf(uartMsg, "session:start");
                uartState = SEND_MSG;
                break;

            case PLAY_GAME:
                //led-game;
                PIN_setOutputValue( ledHandle, Board_LED0, 1 );
                PIN_setOutputValue( ledHandle, Board_LED1, 0 );
                programState = MUSIC;
                nextState = GAME;
                System_printf("Push button0 when red led is on.\nPush button1 when green led is on.\n");
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
        }
    } else if (programState == MUSIC) {
        programState = MENU;
        System_printf("programState: MENU\n");
    } else if (programState == MOVE_DETECTION || programState == MOVE_DETECTION_DATA_READY || programState == MOVE_DETECTION_ALGORITHM){
        programState = MUSIC;
        nextState = MENU;
        //Clock_stop(clkmasaHandle);
        music = backMusic;
        System_printf("MOVE_DETECTION stopped!\n");
    } else if (programState == GAME) {
        if (pinValue_0 == 1) {
            char msg[30];
            programState = MUSIC;
            nextState = GAME;
            music = gamePointMusic;
            sprintf(msg, "You get a point! pinValue_0 : %d\n", pinValue_0);
            System_printf(msg);
        }
        else {
            programState = MENU;
            music = gameEndMusic;
            System_printf("Wrong button!\n");
        }
    }
    System_flush();
}

/*This funktion is called when button 1 is pressed.
 * MENU:
 * roll menu with this button.
 *
 */

void button1Fxn(PIN_Handle handle, PIN_Id pinId) {
    System_printf("button1 pressed!\n");
    System_flush();

    if(programState == MENU) {
        switch(menuState){
            case MOVE:
                System_printf("b0: start led-game\nb1: next state\n---\n");
                sprintf(uartMsg, "MSG1:MENU: LED-game");
                uartState = SEND_MSG;
                //molemmat ledit p��lle
                PIN_setOutputValue( ledHandle, Board_LED0, 1 );
                PIN_setOutputValue( ledHandle, Board_LED1, 1 );
                programState = MUSIC;
                nextState = MENU;
                music = gameMusic;
                menuState = PLAY_GAME;
                break;

            case PLAY_GAME:
                System_printf("b0: listen music\nb1: next state\n---\n");
                sprintf(uartMsg, "MSG1:MENU: Listen music");
                uartState = SEND_MSG;
                //vain punainen led p��lle
                PIN_setOutputValue( ledHandle, Board_LED0, 1 );
                PIN_setOutputValue( ledHandle, Board_LED1, 0 );
                programState = MUSIC;
                nextState = MENU;
                music = listenMusic;
                menuState = PLAY_MUSIC;
                break;

            case PLAY_MUSIC:
                System_printf("b0: Move your tamagotchi\nb1: next state\n---\n");
                sprintf(uartMsg, "MSG1:MENU: Move your tamagotchi");
                uartState = SEND_MSG;
                //vain vihre� led p��lle
                PIN_setOutputValue( ledHandle, Board_LED0, 0 );
                PIN_setOutputValue( ledHandle, Board_LED1, 1 );
                programState = MUSIC;
                nextState = MENU;
                music = moveMusic;
                menuState = MOVE;
                break;
            default:
                System_printf("ERROR, invalid menuState\n");
        }
    } else if (programState == GAME) {
        if (pinValue_1 == 1) {
            char msg[30];
            programState = MUSIC;
            nextState = GAME;
            music = gamePointMusic;
            sprintf(msg, "You get a point! pinValue_0 : %d\n", pinValue_1);
            System_printf(msg);
        }
        else {
            programState = MENU;
            music = gameEndMusic;
            System_printf("Wrong button!\n");
        }
    }
    System_flush();

}


/*
Void mpuFxn(PIN_Handle mpuHandle, PIN_Id pinId) {
      System_printf("MpuFxn");
      System_flush();
}
*/

static void uartFxn(UART_Handle uart, void *rxBuf, size_t len) {

    System_printf("uartFxn!\n");
    System_flush();
    uartState = MSG_RECEIVED;

    // K�sittelij�n viimeisen� asiana siirryt��n odottamaan uutta keskeytyst�..
    UART_read(uart, rxBuf, uartBufferSize);
}

Void clkFxn(UArg arg0) {
    clockTicks = Clock_getTicks(); //tallentaa ajan k�ynnistyksest�.
    if (programState == GAME) {
        //printf("clock test working\n");
        //System_flush();
        }
}

/* Task Functions */
static void uartTaskFxn(UArg arg0, UArg arg1) {

    char msg[85];

    // UART-library settings
    UART_Handle uart;
    UART_Params uartParams;

    // intialise serial monitoring
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_TEXT;
    uartParams.readDataMode = UART_DATA_TEXT;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.readMode = UART_MODE_CALLBACK; // Keskeytyspohjainen vastaanotto
    uartParams.readCallback  = &uartFxn; // K�sittelij�funktio
    uartParams.baudRate = 9600; // nopeus 9600baud
    uartParams.dataLength = UART_LEN_8; // 8
    uartParams.parityType = UART_PAR_NONE; // n
    uartParams.stopBits = UART_STOP_ONE; // 1

    // Avataan yhteys laitteen sarjaporttiin vakiossa Board_UART0
    uart = UART_open(Board_UART0, &uartParams);
    if (uart == NULL) {
        System_abort("Error opening the UART");
    }

    //k�ynnist�� datan vastaanottamisen
    UART_read(uart, &uartBuffer, uartBufferSize);

    while (1) {
        if(uartState == MSG_RECEIVED){
            uartState = WAITING;
            sprintf(msg, "Uartmsg: %s\n", uartBuffer);
            System_printf(msg);
            System_flush();

            //send message back, for testing
            sprintf(uartMsg, "%s", uartBuffer);
            uartState = SEND_MSG;
        } else if (uartState == SEND_MSG){
            uartState = WAITING;
            sprintf(msg, "id:2064,%s\0", uartMsg);
            UART_write(uart, msg, strlen(msg) + 1);
            System_printf("uart message:\n");
            System_printf(msg);
            System_printf("\n");
            System_flush();
        }

        // Just for sanity check for exercise, you can comment this out
        //System_printf("uartTask\n");
        //System_flush();

        Task_sleep(100000 / Clock_tickPeriod);
    }
}

Void sensorTaskFxn(UArg arg0, UArg arg1) {

    I2C_Handle      i2c;
    I2C_Params      i2cParams;
    I2C_Handle i2cMPU; // Own i2c-interface for MPU9250 sensor
    I2C_Params i2cMPUParams;
    char msg[30];

    //Configure i2cMPU
    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    // Note the different configuration below
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    // MPU power on
    PIN_setOutputValue(mpuHandle,Board_MPU_POWER, Board_MPU_POWER_ON);

    // Wait 100ms for the MPU sensor to power up
    Task_sleep(100000 / Clock_tickPeriod);
    System_printf("MPU9250: Power ON\n");
    System_flush();

    // MPU open i2c
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL) {
        System_abort("Error Initializing I2CMPU\n");
    }

    // MPU setup and calibration
    System_printf("MPU9250: Setup and calibration...\n");
    System_flush();

    mpu9250_setup(&i2cMPU);

    System_printf("MPU9250: Setup and calibration OK\n");
    System_flush();

    //MPU close i2c
    I2C_close(i2cMPU);
    //MPU power off
    //PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);

    // Open I2C
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    // open i2c connection
    i2c = I2C_open(Board_I2C_TMP, &i2cParams);
    if (i2c == NULL) {
      System_abort("Error Initializing I2C\n");
    }

    // JTKJ: Tehtävä 2. Alusta sensorin OPT3001 setup-funktiolla
    //       Laita enne funktiokutsua eteen 100ms viive (Task_sleep)
    // JTKJ: Exercise 2. Setup the OPT3001 sensor for use
    //       Before calling the setup function, insertt 100ms delay with Task_sleep
    Task_sleep(1000 / Clock_tickPeriod);
    opt3001_setup(&i2c);
    I2C_close(i2c);

    while (1) {

        // JTKJ: Tehtävä 2. Lue sensorilta dataa ja tulosta se Debug-ikkunaan merkkijonona
        // JTKJ: Exercise 2. Read sensor data and print it to the Debug window as string

        /*
        sprintf(msg,"valoisuus: %.2f luxia\n",valoisuus);
        System_printf(msg);
        System_flush();
        */

        // JTKJ: Tehtävä 3. Tallenna mittausarvo globaaliin muuttujaan
        //       Muista tilamuutos
        // JTKJ: Exercise 3. Save the sensor value into the global variable
        //       Remember to modify state

        //open i2c
        i2c = I2C_open(Board_I2C_TMP, &i2cParams);
        if (i2c == NULL) {
          System_abort("Error Initializing I2C\n");
        }
        //ambientLight = opt3001_get_data(&i2c);

        //close i2c
        I2C_close(i2c);

        if (programState == MOVE_DETECTION){
            programState = MOVE_DETECTION_DATA_READY;
            if ((clockTicks - mpuStartTicks) * Clock_tickPeriod < 4000000 ){
                //MPU open i2c
                i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
                if (i2cMPU == NULL) {
                   System_abort("Error Initializing I2CMPU\n");
                }
                // MPU ask data
                mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
                //time
                time = Clock_getTicks() * Clock_tickPeriod / 1000;
                //MPU close i2c
                I2C_close(i2cMPU);

                /*
                //Test printing data. !Slow operation!
                sprintf(msg, "dataIndex: %.d \n",dataIndex);
                System_printf(msg);
                sprintf(msg, "time: %.2f ax: %.2f, ay: %.2f, az: %.2f, gx: %.2f, gy: %.2f, gz: %.2f\n",time ,ax, ay, az, gx, gy, gz);
                System_printf(msg);
                System_flush();
                */

                sprintf(uartMsg, "time:%.2f,ax:%.2f,ay:%.2f,az:%.2f,gx:%.2f,gy:%.2f,gz:%.2f\n",time ,ax, ay, az, gx, gy, gz);
                uartState = SEND_MSG;

                //saving data to lists
                timeData[dataIndex] = time;
                axData[dataIndex] = ax;
                ayData[dataIndex] = ay;
                azData[dataIndex] = az;
                gxData[dataIndex] = gx;
                gyData[dataIndex] = gy;
                gzData[dataIndex] = gz;
                dataIndex++;

                sprintf(uartMsg, "MSG1:MENU: LED-game");
                uartState = SEND_MSG;

            } else {
                programState = MUSIC;
                nextState = MOVE_DETECTION_ALGORITHM;
                music = dataReadyMusic;
                dataIndex = 0;
                System_printf("Data ready for movement algorithm.\n");
                System_flush();
            }
        }

        // Just for sanity check for exercise, you can comment this out
        //System_printf("sensorTask\n");
        //System_flush();

        // 20 times per second, you can modify this
        Task_sleep(50000 / Clock_tickPeriod);
    }
}



Void mainTaskFxn(UArg arg0, UArg arg1) {
    //vihre� led palaa aluksi
    System_printf("b0: Move your tamagotchi\nb1: next state\n---\n");
    PIN_setOutputValue( ledHandle, Board_LED0, 1 );
    int blinkAccelator = 1;



    while (1){

        //soittaa musiikkia
        if(programState == MUSIC) {
            //System_printf("Playing music.\n");
            //System_flush();
            playMusic(buzzerHandle, music, 144);
            programState = nextState;
            //System_printf("programState: nextState\n");
        } else if (programState == GAME) {
            if ((clockTicks - gameStartTicks) * Clock_tickPeriod / 1000 > 2000 - blinkAccelator) {
                //System_printf("led changes/n");
                //System_flush();
                pinValue_0 = PIN_getOutputValue( Board_LED0 );
                pinValue_0 = !pinValue_0;
                PIN_setOutputValue( ledHandle, Board_LED0, pinValue_0 );
                pinValue_1 = PIN_getOutputValue( Board_LED1 );
                pinValue_1 = !pinValue_1;
                PIN_setOutputValue( ledHandle, Board_LED1, pinValue_1 );
                if (blinkAccelator < 1800) {
                    blinkAccelator = blinkAccelator + 100;
                }
                gameStartTicks = clockTicks;
            }
        } else if (programState == MOVE_DETECTION_ALGORITHM){
            int peaks = peakCount(timeData, axData, dataSize, 0.25, 1, 0);
            char msg[30];
            sprintf(msg, "peakCount without error margin = %d\n", peaks);
            System_printf(msg);
            peaks = peakCountMargin(&time, &ax, &ay, &az, dataSize, 'x', 0.25, 0.1, 0);
            sprintf(msg, "peakCount with error margin = %d\n", peaks);
            System_printf(msg);
            System_flush();

            clearAllData();
            //send message
            sprintf(uartMsg, "session:end,EAT:8,EXERCISE:1");
            uartState = SEND_MSG;
            programState = MUSIC;
            nextState = MENU;
            music = menuMusic;
        }

        System_flush();
        Task_sleep(50000 / Clock_tickPeriod);
    }
}

Void dataTaskFxn(UArg arg0, UArg arg1){
    char msg[10];
    dataIndex = 0;

    while (1){

        if(programState == SEND_DATA){
            programState = MUSIC;
            nextState = MENU;
            music = backMusic;
            sendData();
            System_printf("data sent.");
        } else if(programState == MOVE_DETECTION_DATA_READY)
        {
            programState = MOVE_DETECTION;
            //save data to lists
            /*
            timeData[dataIndex] = time - timeData[0];
            axData[dataIndex] = ax;
            ayData[dataIndex] = ay;
            azData[dataIndex] = az;
            gxData[dataIndex] = gx;
            gyData[dataIndex] = gy;
            gzData[dataIndex] = gz;
            */
            /*
            sprintf(msg, "aika: %d\n", timeData[dataIndex]);
            System_printf(msg);
            sprintf(msg,"valoisuus: %.2f luxia\n",ambientLight);
            System_printf(msg);
            */
            /*
            sprintf(msg, "dataIndex: %.d \n",dataIndex);
            System_printf(msg);
            sprintf(msg, "time: %.2f ax: %.2f, ay: %.2f, az: %.2f, gx: %.2f, gy: %.2f, gz: %.2f\n",time ,ax, ay, az, gx, gy, gz);
            System_printf(msg);
            System_flush();
            */

            /*
            dataIndex++;
            if (dataIndex == dataSize){
                dataIndex = 0;
            }
            */
        }

        //System_printf("dataTask\n");
        //System_flush();

        Task_sleep(50000 / Clock_tickPeriod);
    }
}

//Own functions
void sendData(){
    //sends data
    char msgg[30];
    int i;
    sprintf(msgg, "[time, ax, ay, az, gx, gy, gz]\n");
    System_printf(msgg);
    for(i = 0; i < dataSize; i++){
        /*
        index = dataIndex + i;
        if (index >= dataSize){
            index = index - dataSize;
        }
        */

        sprintf(msgg, "%05f,%05f,%05f,%05f,%05f,%05f,%05f\n",
                timeData[i] - timeData[0],
                axData[i],
                ayData[i],
                azData[i],
                gxData[i],
                gyData[i],
                gzData[i]);
        System_printf(msgg);
        System_flush();
    }
    clearAllData();
}

void playMusic(PIN_Handle buzzerPin, int *note, int tempo){

    // this calculates the duration of a whole note in ms (60s/tempo)*4 beats
    int wholenote = (60000000 * 4) / tempo;

    int divider = 0, noteDuration = 0;

    //char msg[10];



    // code modifief from: https://github.com/robsoncouto/arduino-songs
    // iterate over the notes of the melody.
    // Remember, the array is twice the number of notes (notes + durations)

    for (note; *note != -1; note = note + 2) {
        if (programState == MUSIC) {
            // calculates the duration of each note
            divider = *(note + 1);
            if (divider > 0) {
                // regular note, just proceed
                noteDuration = (wholenote) / divider;
            } else if (divider < 0) {
                // dotted notes are represented with negative durations!!
                noteDuration = (wholenote) / abs(divider);
                noteDuration *= 1.5; // increases the duration in half for dotted notes
            }

            // we only play the note for 90% of the duration, leaving 10% as a pause
            buzzerOpen(buzzerPin);
            buzzerSetFrequency(*note);
            Task_sleep(noteDuration*0.9 / Clock_tickPeriod);

            /*for testing purposes
            sprintf(msg, "taajuus: %d\n", *note);
            System_printf(msg);
            System_flush();
            */

            // stop buzzer before the next note.
            buzzerClose();

            // Wait for the specified duration before playing the next note.
            Task_sleep(noteDuration*0.1 / Clock_tickPeriod);
        } else {
            nextState = MENU;
            System_printf("Music stop!\n");
            System_flush();
            break;
        }
    }
}

void clearAllData(){
    clearData(axData, dataSize);
    clearData(ayData, dataSize);
    clearData(azData, dataSize);
    clearData(gxData, dataSize);
    clearData(gyData, dataSize);
    clearData(gzData, dataSize);
    clearData(timeData, dataSize);
    System_printf("All data cleared!\n");
    System_flush();
}


 Int main(void) {

    // Task variables
    Task_Handle sensorTaskHandle;
    Task_Params sensorTaskParams;
    Task_Handle uartTaskHandle;
    Task_Params uartTaskParams;
    Task_Handle dataTaskHandle;
    Task_Params dataTaskParams;
    Task_Handle mainTaskHandle;
    Task_Params mainTaskParams;


    // Initialize board
    Board_initGeneral();
    Init6LoWPAN();
    
    //Initialize i2c bus
    Board_initI2C();
    //Initialize UART
    Board_initUART();

    //button and led
    button0Handle = PIN_open(&button0State, button0Config);
    if(!button0Handle) {
        System_abort("Error initializing button0 pins\n");
    }
    button1Handle = PIN_open(&button1State, button1Config);
    if(!button1Handle) {
        System_abort("Error initializing button1 pins\n");
    }
    ledHandle = PIN_open(&ledState, led1Config);
    if(!ledHandle) {
       System_abort("Error initializing LED pins\n");
    }
    ledHandle = PIN_open(&ledState, led0Config);
    if(!ledHandle) {
    System_abort("Error initializing LED pins\n");
    }

    if (PIN_registerIntCb(button0Handle, &button0Fxn) != 0) {
       System_abort("Error registering button0 callback function");
    }
    if (PIN_registerIntCb(button1Handle, &button1Fxn) != 0) {
       System_abort("Error registering button1 callback function");
    }

    // Initialize clock
    Clock_Params_init(&clkParams);
    clkParams.period = 50000 / Clock_tickPeriod; //50ms
    clkParams.startFlag = TRUE;

    clkHandle = Clock_create((Clock_FuncPtr)clkFxn, 50000 / Clock_tickPeriod, &clkParams, NULL);
    if (clkHandle == NULL) {
      System_abort("Clock create failed");
    }

    // Buzzer
    buzzerHandle = PIN_open(&buzzerState, buzzerConfig);
    if (buzzerHandle == NULL) {
    System_abort("Pin open failed!");
    }

    /* Task */
    Task_Params_init(&sensorTaskParams);
    sensorTaskParams.stackSize = STACKSIZE;
    sensorTaskParams.stack = &sensorTaskStack;
    sensorTaskParams.priority=2;
    sensorTaskHandle = Task_create(sensorTaskFxn, &sensorTaskParams, NULL);
    if (sensorTaskHandle == NULL) {
        System_abort("sensorTask create failed!");
    }

    Task_Params_init(&dataTaskParams);
    dataTaskParams.stackSize = STACKSIZE;
    dataTaskParams.stack = &dataTaskStack;
    dataTaskParams.priority=2;
    dataTaskHandle = Task_create(dataTaskFxn, &dataTaskParams, NULL);
    if (dataTaskHandle == NULL) {
        System_abort("dataTask create failed!");
    }

    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority=2;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL) {
        System_abort("uartTask create failed!");
    }


    Task_Params_init(&mainTaskParams);
    mainTaskParams.stackSize = STACKSIZE;
    mainTaskParams.stack = &mainTaskStack;
    mainTaskParams.priority=2;
    mainTaskHandle = Task_create(mainTaskFxn, &mainTaskParams, NULL);
    if (mainTaskHandle == NULL) {
        System_abort("mainTask create failed!");
    }

    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
