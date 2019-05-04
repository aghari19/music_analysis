/*
 * menu.c
 *
 *  Created on: Apr 29, 2019
 *      Author: abarh
 */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>

#include "ButtonLED_HAL.h"
#include "Timer.h"
#include "sound.h"
#include "graphics.h"

#include <ti/devices/msp432p4xx/inc/msp.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include <arm_math.h>
#include <arm_const_structs.h>

#include "HAL_I2C.h"
#include "HAL_OPT3001.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"

#define TEST_LENGTH_SAMPLES 512
#define SAMPLE_LENGTH 512

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
#pragma DATA_ALIGN(MSP_EXP432P401RLP_DMAControlTable, 1024)
#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment=1024
#elif defined(__GNUC__)
__attribute__ ((aligned (1024)))
#elif defined(__CC_ARM)
__align(1024)
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

/*Light Sensor Variables*/
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

#define milsec 60000

typedef enum {FFT,metronome,note_detection} options;

button_t BoostS1 = {GPIO_PORT_P5, GPIO_PIN1, Stable_R, RELEASED_STATE, TIMER32_0_BASE};
button_t BoostS2 = {GPIO_PORT_P3, GPIO_PIN5, Stable_R, RELEASED_STATE, TIMER32_1_BASE};
button_t LaunchL = {GPIO_PORT_P1, GPIO_PIN1, Stable_R, RELEASED_STATE, TIMER32_1_BASE};
button_t LaunchR = {GPIO_PORT_P1, GPIO_PIN4, Stable_R, RELEASED_STATE, TIMER32_1_BASE};


void metronome_play(bool Booster1_Pressed,bool Booster2_Pressed);
void FFT_play(bool sensor);
void note_detection_play();
void tone_frequency(int maxIndex);
void configureMic();
void initial();
void initial_note();
void initialLUX();

void menu()
{
    //initialLUX();

    static bool first = true;
    static bool second = false;
    static bool light_sensor = false;
    static options option = FFT;

    bool Booster1_Pressed  = false;
    bool Booster2_Pressed = false;
    bool LaunchL_Pressed  = false;
    bool LaunchR_Pressed = false;

    Booster1_Pressed = ButtonPushed(&BoostS1);
    Booster2_Pressed = ButtonPushed(&BoostS2);
    LaunchL_Pressed = ButtonPushed(&LaunchL);
    LaunchR_Pressed = ButtonPushed(&LaunchR);

    //lux = OPT3001_getLux();
    if(12<10)
    {
        light_sensor = true;
    }
    else
    {
        light_sensor = false;
    }

    switch(option)
    {
        case FFT:
            if(first)
            {
                initial();
                first = false;
            }
            FFT_play(light_sensor);
            if(LaunchL_Pressed)
            {
                option = metronome;
                draw_white_rec(g_sContext);
            }
            else if(LaunchR_Pressed)
            {
                option = note_detection;
                draw_white_rec(g_sContext);
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
            note_detection_play();
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

void note_detection_play()
{
    note_detection_header_black(g_sContext);
    MAP_PCM_gotoLPM0();

    int i = 0;

    /* Computer real FFT using the completed data buffer */
    if(switch_data & 1)
    {
        for(i = 0; i < 512; i++)
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
        for(i = 0; i < 512; i++)
        {
            data_array2[i] = (int16_t)(hann[i] * data_array2[i]);
        }
        arm_rfft_instance_q15 instance;
        status = arm_rfft_init_q15(&instance, fftSize, ifftFlag,
                                   doBitReverse);

        arm_rfft_q15(&instance, data_array2, data_input);
    }

    /* Calculate magnitude of FFT complex output */
    for(i = 0; i < 1024; i += 2)
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
    tone_frequency(maxIndex);
}

void tone_frequency(int Index)
{
    int maxIndex = Index;

    maxIndex = (maxIndex*4000)/256;
    maxIndex = maxIndex +(4000/(256*2));
    display_note_detection_black(g_sContext, maxIndex);
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

    draw_black_circle_fill(g_sContext, BPM);

    if(checking)
    {
        startOneShotTimer0(load1);//Timer for the timer
        startOneShotTimer1(load);//Timer for the beep
        Timer_A_generatePWM(TIMER_A0_BASE, &pwmConfig_h);
        draw_black_circles(g_sContext);
        checking = false;
    }

    if(timer1Expired() && time)
    {
        time = false;
        Timer_A_stopTimer(TIMER_A0_BASE);
        draw_white_circles(g_sContext);
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

void FFT_play(bool sensor)
{
    if(sensor)
    {
        FFT_display_white(g_sContext);
    }
    else
    {
        FFT_display_black(g_sContext);
    }
    MAP_PCM_gotoLPM0();

    int i = 0;

    /* Computer real FFT using the completed data buffer */
    if(switch_data & 1)
    {
        for(i = 0; i < 512; i++)
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
        for(i = 0; i < 512; i++)
        {
            data_array2[i] = (int16_t)(hann[i] * data_array2[i]);
        }
        arm_rfft_instance_q15 instance;
        status = arm_rfft_init_q15(&instance, fftSize, ifftFlag,
                                   doBitReverse);

        arm_rfft_q15(&instance, data_array2, data_input);
    }

    /* Calculate magnitude of FFT complex output */
    for(i = 0; i < 1024; i += 2)
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
    for(i = 0; i < 256; i += 2)
    {
        //if the energy is too high just use 100 for the display
        //the raw data is what I care and the below line is the raw data. The 8 is for display for purpose. For
        //note detection you loose three bits so you can take them off.
        /*
         * data_output[i] + data_output[i + 1] creates the value for the FFT
         */
        int x = min(100, (int)((data_output[i] + data_output[i + 1]) / 8));

        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_drawLineV(&g_sContext, i / 2, 114 - x, 14);
        Graphics_setForegroundColor(&g_sContext, color);
        Graphics_drawLineV(&g_sContext, i / 2, 114, 114 - x);
    }

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

void initial()
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

        /* Draw Title, x-axis, gradation & labels */
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
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
                                    (int8_t *)"512-Point FFT",
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

void initialLUX()
{
    MAP_WDT_A_holdTimer();
    MAP_Interrupt_disableMaster();

    MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);
    MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
    MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);

    MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
    MAP_CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    Crystalfontz128x128_Init();

    Crystalfontz128x128_SetOrientation(0);

    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);

    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN6,
            GPIO_PRIMARY_MODULE_FUNCTION);

    MAP_Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig);
    MAP_Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

    MAP_Timer_A_initCompare(TIMER_A0_BASE, &compareConfig_PWM);

    Init_I2C_GPIO();
    I2C_init();

    OPT3001_init();

    __delay_cycles(100000);
}

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
