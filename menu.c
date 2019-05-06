/*
 * menu.c
 *
 *  Created on: Apr 29, 2019
 *      Author: abarh
 */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/grlib/grlib.h>

#include "ButtonLED_HAL.h"
#include "Timer.h"
#include "sound.h"
#include "graphics.h"

#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include <arm_math.h>
#include <arm_const_structs.h>

#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"
#include "HAL_I2C.h"
#include "HAL_OPT3001.h"

#define TEST_LENGTH_SAMPLES 2048
#define SAMPLE_LENGTH 2048

/* ------------------------------------------------------------------
 * Global variables for FFT Bin Example
 * ------------------------------------------------------------------- */
uint32_t fftSize = SAMPLE_LENGTH;
uint32_t ifftFlag = 0;
uint32_t doBitReverse = 1;
volatile arm_status status;

/* Graphic library context */
Graphics_Context g_sContext;

#define SMCLK_FREQUENCY     48000000
#define SAMPLE_FREQUENCY    8000

/* DMA Control Table */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(MSP_EXP432P401RLP_DMAControlTable, 4096)
#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment=4096
#elif defined(__GNUC__)
__attribute__ ((aligned (4096)))
#elif defined(__CC_ARM)
__align(4096)
#endif
static DMA_ControlTable MSP_EXP432P401RLP_DMAControlTable[32];

/* FFT data/processing buffers*/
float hann[SAMPLE_LENGTH];
int16_t data_array1[SAMPLE_LENGTH];
int16_t data_array2[SAMPLE_LENGTH];
int16_t data_input[SAMPLE_LENGTH * 2];
int16_t data_output[SAMPLE_LENGTH];

volatile int switch_data = 0;

uint32_t color = 0;

/* Timer_A PWM Configuration Parameter */

void configureMic()
{
    Timer_A_PWMConfig pwmConfig =
    {
        TIMER_A_CLOCKSOURCE_SMCLK,
        TIMER_A_CLOCKSOURCE_DIVIDER_1,
        (SMCLK_FREQUENCY / SAMPLE_FREQUENCY),
        TIMER_A_CAPTURECOMPARE_REGISTER_1,
        TIMER_A_OUTPUTMODE_SET_RESET,
        (SMCLK_FREQUENCY / SAMPLE_FREQUENCY) / 2
    };

    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN3,
                                                       GPIO_TERTIARY_MODULE_FUNCTION);
}

#define milsec 60000

typedef enum {FFT,metronome,note_detection} options;

button_t BoostS1 = {GPIO_PORT_P5, GPIO_PIN1, Stable_R, RELEASED_STATE, TIMER32_0_BASE};
button_t BoostS2 = {GPIO_PORT_P3, GPIO_PIN5, Stable_R, RELEASED_STATE, TIMER32_1_BASE};
button_t LaunchL = {GPIO_PORT_P1, GPIO_PIN1, Stable_R, RELEASED_STATE, TIMER32_0_BASE};
button_t LaunchR = {GPIO_PORT_P1, GPIO_PIN4, Stable_R, RELEASED_STATE, TIMER32_1_BASE};


void metronome_play(bool Booster1_Pressed,bool Booster2_Pressed);
void make_3digit_NumString(unsigned int num, char *string);
void FFT_play(bool issensor);
void note_detection_play(bool islight);
void tone_frequency(int maxIndex, bool islight);
void initial(bool issensor);
void initial_note();
void light_initial();

/* Variable for storing lux value returned from OPT3001 */
float lux;

/* Timer_A Up Configuration Parameter */
const Timer_A_UpModeConfig upConfig =
{
        TIMER_A_CLOCKSOURCE_ACLK,               // ACLK Clock SOurce
        TIMER_A_CLOCKSOURCE_DIVIDER_1,          // ACLK/1 = 3MHz
        200,                                    // 200 tick period
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE,    // Disable CCR0 interrupt
        TIMER_A_DO_CLEAR                        // Clear value
};

/* Timer_A Compare Configuration Parameter  (PWM) */
Timer_A_CompareModeConfig compareConfig_PWM =
{
        TIMER_A_CAPTURECOMPARE_REGISTER_3,          // Use CCR3
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,   // Disable CCR interrupt
        TIMER_A_OUTPUTMODE_TOGGLE_SET,              // Toggle output but
        100                                         // 50% Duty Cycle
};

