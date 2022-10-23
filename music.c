/*
 * music.c
 *
 *  Created on: 21.10.2022
 *      Author: matti
 */
#include "music.h"

#include <ti/sysbios/knl/Clock.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/System.h>

#include "board.h"
#include "buzzer.h"

/*******************************************************************************
 * @fn          playMusic
 *
 * @brief       playMusic with listo of
 *
 * @descr       Initializes pin and PWM
 *
 * @return      -
 */
/*
void playMusic(PIN_Handle buzzerPin, int *note, int tempo){

    //

    // this calculates the duration of a whole note in ms (60s/tempo)*4 beats
    int wholenote = (60000000 * 4) / tempo;

    int divider = 0, noteDuration = 0;

    char msg[10];

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

        for testing purposes
        sprintf(msg, "taajuus: %d\n", *note);
        System_printf(msg);
        System_flush();


        // Wait for the specified duration before playing the next note.
        Task_sleep(noteDuration*0.1 / Clock_tickPeriod);

        // stop the waveform generation before the next note.
        buzzerClose();
    }
}
*/
