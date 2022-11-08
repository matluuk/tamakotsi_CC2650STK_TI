/* C Standard library */
#include <stdio.h>
#include <inttypes.h>

/* testi kommenttia*/

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

/* Task */
#define STACKSIZE 2048
Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];
Char dataTaskStack[STACKSIZE];

//Tilamuuttujat
enum state { WAITING=1, DATA_READY, UART_MSG_READY, MUSIC, SEND_DATA, MOVE_DETECTION, MOVE_DETECTION_DATA_READY };
enum state programState = WAITING;
//seuraava tilamuutos
enum state nextState;
//chosen music
int *music;
//Menu


//Globaalit muutujat
float ambientLight = -1000.0;
float ax, ay, az, gx, gy, gz;
char uartBuffer[10];

int dataIndex = 0;
int dataSize = 110;
float lightData[110];
float axData[110];
float ayData[110];
float azData[110];
float gxData[110];
float gyData[110];
float gzData[110];
int timeData[110];

//musiikkia
int hedwigsTheme[] = {
    // Hedwig's theme from the Harry Potter Movies
    // Score from https://musescore.com/user/3811306/scores/4906610
    0,2,294,4,392,-4,466,8,440,4,392,2,587,4,523,-2,440,-2,392,-4,466,8,440,4,349,2,415,4,294,-1,294,4,392,-4,466,8,440,4,392,2,587,4,698,2,659,4,622,2,494,4,622,-4,587,8,554,4,277,2,494,4,392,-1,466,4,587,2,466,4,587,2,466,4,622,2,587,4,554,2,440,4,466,-4,587,8,554,4,277,2,294,4,587,-1,0,4,466,4,587,2,466,4,587,2,466,4,698,2,659,4,622,2,494,4,622,-4,587,8,554,4,277,2,466,4,392,-1,-1
};
//testi ��ni
int testMusic[] = {
    440, 4, 440, 4, 440, 4, 440, 4, 440, 4, 440, 4, 440, 4, 440, 4, 440, 4, -1
};
int back[] = {
    400, 2, 200, 4, 300, 4, -1
};
int choose[] = {
                300, 16, 400, 16, -1
};


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
static PIN_State mpuState;

// RTOS:n kellomuuttujat
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

PIN_Config ledConfig[] = {
   Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE
};
PIN_Config buzzerConfig[] = {
  Board_BUZZER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
  PIN_TERMINATE
};
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

void button0Fxn(PIN_Handle handle, PIN_Id pinId) {

    // JTKJ: Tehtävä 1. Vilkuta jompaa kumpaa lediä
    // JTKJ: Exercise 1. Blink either led of the device
    uint_t pinValue = PIN_getOutputValue( Board_LED1 );
    pinValue = !pinValue;
    PIN_setOutputValue( ledHandle, Board_LED1, pinValue );
    System_printf("button0 pressed!\n");
    System_flush();

    if(programState == WAITING){
        programState = MUSIC;
        nextState = WAITING;
        music = testMusic;
    }
}

void button1Fxn(PIN_Handle handle, PIN_Id pinId) {
    System_printf("button1 pressed!\n");
    System_flush();

    if(programState == WAITING){
        programState = MUSIC;
        nextState = MOVE_DETECTION;
        music = choose;
        Clock_start(clkHandle);
    } else if (programState == MOVE_DETECTION || programState == MOVE_DETECTION_DATA_READY){
        programState = MUSIC;
        Clock_stop(clkHandle);
        nextState = WAITING;
        music = back;
        System_printf("MOVE_DETECTION stopped!\n");
        System_flush();
    }
}

/*
Void mpuFxn(PIN_Handle mpuHandle, PIN_Id pinId) {
      System_printf("MpuFxn");
      System_flush();
}
*/