void menu()
{
    static bool first = true;
    static bool second = false;
    static options option = FFT;
    static bool light_check = true;
    static bool islight = true;

    bool Booster1_Pressed  = false;
    bool Booster2_Pressed = false;
    bool LaunchL_Pressed  = false;
    bool LaunchR_Pressed = false;

    Booster1_Pressed = ButtonPushed(&BoostS1);
    Booster2_Pressed = ButtonPushed(&BoostS2);
    LaunchL_Pressed = ButtonPushed(&LaunchL);
    LaunchR_Pressed = ButtonPushed(&LaunchR);

    if(light_check)
    {
        light_initial();
        light_check = false;
    }

    lux = OPT3001_getLux();
    if(lux<=10)
    {
        islight = false;
    }
    else
    {
       islight = true;
    }
    switch(option)
    {
        case FFT:
            if(first)
            {
                initial(islight);
                first = false;
            }
            FFT_play(islight);
            if(LaunchL_Pressed)
            {
                option = metronome;
                if(islight)
                {
                    draw_white_rec(g_sContext);
                }
                else
                {
                    draw_black_rec(g_sContext);
                }
            }
            else if(LaunchR_Pressed)
            {
                option = note_detection;
                if(islight)
                {
                    draw_white_rec(g_sContext);
                }
                else
                {
                    draw_black_rec(g_sContext);
                }
            }
            break;
        case metronome:
            metronome_play(Booster1_Pressed, Booster2_Pressed);
            if(LaunchL_Pressed)
            {
                Timer_A_stopTimer(TIMER_A0_BASE);
                configureMic();
                second = true;
                option = note_detection;
                draw_white_rec(g_sContext);
            }
            else if(LaunchR_Pressed)
            {
                Timer_A_stopTimer(TIMER_A0_BASE);
                configureMic();
                option = FFT;
                draw_white_rec(g_sContext);
                first = true;
            }
            break;
        case note_detection:
            if(second)
            {
                initial_note();
                second = false;
            }
            configureMic();
            note_detection_play(islight);
            if(LaunchL_Pressed)
            {
                option = FFT;
                configureMic();
                draw_white_rec(g_sContext);
                first = true;
            }
            else if(LaunchR_Pressed)
            {
                option = metronome;
                draw_white_rec(g_sContext);
            }
            break;
    }
}

void note_detection_play(bool islight)
{
    static bool prev = false;
    lux = OPT3001_getLux();
    if(lux<=10)
    {
        islight = false;
    }
    else
    {
        islight = true;
    }

    if(islight)
    {
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_drawString(&g_sContext,(int8_t*) "NOTE DETECTION", -1, 25, 10, true);
    }
    else
    {
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
        Graphics_drawString(&g_sContext,(int8_t*) "NOTE DETECTION", -1, 25, 10, true);
    }
    if(prev != islight)
    {
      Graphics_clearDisplay(&g_sContext);
      prev = islight;
    }
    MAP_PCM_gotoLPM0();

    int i = 0;

    /* Computer real FFT using the completed data buffer */
    if(switch_data & 1)
    {
        for(i = 0; i < 2048; i++)
        {
            data_array1[i] = (int16_t)(hann[i] * data_array1[i]);
        }
        arm_rfft_instance_q15 instance;
        status = arm_rfft_init_q15(&instance, fftSize, ifftFlag,
                                   doBitReverse);

        arm_rfft_q15(&instance, data_array1, data_input);
    }
    else
    {
        for(i = 0; i < 2048; i++)
        {
            data_array2[i] = (int16_t)(hann[i] * data_array2[i]);
        }
        arm_rfft_instance_q15 instance;
        status = arm_rfft_init_q15(&instance, fftSize, ifftFlag,
                                   doBitReverse);

        arm_rfft_q15(&instance, data_array2, data_input);
    }

    /* Calculate magnitude of FFT complex output */
    for(i = 0; i < 4096; i += 2)
    {
        data_output[i /
                    2] =
            (int32_t)(sqrtf((data_input[i] *
                             data_input[i]) +
                            (data_input[i + 1] * data_input[i + 1])));
    }

    q15_t maxValue;
    uint32_t maxIndex = 0;

    arm_max_q15(data_output, fftSize, &maxValue, &maxIndex);
    tone_frequency(maxIndex, islight);
}

