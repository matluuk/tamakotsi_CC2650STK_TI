/* C Standard library */
#include <stdio.h>

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
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/UART.h>

/* Board Header files */
#include "Board.h"
#include "wireless/comm_lib.h"
#include "sensors/opt3001.h"

/* Task */
#define STACKSIZE 2048
Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];
Char dataTaskStack[STACKSIZE];

// JTKJ: Tehtävä 3. Tilakoneen esittely
// JTKJ: Exercise 3. Definition of the state machine
enum state { WAITING=1, DATA_READY, UART_MSG_READY };
enum state programState = WAITING;

// JTKJ: Tehtävä 3. Valoisuuden globaali muuttuja
// JTKJ: Exercise 3. Global variable for ambient light
float ambientLight = -1000.0;
char uartBuffer[10];
float lightData[32];

// JTKJ: Tehtävä 1. Lisää painonappien RTOS-muuttujat ja alustus
static PIN_Handle buttonHandle;
static PIN_State buttonState;
static PIN_Handle ledHandle;
static PIN_State ledState;



PIN_Config buttonConfig[] = {
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE
};

PIN_Config ledConfig[] = {
   Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE
};

void buttonFxn(PIN_Handle handle, PIN_Id pinId) {

    // JTKJ: Tehtävä 1. Vilkuta jompaa kumpaa lediä
    // JTKJ: Exercise 1. Blink either led of the device
    uint_t pinValue = PIN_getOutputValue( Board_LED1 );
    pinValue = !pinValue;
    PIN_setOutputValue( ledHandle, Board_LED1, pinValue );
    System_printf("button pressed!\n");
    System_flush();

    //l�hett�� datan napinpainalluksella
    char msgg[10];
    int i;
    for(i = 0; i < 32; i++){
        sprintf(msgg, "%.4f,", lightData[i]);
        System_printf(msgg);
        System_flush();
    }
    System_printf("\n");
    System_flush();
}

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

/* Task Functions */
static void uartTaskFxn(UArg arg0, UArg arg1) {

    char merkkijono[32];
    char testiviesti[] = "id:64,EAT:8,ping\n\r";

    // JTKJ: Tehtävä 4. Lisää UARTin alustus: 9600,8n1
    // JTKJ: Exercise 4. Setup here UART connection as 9600,8n1

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

        Task_sleep(100000 / Clock_tickPeriod);
    }
}

Void sensorTaskFxn(UArg arg0, UArg arg1) {

    I2C_Handle      i2c;
    I2C_Params      i2cParams;
    double valoisuus;
    char merkkijono[32];

    // JTKJ: Tehtävä 2. Avaa i2c-väylä taskin käyttöön
    // JTKJ: Exercise 2. Open the i2c bus
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    // Avataan yhteys
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

    while (1) {

        // JTKJ: Tehtävä 2. Lue sensorilta dataa ja tulosta se Debug-ikkunaan merkkijonona
        // JTKJ: Exercise 2. Read sensor data and print it to the Debug window as string
        valoisuus = opt3001_get_data(&i2c);

        /*
        sprintf(merkkijono,"valoisuus: %.2f luxia\n",valoisuus);
        System_printf(merkkijono);
        System_flush();
        */

        // JTKJ: Tehtävä 3. Tallenna mittausarvo globaaliin muuttujaan
        //       Muista tilamuutos
        // JTKJ: Exercise 3. Save the sensor value into the global variable
        //       Remember to modify state

        if ((valoisuus >= 0) && (programState == WAITING)){
            programState = DATA_READY;

            ambientLight = valoisuus;
        }

        // Just for sanity check for exercise, you can comment this out
        //System_printf("sensorTask\n");
        //System_flush();

        // Once per second, you can modify this
        Task_sleep(1000000 / Clock_tickPeriod);
    }
}

Void dataTaskFxn(UArg arg0, UArg arg1){
    int i = 0;
    char merkkijono[10];

    while (1){

        if(programState == DATA_READY){
            lightData[i] = ambientLight;
            sprintf(merkkijono,"valoisuus: %.2f luxia\n\r",ambientLight);
            System_printf(merkkijono);
            System_flush();
            if (i == 32){
                i = 0;
            }else{
                i++;
            }
            programState = WAITING;
        }

        //System_printf("dataTask\n");
        //System_flush();

        Task_sleep(100000 / Clock_tickPeriod);
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
    
    // JTKJ: Tehtävä 2. Ota i2c-väylä käyttöön ohjelmassa
    // JTKJ: Exercise 2. Initialize i2c bus
    Board_initI2C();
    // JTKJ: Tehtävä 4. Ota UART käyttöön ohjelmassa
    // JTKJ: Exercise 4. Initialize UART
    Board_initUART();

    // JTKJ: Tehtävä 1. Ota painonappi ja ledi ohjelman käyttöön
    //       Muista rekisteröidä keskeytyksen käsittelijä painonapille
    buttonHandle = PIN_open(&buttonState, buttonConfig);
    if(!buttonHandle) {
        System_abort("Error initializing button pins\n");
    }
    ledHandle = PIN_open(&ledState, ledConfig);
    if(!ledHandle) {
       System_abort("Error initializing LED pins\n");
    }
    if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
       System_abort("Error registering button callback function");
    }
    /* Task */
    Task_Params_init(&sensorTaskParams);
    sensorTaskParams.stackSize = STACKSIZE;
    sensorTaskParams.stack = &sensorTaskStack;
    sensorTaskParams.priority=2;
    sensorTaskHandle = Task_create(sensorTaskFxn, &sensorTaskParams, NULL);
    if (sensorTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority=2;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL) {
        System_abort("Task create failed!");
    }


    Task_Params_init(&dataTaskParams);
    dataTaskParams.stackSize = STACKSIZE;
    dataTaskParams.stack = &dataTaskStack;
    dataTaskParams.priority=2;
    dataTaskHandle = Task_create(dataTaskFxn, &dataTaskParams, NULL);
    if (dataTaskHandle == NULL) {
        System_abort("Task create failed!");
    }


    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