static void uartFxn(UART_Handle uart, void *rxBuf, size_t len) {
    char msg[30];
    char msg2[30];
   // Nyt meill� on siis haluttu m��r� merkkej� k�ytett�viss�
   // rxBuf-taulukossa, pituus len, jota voimme k�sitell� halutusti
   // T�ss� ne annetaan argumentiksi toiselle funktiolle (esimerkin vuoksi)
   //tehdaan_jotain_nopeasti(rxBuf,len);

    /*if(programState == WAITING){
        programState = UART_MSG_READY;


    }*/

   sprintf(msg,"received: %c\n\r", uartBuffer[0]);
   //UART_write(uart, msg, strlen(msg));
   System_printf(msg);
   sprintf(msg2, "rxBuf: %x, len %d", rxBuf, len);
   System_printf(msg2);
   System_flush;

   // K�sittelij�n viimeisen� asiana siirryt��n odottamaan uutta keskeytyst�..
   UART_read(uart, rxBuf, 1);
}

// Kellokeskeytyksen k�sittelij�
Void clkFxn(UArg arg0) {
    System_printf("clkFxn\n");
    if (programState == MOVE_DETECTION || programState == MOVE_DETECTION_DATA_READY){
        programState = SEND_DATA;
        Clock_stop(clkHandle);
        System_printf("clkFxn_state_change\n");
    }
    System_flush();
}

/* Task Functions */
static void uartTaskFxn(UArg arg0, UArg arg1) {

    char merkkijono[32];
    char testiviesti[] = "id:64,EAT:8,ping\n\r";

    // UART-kirjaston asetukset
    UART_Handle uart;
    UART_Params uartParams;

    // Alustetaan sarjaliikenne
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
    UART_read(uart, uartBuffer, 1);

    while (1) {

        // JTKJ: Tehtävä 3. Kun tila on oikea, tulosta sensoridata merkkijonossa debug-ikkunaan
        //       Muista tilamuutos
        // JTKJ: Exercise 3. Print out sensor data as string to debug window if the state is correct
        //       Remember to modify state

        /*
        if(programState == DATA_READY){
            programState = WAITING;

            sprintf(merkkijono,"valoisuus: %.2f luxia\n\r",ambientLight);
            System_printf(merkkijono);
            System_flush();

            // JTKJ: Tehtävä 4. Lähetä sama merkkijono UARTilla
            // JTKJ: Exercise 4. Send the same sensor data string with UART
            //UART_write(uart, merkkijono, strlen(merkkijono));

            //testiviesti
            //UART_write(uart, testiviesti, strlen(testiviesti));

        }*/

        // Just for sanity check for exercise, you can comment this out
        //System_printf("uartTask\n");
        //System_flush();

        Task_sleep(200000 / Clock_tickPeriod);
    }
}

Void sensorTaskFxn(UArg arg0, UArg arg1) {

    I2C_Handle      i2c;
    I2C_Params      i2cParams;
    I2C_Handle i2cMPU; // Own i2c-interface for MPU9250 sensor
    I2C_Params i2cMPUParams;
    double valoisuus;
    char merkkijono[32];

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
        sprintf(merkkijono,"valoisuus: %.2f luxia\n",valoisuus);
        System_printf(merkkijono);
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



            //MPU open i2c
            i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
            if (i2cMPU == NULL) {
                System_abort("Error Initializing I2CMPU\n");
            }
            // MPU ask data
            mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
            //MPU close i2c
            I2C_close(i2cMPU);
        }

        // Just for sanity check for exercise, you can comment this out
        //System_printf("sensorTask\n");
        //System_flush();

        // 5 times per second, you can modify this
        Task_sleep(50000 / Clock_tickPeriod);
    }
}