void tone_frequency(int Index, bool islight)
{
    int maxIndex = Index;
    maxIndex = (maxIndex * 4000) / 512;
    maxIndex = maxIndex + (4000 / (1024));
    if(islight)
    {
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_drawString(&g_sContext, (int8_t*) "###", -1, 50, 30, true);
        Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
                                        true);
        Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
                                        true);
        Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
                                        true);
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    }
    else
    {
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
        Graphics_drawString(&g_sContext, (int8_t*) "###", -1, 50, 30, true);
        Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
                                        true);
        Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
                                        true);
        Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
                                        true);
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    }
    if ((maxIndex >= 127) && (maxIndex < 140))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "C3", -1, 50, 30, true);
    }
    else if ((maxIndex >= 140) && (maxIndex < 157))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "D3", -1, 50, 30, true);
    }
    else if ((maxIndex >= 157) && (maxIndex < 170))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "E3", -1, 50, 30, true);
    }
    else if ((maxIndex >= 170) && (maxIndex < 190))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "F3", -1, 50, 30, true);
    }
    else if ((maxIndex >= 190) && (maxIndex < 205))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "G3", -1, 50, 30, true);
    }
    else if ((maxIndex >= 205) && (maxIndex < 230))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "A3", -1, 50, 30, true);
    }
    else if ((maxIndex >= 240) && (maxIndex < 251))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "B3", -1, 50, 30, true);
    }
    else if ((maxIndex > 252) && (maxIndex <= 271))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "C4", -1, 50, 30, true);
        if ((maxIndex > 252) && (maxIndex <= 256))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
                                true);
        }
        else if ((maxIndex > 256) && (maxIndex <= 266))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
                                true);
        }
        else if ((maxIndex > 266) && (maxIndex <= 271))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
                                true);
        }
    }
    else if ((maxIndex > 271) && (maxIndex <= 285))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "C#4", -1, 50, 30, true);
        if ((maxIndex >= 271) && (maxIndex <= 275))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 275) && (maxIndex <= 280))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 280) && (maxIndex <= 285))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex > 286) && (maxIndex <= 300))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "D4", -1, 50, 30, true);
        if ((maxIndex >= 286) && (maxIndex <= 290))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 290) && (maxIndex <= 295))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 295) && (maxIndex <= 300))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }

    }
    else if ((maxIndex > 303) && (maxIndex <= 319))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "D#4", -1, 50, 30, true);
        if ((maxIndex >= 303) && (maxIndex <= 308))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 308) && (maxIndex <= 315))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 315) && (maxIndex <= 319))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex > 321) && (maxIndex <= 340))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "E4", -1, 50, 30, true);
        if ((maxIndex >= 321) && (maxIndex <= 327))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 327) && (maxIndex <= 335))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 335) && (maxIndex <= 340))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex > 340) && (maxIndex <= 355))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "F4", -1, 50, 30, true);
        if ((maxIndex >= 340) && (maxIndex <= 346))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 346) && (maxIndex <= 353))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 353) && (maxIndex <= 355))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex > 355) && (maxIndex <= 380))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "F#4", -1, 50, 30, true);
        if ((maxIndex >= 355) && (maxIndex <= 363))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 363) && (maxIndex <= 375))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 375) && (maxIndex <= 380))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex > 380) && (maxIndex <= 400))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "G4", -1, 50, 30, true);
        if ((maxIndex >= 380) && (maxIndex <= 385))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 385) && (maxIndex <= 395))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 395) && (maxIndex <= 400))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex > 405) && (maxIndex <= 425))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "G#4", -1, 50, 30, true);
        if ((maxIndex >= 405) && (maxIndex <= 208))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 408) && (maxIndex <= 418))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 418) && (maxIndex <= 425))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex > 430) && (maxIndex <= 450))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "A4", -1, 50, 30, true);
        if ((maxIndex >= 430) && (maxIndex <= 435))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 435) && (maxIndex <= 443))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 443) && (maxIndex <= 450))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex > 455) && (maxIndex <= 480))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "A#4", -1, 50, 30, true);
        if ((maxIndex >= 455) && (maxIndex <= 461))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 461) && (maxIndex <= 470))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 470) && (maxIndex <= 480))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex > 483) && (maxIndex <= 510))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "B4", -1, 50, 30, true);
        if ((maxIndex >= 483) && (maxIndex <= 487))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 487) && (maxIndex <= 497))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 497) && (maxIndex <= 510))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }

    }
    else if ((maxIndex > 513) && (maxIndex < 540))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "C5", -1, 50, 30, true);
        if ((maxIndex >= 513) && (maxIndex <= 518))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 518) && (maxIndex <= 530))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 530) && (maxIndex <= 540))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 544) && (maxIndex < 565))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "C#5", -1, 50, 30, true);
        if ((maxIndex >= 544) && (maxIndex <= 550))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 554) && (maxIndex <= 560))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 560) && (maxIndex <= 565))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 570) && (maxIndex < 600))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "D5", -1, 50, 30, true);
        if ((maxIndex >= 570) && (maxIndex <= 580))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 580) && (maxIndex <= 591))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 591) && (maxIndex <= 600))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 610) && (maxIndex < 645))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "D#5", -1, 50, 30, true);
        if ((maxIndex >= 610) && (maxIndex <= 615))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 615) && (maxIndex <= 630))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 630) && (maxIndex <= 645))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 650) && (maxIndex < 680))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "E5", -1, 50, 30, true);
        if ((maxIndex >= 650) && (maxIndex <= 255))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 655) && (maxIndex <= 670))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 670) && (maxIndex <= 680))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 688) && (maxIndex < 725))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "F5", -1, 50, 30, true);
        if ((maxIndex >= 688) && (maxIndex <= 694))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 694) && (maxIndex <= 710))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 710) && (maxIndex <= 725))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 730) && (maxIndex < 760))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "F#5", -1, 50, 30, true);
        if ((maxIndex >= 730) && (maxIndex <= 736))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 736) && (maxIndex <= 745))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 750) && (maxIndex <= 760))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 770) && (maxIndex < 810))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "G5", -1, 50, 30, true);
        if ((maxIndex >= 770) && (maxIndex <= 776))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 776) && (maxIndex <= 790))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 790) && (maxIndex <= 810))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 815) && (maxIndex < 850))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "G#5", -1, 50, 30, true);
        if ((maxIndex >= 815) && (maxIndex <= 823))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 823) && (maxIndex <= 835))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 835) && (maxIndex <= 850))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 860) && (maxIndex < 900))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "A5", -1, 50, 30, true);
        if ((maxIndex >= 860) && (maxIndex <= 875))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 875) && (maxIndex <= 890))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 890) && (maxIndex <= 900))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 910) && (maxIndex < 960))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "A#5", -1, 50, 30, true);
        if ((maxIndex >= 910) && (maxIndex <= 923))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 923) && (maxIndex <= 940))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 940) && (maxIndex <= 960))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 970) && (maxIndex < 1010))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "B5", -1, 50, 30, true);
        if ((maxIndex >= 970) && (maxIndex <= 980))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 980) && (maxIndex <= 995))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 995) && (maxIndex <= 1010))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 1030) && (maxIndex < 1070))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "C6", -1, 50, 30, true);
        if ((maxIndex >= 1030) && (maxIndex <= 1040))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 1040) && (maxIndex <= 1055))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 1055) && (maxIndex <= 1070))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 1090) && (maxIndex < 1130))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "C#6", -1, 50, 30, true);
        if ((maxIndex >= 1090) && (maxIndex <= 1100))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 1100) && (maxIndex <= 1120))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 1120) && (maxIndex <= 1130))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 1150) && (maxIndex < 1195))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "D6", -1, 50, 30, true);
        if ((maxIndex >= 1150) && (maxIndex <= 1165))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 1165) && (maxIndex <= 1180))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 1180) && (maxIndex <= 1195))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 1220) && (maxIndex < 1265))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "D#6", -1, 50, 30, true);
        if ((maxIndex >= 1220) && (maxIndex <= 1235))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 1235) && (maxIndex <= 1251))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 1251) && (maxIndex <= 1265))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 1300) && (maxIndex < 1335))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "E6", -1, 50, 30, true);
        if ((maxIndex >= 1300) && (maxIndex <= 1310))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 1310) && (maxIndex <= 1325))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 1325) && (maxIndex <= 1335))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 1375) && (maxIndex < 1420))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "F6", -1, 50, 30, true);
        if ((maxIndex >= 1375) && (maxIndex <= 1390))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 1390) && (maxIndex <= 1410))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 1410) && (maxIndex <= 1420))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 1465) && (maxIndex < 1500))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "F#6", -1, 50, 30, true);
        if ((maxIndex >= 1465) && (maxIndex <= 1470))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 1470) && (maxIndex <= 1490))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 1490) && (maxIndex <= 1500))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 1547) && (maxIndex < 1587))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "G6", -1, 50, 30, true);
        if ((maxIndex >= 1547) && (maxIndex <= 1557))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 1557) && (maxIndex <= 1577))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 1577) && (maxIndex <= 1587))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 1645) && (maxIndex < 1678))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "G#6", -1, 50, 30, true);
        if ((maxIndex >= 1645) && (maxIndex <= 1655))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 1655) && (maxIndex <= 1667))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 1667) && (maxIndex <= 1678))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 1740) && (maxIndex < 1785))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "A6", -1, 50, 30, true);
        if ((maxIndex >= 1740) && (maxIndex <= 1755))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 1755) && (maxIndex <= 1770))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 1770) && (maxIndex <= 1785))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 1844) && (maxIndex < 1884))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "A#6", -1, 50, 30, true);
        if ((maxIndex >= 1844) && (maxIndex <= 1854))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 1854) && (maxIndex <= 1874))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 1874) && (maxIndex <= 1884))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
    else if ((maxIndex >= 1960) && (maxIndex < 1976))
    {
        Graphics_drawString(&g_sContext, (int8_t*) "B6", -1, 50, 30, true);
        if ((maxIndex >= 1960) && (maxIndex <= 1970))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "FLAT", -1, 50, 50,
            true);
        }
        else if ((maxIndex > 1985) && (maxIndex <= 1990))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "TUNE", -1, 50, 70,
            true);
        }
        else if ((maxIndex > 1990) && (maxIndex <= 2000))
        {
            Graphics_drawString(&g_sContext, (int8_t*) "SHARP", -1, 50, 90,
            true);
        }
    }
}

