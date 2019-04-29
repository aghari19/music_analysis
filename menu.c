/*
 * menu.c
 *
 *  Created on: Apr 29, 2019
 *      Author: abarh
 */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>

#include "ButtonLED_HAL.h"

#define milsec 60000

typedef enum {FFT,metronome,note_detection} options;

button_t BoostS1 = {GPIO_PORT_P5, GPIO_PIN1, Stable_R, RELEASED_STATE, TIMER32_0_BASE};
button_t BoostS2 = {GPIO_PORT_P3, GPIO_PIN5, Stable_R, RELEASED_STATE, TIMER32_1_BASE};
button_t LaunchL = {GPIO_PORT_P1, GPIO_PIN1, Stable_R, RELEASED_STATE, TIMER32_0_BASE};
button_t LaunchR = {GPIO_PORT_P1, GPIO_PIN4, Stable_R, RELEASED_STATE, TIMER32_1_BASE};

void metronome_play(bool Booster1_Pressed,bool Booster2_Pressed);

void menu()
{
    static options option = FFT;

    bool Booster1_Pressed  = false;
    bool Booster2_Pressed = false;
    bool LaunchL_Pressed  = false;
    bool LaunchR_Pressed = false;

    Booster1_Pressed = ButtonPushed(&BoostS1);
    Booster2_Pressed = ButtonPushed(&BoostS2);
    LaunchL_Pressed = ButtonPushed(&LaunchL);
    LaunchR_Pressed = ButtonPushed(&LaunchR);

    switch(option)
    {
        case FFT:
            turnOff_BoosterpackLED_blue();
            turnOff_BoosterpackLED_green();
            turnOn_BoosterpackLED_red();
            if(LaunchL_Pressed)
            {
                option = metronome;
            }
            else if(LaunchR_Pressed)
            {
                option = note_detection;
            }
            break;
        case metronome:
            turnOff_BoosterpackLED_red();
            turnOff_BoosterpackLED_green();
            turnOn_BoosterpackLED_blue();
            metronome_play(Booster1_Pressed, Booster2_Pressed);
            if(LaunchL_Pressed)
            {
                option = note_detection;
            }
            else if(LaunchR_Pressed)
            {
                option = FFT;
            }
            break;
        case note_detection:
            turnOff_BoosterpackLED_red();
            turnOff_BoosterpackLED_blue();
            turnOn_BoosterpackLED_green();
            if(LaunchL_Pressed)
            {
                option = FFT;
            }
            else if(LaunchR_Pressed)
            {
                option = metronome;
            }
            break;
    }
}

void metronome_play(bool Booster1_Pressed,bool Booster2_Pressed)
{
    turnOn_LaunchpadLED2_green();
    static int BPM = 100;

    if(Booster1_Pressed)
    {
        BPM = BPM + 10;
    }
    else if(Booster2_Pressed)
    {
        BPM = BPM - 10;
    }
}