Void dataTaskFxn(UArg arg0, UArg arg1){
    char merkkijono[10];

    while (1){

        //soittaa musiikkia, pit�isi siirt�� johonkin toiseen taskiin
        if(programState == MUSIC){
            programState = nextState;
            playMusic(buzzerHandle, music, 144);
        }

        if(programState == SEND_DATA){
            programState = WAITING;
            sendData();
            System_printf("data sent.");
        }


        if(programState == MOVE_DETECTION_DATA_READY){
            programState = MOVE_DETECTION;
            lightData[dataIndex] = ambientLight;
            axData[dataIndex] = ax;
            ayData[dataIndex] = ay;
            azData[dataIndex] = az;
            gxData[dataIndex] = gx;
            gyData[dataIndex] = gy;
            gzData[dataIndex] = gz;
            timeData[dataIndex] = Clock_getTicks()*Clock_tickPeriod/1000;//aika mikrosekunteina
            /*
            sprintf(merkkijono, "aika: %d\n", timeData[dataIndex]);
            System_printf(merkkijono);
            sprintf(merkkijono,"valoisuus: %.2f luxia\n",ambientLight);
            System_printf(merkkijono);
            */
            sprintf(merkkijono, "ax: %.2f, ay: %.2f, az: %.2f, gx: %.2f, gy: %.2f, gz: %.2f\n", ax, ay, az, gx, gy, gz);
            System_printf(merkkijono);
            System_flush();

            dataIndex++;
            if (dataIndex == dataSize){
                dataIndex = 0;
            }
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
    int index;
    sprintf(msgg, "[time, ax, ay, az, gx, gy, gz]\n");
    System_printf(msgg);
    for(i = 0; i < dataSize; i++){
        index = i;
        /*
        index = dataIndex + i;
        if (index >= dataSize){
            index = index - dataSize;
        }
        */

        sprintf(msgg, "%08d,%.4f,%05f,%05f,%05f,%05f,%05f\n",
                (timeData[index] - timeData[0]),
                axData[index],
                ayData[index],
                azData[index],
                gxData[index],
                gyData[index],
                gzData[index]);
        System_printf(msgg);
        System_flush();
    }
}

void playMusic(PIN_Handle buzzerPin, int *note, int tempo){

    //

    // this calculates the duration of a whole note in ms (60s/tempo)*4 beats
    int wholenote = (60000000 * 4) / tempo;

    int divider = 0, noteDuration = 0;

    //char msg[10];

    System_printf("funktio\n");

    // code modifief from: https://github.com/robsoncouto/arduino-songs
    // iterate over the notes of the melody.
    // Remember, the array is twice the number of notes (notes + durations)
    for (note; *note != -1; note = note + 2) {
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

        // stop the waveform generation before the next note.
        buzzerClose();

        // Wait for the specified duration before playing the next note.
        Task_sleep(noteDuration*0.1 / Clock_tickPeriod);
    }
}

Int main(void) {

    // Task variables
    Task_Handle sensorTaskHandle;
    Task_Params sensorTaskParams;
    Task_Handle uartTaskHandle;
    Task_Params uartTaskParams;
    Task_Handle dataTaskHandle;
    Task_Params dataTaskParams;

    // Initialize board
    Board_initGeneral();
    Init6LoWPAN();
    
    //Initialize i2c bus
    Board_initI2C();
    //Initialize UART
    Board_initUART();

    //painonappi ja ledi k�ytt��n
    button0Handle = PIN_open(&button0State, button0Config);
    if(!button0Handle) {
        System_abort("Error initializing button0 pins\n");
    }
    button1Handle = PIN_open(&button1State, button1Config);
    if(!button1Handle) {
        System_abort("Error initializing button1 pins\n");
    }
    ledHandle = PIN_open(&ledState, ledConfig);
    if(!ledHandle) {
       System_abort("Error initializing LED pins\n");
    }

    if (PIN_registerIntCb(button0Handle, &button0Fxn) != 0) {
       System_abort("Error registering button0 callback function");
    }
    if (PIN_registerIntCb(button1Handle, &button1Fxn) != 0) {
       System_abort("Error registering button1 callback function");
    }

    // Alustetaan kello
    Clock_Params_init(&clkParams);
    clkParams.period = 5000000 / Clock_tickPeriod;
    clkParams.startFlag = FALSE;

    // Otetaan k�ytt��n ohjelmassa
    clkHandle = Clock_create((Clock_FuncPtr)clkFxn, 5000000 / Clock_tickPeriod, &clkParams, NULL);
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

    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