void metronome_play(bool Booster1_Pressed,bool Booster2_Pressed)
{
    // The struct that holds all the info for PWM driving the buzzer
    Timer_A_PWMConfig pwmConfig_h;
    static int BPM = 100;

    song_note_t note = {note_c2, (60000)/(8*BPM)};

    InitSound();
    ConfigurePWM(&pwmConfig_h, note.note_name);

    static bool checking = true;
    static bool time = true;

    double load = 1;
    load = 48000000/(8*BPM);
    load = load*60;

    double load1 = 1;
    load1 = 48000000/(BPM);
    load1 = load1*60;

    static bool prev = false;
    static bool islight;
    lux = OPT3001_getLux();
    if(lux<=10)
    {
        islight = false;
    }
    else
    {
        islight = true;
    }

    if(islight)
    {
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        draw_black_circle_fill(g_sContext, BPM);
    }
    else
    {
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
        draw_white_circle_fill(g_sContext, BPM);
    }
    if(prev != islight)
    {
      Graphics_clearDisplay(&g_sContext);
      prev = islight;
    }
    if(checking)
    {
        startOneShotTimer0(load1);//Timer for the timer
        startOneShotTimer1(load);//Timer for the beep
        Timer_A_generatePWM(TIMER_A0_BASE, &pwmConfig_h);
        if(islight)
        {
            draw_black_circles(g_sContext);
        }
        else
        {
            draw_white_circles(g_sContext);
        }
        checking = false;
    }

    if(timer1Expired() && time)
    {
        time = false;
        Timer_A_stopTimer(TIMER_A0_BASE);
        if(islight)
        {
            draw_white_circles(g_sContext);
        }
        else
        {
            draw_black_circles(g_sContext);
        }
    }

    if((timer0Expired()) && (!time))
    {
        time = true;
        checking = true;
    }

    if((Booster1_Pressed) && (BPM<200))
    {
        BPM = BPM + 10;
    }
    else if((Booster2_Pressed) && (BPM>10))
    {
        BPM = BPM - 10;
    }
}

