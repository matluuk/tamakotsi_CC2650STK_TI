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

//music
static int hedwigsThemeMusic[] = {
    // Hedwig's theme from the Harry Potter Movies
    // Score from https://musescore.com/user/3811306/scores/4906610
                      0,2,294,4,392,-4,466,8,440,4,392,2,587,4,523,-2,440,-2,392,-4,466,8,440,4,349,2,415,4,294,-1,294,4,392,-4,466,8,440,4,392,2,587,4,698,2,659,4,622,2,494,4,622,-4,587,8,554,4,277,2,494,4,392,-1,466,4,587,2,466,4,587,2,466,4,622,2,587,4,554,2,440,4,466,-4,587,8,554,4,277,2,294,4,587,-1,0,4,466,4,587,2,466,4,587,2,466,4,698,2,659,4,622,2,494,4,622,-4,587,8,554,4,277,2,466,4,392,-1,-1
};
//test sound
static int dataReadyMusic[] = {
                   440, 4, 550, 8, 660, 16, 660, 16, 440, 4, 440, 4, -1
};
static int backMusic[] = {
              400, 2, 200, 4, 300, 4, -1
};
static int chooseMusic[] = {
                300, 16, 400, 16, -1
};
static int moveMusic[] = {
              340, 16, 400, 16,300, 16, 400, 16, -1
};
static int listenMusic[] = {
              400, 16, 370, 16, 440, 4, -1
};
static int gameMusic[] = {
                170, 8, 300, 16, 300, 8, 500, 16, -1
};
static int gamePointMusic[] = {
                319, 16, 510, 16, -1
};
static int gameEndMusic[] = {
                   415, 16, 400, 16, 380, 16 ,230, 32, 230, 32, 230,23, 190, 2, -1
};
static int lowFeaturesMusic[] = {
                   600, 4, 600, 4, 600, 4, -1
};
static int menuMusic[] = {560, 16, 530, 16, 760, 16 ,870, 32, 230, 32, 340,23, 1000, 2, -1
};



#endif /* MUSIC_H_ */
