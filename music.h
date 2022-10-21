/*
 * music.h
 *
 *  Created on: 21.10.2022
 *      Author: matti
 */

#ifndef MUSIC_H_
#define MUSIC_H_

//includes
#include <ti/drivers/pin/PINCC26XX.h>

//Songs and sound effects:
//Consist of alternating frequency and duration in list.
//Ends to -1 frequency.
//0 frequency is pause.
//minus durations are dotted notes
int hedwigsTheme[] = {
      // Hedwig's theme fromn the Harry Potter Movies
      // Socre from https://musescore.com/user/3811306/scores/4906610
      0,2,294,4,392,-4,466,8,440,4,392,2,587,4,523,-2,440,-2,392,-4,466,8,440,4,349,2,415,4,294,-1,294,4,392,-4,466,8,440,4,392,2,587,4,698,2,659,4,622,2,494,4,622,-4,587,8,554,4,277,2,494,4,392,-1,466,4,587,2,466,4,587,2,466,4,622,2,587,4,554,2,440,4,466,-4,587,8,554,4,277,2,294,4,587,-1,0,4,466,4,587,2,466,4,587,2,466,4,698,2,659,4,622,2,494,4,622,-4,587,8,554,4,277,2,466,4,392,-1,-1
    };

void playMusic(PIN_Handle buzzerPin,int *note, int tempo);




#endif /* MUSIC_H_ */