void FFT_play(bool islight)
{
    static bool prev = true;
    if(islight)
    {
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    }
    else
    {
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    }

    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    if(prev != islight)
    {
        Graphics_clearDisplay(&g_sContext);
    }
    Graphics_drawLineH(&g_sContext, 0, 127, 115);
    Graphics_drawLineV(&g_sContext, 0, 115, 117);
    Graphics_drawLineV(&g_sContext, 16, 115, 116);
    Graphics_drawLineV(&g_sContext, 31, 115, 117);
    Graphics_drawLineV(&g_sContext, 32, 115, 117);
    Graphics_drawLineV(&g_sContext, 48, 115, 116);
    Graphics_drawLineV(&g_sContext, 63, 115, 117);
    Graphics_drawLineV(&g_sContext, 64, 115, 117);
    Graphics_drawLineV(&g_sContext, 80, 115, 116);
    Graphics_drawLineV(&g_sContext, 95, 115, 117);
    Graphics_drawLineV(&g_sContext, 96, 115, 117);
    Graphics_drawLineV(&g_sContext, 112, 115, 116);
    Graphics_drawLineV(&g_sContext, 127, 115, 117);

    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"2048-Point FFT",
                                AUTO_STRING_LENGTH,
                                64,
                                6,
                                OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"0",
                                AUTO_STRING_LENGTH,
                                4,
                                122,
                                OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"1",
                                AUTO_STRING_LENGTH,
                                32,
                                122,
                                OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"2",
                                AUTO_STRING_LENGTH,
                                64,
                                122,
                                OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"3",
                                AUTO_STRING_LENGTH,
                                96,
                                122,
                                OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"4",
                                AUTO_STRING_LENGTH,
                                125,
                                122,
                                OPAQUE_TEXT);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
    Graphics_drawStringCentered(&g_sContext,
                                (int8_t *)"kHz",
                                AUTO_STRING_LENGTH,
                                112,
                                122,
                                OPAQUE_TEXT);

    MAP_PCM_gotoLPM0();

    int i = 0;

    /* Computer real FFT using the completed data buffer */
    if(switch_data & 1)
    {
        for(i = 0; i < 2048; i++)
        {
            data_array1[i] = (int16_t)(hann[i] * data_array1[i]);
        }
        arm_rfft_instance_q15 instance;
        status = arm_rfft_init_q15(&instance, fftSize, ifftFlag,
                                   doBitReverse);

        arm_rfft_q15(&instance, data_array1, data_input);
    }
    else
    {
        for(i = 0; i < 2048; i++)
        {
            data_array2[i] = (int16_t)(hann[i] * data_array2[i]);
        }
        arm_rfft_instance_q15 instance;
        status = arm_rfft_init_q15(&instance, fftSize, ifftFlag,
                                   doBitReverse);

        arm_rfft_q15(&instance, data_array2, data_input);
    }

    /* Calculate magnitude of FFT complex output */
    for(i = 0; i < 4096; i += 2)
    {
        data_output[i /
                    2] =
            (int32_t)(sqrtf((data_input[i] *
                             data_input[i]) +
                            (data_input[i + 1] * data_input[i + 1])));
    }

    q15_t maxValue;
    uint32_t maxIndex = 0;

    //Upto this point the FFT is done!!
    //data output is keeping the FFT result
    //FFT is done it finds the max value in the array and it returns the maxIndex
    //The below function return what index has the maxIndex
    arm_max_q15(data_output, fftSize, &maxValue, &maxIndex);

    if(maxIndex <= 64)
    {
        color = ((uint32_t)(0xFF * (maxIndex / 64.0f)) << 8) + 0xFF;
    }
    else if(maxIndex <= 128)
    {
        color =
            (0xFF - (uint32_t)(0xFF * ((maxIndex - 64) / 64.0f))) + 0xFF00;
    }
    else if(maxIndex <= 192)
    {
        color =
            ((uint32_t)(0xFF * ((maxIndex - 128) / 64.0f)) << 16) + 0xFF00;
    }
    else
    {
        color =
            ((0xFF -
              (uint32_t)(0xFF *
                         ((maxIndex - 192) / 64.0f))) << 8) + 0xFF0000;
    }

    /* Draw frequency bin graph */
    for(i = 0; i < 2048; i += 2)
    {
        //if the energy is too high just use 100 for the display
        //the raw data is what I care and the below line is the raw data. The 8 is for display for purpose. For
        //note detection you loose three bits so you can take them off.
        /*
         * data_output[i] + data_output[i + 1] creates the value for the FFT
         */
        int x = min(100, (int)((data_output[i] + data_output[i + 1]) / 2));

        if(islight)
        {
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        }
        else
        {
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
        }
        Graphics_drawLineV(&g_sContext, i / 4, 114 - x, 14);
        Graphics_setForegroundColor(&g_sContext, color);
        Graphics_drawLineV(&g_sContext, i / 4, 114, 114 - x);
    }
    prev = islight;
}

/* Completion interrupt for ADC14 MEM0 */
void DMA_INT1_IRQHandler(void)
{
    /* Switch between primary and alternate bufferes with DMA's PingPong mode */
    if(DMA_getChannelAttribute(7) & UDMA_ATTR_ALTSELECT)
    {
        DMA_setChannelControl(
            UDMA_PRI_SELECT | DMA_CH7_ADC14,
            UDMA_SIZE_16 | UDMA_SRC_INC_NONE |
            UDMA_DST_INC_16 | UDMA_ARB_1);
        DMA_setChannelTransfer(UDMA_PRI_SELECT | DMA_CH7_ADC14,
                               UDMA_MODE_PINGPONG, (void*) &ADC14->MEM[0],
                               data_array1, SAMPLE_LENGTH);
        switch_data = 1;
    }
    else
    {
        DMA_setChannelControl(
            UDMA_ALT_SELECT | DMA_CH7_ADC14,
            UDMA_SIZE_16 | UDMA_SRC_INC_NONE |
            UDMA_DST_INC_16 | UDMA_ARB_1);
        DMA_setChannelTransfer(UDMA_ALT_SELECT | DMA_CH7_ADC14,
                               UDMA_MODE_PINGPONG, (void*) &ADC14->MEM[0],
                               data_array2, SAMPLE_LENGTH);
        switch_data = 0;
    }
}

void initial_note()
{
    Timer_A_PWMConfig pwmConfig =
    {
        TIMER_A_CLOCKSOURCE_SMCLK,
        TIMER_A_CLOCKSOURCE_DIVIDER_1,
        (SMCLK_FREQUENCY / SAMPLE_FREQUENCY),
        TIMER_A_CAPTURECOMPARE_REGISTER_1,
        TIMER_A_OUTPUTMODE_SET_RESET,
        (SMCLK_FREQUENCY / SAMPLE_FREQUENCY) / 2
    };

    MAP_WDT_A_holdTimer();
        MAP_Interrupt_disableMaster();

        /* Set the core voltage level to VCORE1 */
        MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);

        /* Set 2 flash wait states for Flash bank 0 and 1*/
        MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
        MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);

        /* Initializes Clock System */
        MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
        MAP_CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
        MAP_CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
        MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
        MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

        int n;
        for(n = 0; n < SAMPLE_LENGTH; n++)
        {
            hann[n] = 0.5f - 0.5f * cosf((2 * PI * n) / (SAMPLE_LENGTH - 1));
        }

        /* Configuring Timer_A to have a period of approximately 500ms and
         * an initial duty cycle of 10% of that (3200 ticks)  */
        Timer_A_generatePWM(TIMER_A1_BASE, &pwmConfig);

        /* Initializing ADC (MCLK/1/1) */
        ADC14_enableModule();
        ADC14_initModule(ADC_CLOCKSOURCE_MCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_1, 0);

        ADC14_setSampleHoldTrigger(ADC_TRIGGER_SOURCE3, false);

        /* Configuring GPIOs (4.3 A10) */
        GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN3,
                                                   GPIO_TERTIARY_MODULE_FUNCTION);

        /* Configuring ADC Memory */
        ADC14_configureSingleSampleMode(ADC_MEM0, true);
        ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                        ADC_INPUT_A10, false);

        /* Set ADC result format to signed binary */
        ADC14_setResultFormat(ADC_SIGNED_BINARY);

        /* Configuring DMA module */
        DMA_enableModule();
        DMA_setControlBase(MSP_EXP432P401RLP_DMAControlTable);

        DMA_disableChannelAttribute(DMA_CH7_ADC14,
                                    UDMA_ATTR_ALTSELECT | UDMA_ATTR_USEBURST |
                                    UDMA_ATTR_HIGH_PRIORITY |
                                    UDMA_ATTR_REQMASK);

        /* Setting Control Indexes. In this case we will set the source of the
         * DMA transfer to ADC14 Memory 0
         *  and the destination to the
         * destination data array. */
        MAP_DMA_setChannelControl(
            UDMA_PRI_SELECT | DMA_CH7_ADC14,
            UDMA_SIZE_16 | UDMA_SRC_INC_NONE |
            UDMA_DST_INC_16 | UDMA_ARB_1);
        MAP_DMA_setChannelTransfer(UDMA_PRI_SELECT | DMA_CH7_ADC14,
                                   UDMA_MODE_PINGPONG, (void*) &ADC14->MEM[0],
                                   data_array1, SAMPLE_LENGTH);

        MAP_DMA_setChannelControl(
            UDMA_ALT_SELECT | DMA_CH7_ADC14,
            UDMA_SIZE_16 | UDMA_SRC_INC_NONE |
            UDMA_DST_INC_16 | UDMA_ARB_1);
        MAP_DMA_setChannelTransfer(UDMA_ALT_SELECT | DMA_CH7_ADC14,
                                   UDMA_MODE_PINGPONG, (void*) &ADC14->MEM[0],
                                   data_array2, SAMPLE_LENGTH);

        /* Assigning/Enabling Interrupts */
        MAP_DMA_assignInterrupt(DMA_INT1, 7);
        MAP_Interrupt_enableInterrupt(INT_DMA_INT1);
        MAP_DMA_assignChannel(DMA_CH7_ADC14);
        MAP_DMA_clearInterruptFlag(7);
        MAP_Interrupt_enableMaster();

        /* Now that the DMA is primed and setup, enabling the channels. The ADC14
         * hardware should take over and transfer/receive all bytes */
        MAP_DMA_enableChannel(7);
        MAP_ADC14_enableConversion();
}

void initial(bool issensor)
{
    Timer_A_PWMConfig pwmConfig =
    {
        TIMER_A_CLOCKSOURCE_SMCLK,
        TIMER_A_CLOCKSOURCE_DIVIDER_1,
        (SMCLK_FREQUENCY / SAMPLE_FREQUENCY),
        TIMER_A_CAPTURECOMPARE_REGISTER_1,
        TIMER_A_OUTPUTMODE_SET_RESET,
        (SMCLK_FREQUENCY / SAMPLE_FREQUENCY) / 2
    };

    MAP_WDT_A_holdTimer();
        MAP_Interrupt_disableMaster();

        /* Set the core voltage level to VCORE1 */
        MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);

        /* Set 2 flash wait states for Flash bank 0 and 1*/
        MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
        MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);

        /* Initializes Clock System */
        MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
        MAP_CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
        MAP_CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
        MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
        MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

        /* Initializes display */
        Crystalfontz128x128_Init();

        /* Set default screen orientation */
        Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

        /* Initializes graphics context */
        Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128,
                             &g_sCrystalfontz128x128_funcs);

        if(issensor)
        {
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
            Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        }
        else
        {
            Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        }


        GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
        Graphics_clearDisplay(&g_sContext);
        Graphics_drawLineH(&g_sContext, 0, 127, 115);
        Graphics_drawLineV(&g_sContext, 0, 115, 117);
        Graphics_drawLineV(&g_sContext, 16, 115, 116);
        Graphics_drawLineV(&g_sContext, 31, 115, 117);
        Graphics_drawLineV(&g_sContext, 32, 115, 117);
        Graphics_drawLineV(&g_sContext, 48, 115, 116);
        Graphics_drawLineV(&g_sContext, 63, 115, 117);
        Graphics_drawLineV(&g_sContext, 64, 115, 117);
        Graphics_drawLineV(&g_sContext, 80, 115, 116);
        Graphics_drawLineV(&g_sContext, 95, 115, 117);
        Graphics_drawLineV(&g_sContext, 96, 115, 117);
        Graphics_drawLineV(&g_sContext, 112, 115, 116);
        Graphics_drawLineV(&g_sContext, 127, 115, 117);

        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"2048-Point FFT",
                                    AUTO_STRING_LENGTH,
                                    64,
                                    6,
                                    OPAQUE_TEXT);
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"0",
                                    AUTO_STRING_LENGTH,
                                    4,
                                    122,
                                    OPAQUE_TEXT);
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"1",
                                    AUTO_STRING_LENGTH,
                                    32,
                                    122,
                                    OPAQUE_TEXT);
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"2",
                                    AUTO_STRING_LENGTH,
                                    64,
                                    122,
                                    OPAQUE_TEXT);
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"3",
                                    AUTO_STRING_LENGTH,
                                    96,
                                    122,
                                    OPAQUE_TEXT);
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"4",
                                    AUTO_STRING_LENGTH,
                                    125,
                                    122,
                                    OPAQUE_TEXT);
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
        Graphics_drawStringCentered(&g_sContext,
                                    (int8_t *)"kHz",
                                    AUTO_STRING_LENGTH,
                                    112,
                                    122,
                                    OPAQUE_TEXT);

        // Initialize Hann Window
        int n;
        for(n = 0; n < SAMPLE_LENGTH; n++)
        {
            hann[n] = 0.5f - 0.5f * cosf((2 * PI * n) / (SAMPLE_LENGTH - 1));
        }

        /* Configuring Timer_A to have a period of approximately 500ms and
         * an initial duty cycle of 10% of that (3200 ticks)  */
        Timer_A_generatePWM(TIMER_A1_BASE, &pwmConfig);

        /* Initializing ADC (MCLK/1/1) */
        ADC14_enableModule();
        ADC14_initModule(ADC_CLOCKSOURCE_MCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_1, 0);

        ADC14_setSampleHoldTrigger(ADC_TRIGGER_SOURCE3, false);

        /* Configuring GPIOs (4.3 A10) */
        GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN3,
                                                   GPIO_TERTIARY_MODULE_FUNCTION);

        /* Configuring ADC Memory */
        ADC14_configureSingleSampleMode(ADC_MEM0, true);
        ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                        ADC_INPUT_A10, false);

        /* Set ADC result format to signed binary */
        ADC14_setResultFormat(ADC_SIGNED_BINARY);

        /* Configuring DMA module */
        DMA_enableModule();
        DMA_setControlBase(MSP_EXP432P401RLP_DMAControlTable);

        DMA_disableChannelAttribute(DMA_CH7_ADC14,
                                    UDMA_ATTR_ALTSELECT | UDMA_ATTR_USEBURST |
                                    UDMA_ATTR_HIGH_PRIORITY |
                                    UDMA_ATTR_REQMASK);

        /* Setting Control Indexes. In this case we will set the source of the
         * DMA transfer to ADC14 Memory 0
         *  and the destination to the
         * destination data array. */
        MAP_DMA_setChannelControl(
            UDMA_PRI_SELECT | DMA_CH7_ADC14,
            UDMA_SIZE_16 | UDMA_SRC_INC_NONE |
            UDMA_DST_INC_16 | UDMA_ARB_1);
        MAP_DMA_setChannelTransfer(UDMA_PRI_SELECT | DMA_CH7_ADC14,
                                   UDMA_MODE_PINGPONG, (void*) &ADC14->MEM[0],
                                   data_array1, SAMPLE_LENGTH);

        MAP_DMA_setChannelControl(
            UDMA_ALT_SELECT | DMA_CH7_ADC14,
            UDMA_SIZE_16 | UDMA_SRC_INC_NONE |
            UDMA_DST_INC_16 | UDMA_ARB_1);
        MAP_DMA_setChannelTransfer(UDMA_ALT_SELECT | DMA_CH7_ADC14,
                                   UDMA_MODE_PINGPONG, (void*) &ADC14->MEM[0],
                                   data_array2, SAMPLE_LENGTH);

        /* Assigning/Enabling Interrupts */
        MAP_DMA_assignInterrupt(DMA_INT1, 7);
        MAP_Interrupt_enableInterrupt(INT_DMA_INT1);
        MAP_DMA_assignChannel(DMA_CH7_ADC14);
        MAP_DMA_clearInterruptFlag(7);
        MAP_Interrupt_enableMaster();

        /* Now that the DMA is primed and setup, enabling the channels. The ADC14
         * hardware should take over and transfer/receive all bytes */
        MAP_DMA_enableChannel(7);
        MAP_ADC14_enableConversion();
}

void light_initial()
{
    MAP_WDT_A_holdTimer();
    MAP_Interrupt_disableMaster();

    /* Set the core voltage level to VCORE1 */
    MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);

    /* Set 2 flash wait states for Flash bank 0 and 1*/
    MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
    MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);

    /* Initializes Clock System */
    MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
    MAP_CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    /* Configuring Timer_A0 for Up Mode and starting */
    /*MAP_Timer_A_configureUpMode(TIMER_A2_BASE, &upConfig);
    MAP_Timer_A_startCounter(TIMER_A2_BASE, TIMER_A_UP_MODE);

    /* Initialize compare registers to generate PWM */
    //MAP_Timer_A_initCompare(TIMER_A2_BASE, &compareConfig_PWM);

    /* Initialize I2C communication */
    Init_I2C_GPIO();
    I2C_init();

    /* Initialize OPT3001 digital ambient light sensor */
    OPT3001_init();

    __delay_cycles(100000);
}
