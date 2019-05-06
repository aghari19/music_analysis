/*
 * graphics.c
 *
 *  Created on: May 2, 2019
 *      Author: abarh
 */

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"

#include "graphics.h"

void draw_white_rec(Graphics_Context g_sContext)
{
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_Rectangle Rec = { 0, 0, 128, 128 };
    Graphics_fillRectangle(&g_sContext, &Rec);
}

void draw_black_circle_fill(Graphics_Context g_sContext, int BPM)
{
    char string1[3];

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_drawString(&g_sContext,(int8_t*) "BPM:", -1, 40, 10, true);
    make_3digit_NumString(BPM, string1);
    Graphics_drawString(&g_sContext,(int8_t*)string1, -1, 75, 10, true);
    Graphics_fillCircle(&g_sContext, 64, 64, 10);

}

void make_3digit_NumString(unsigned int num, char *string)
{
    string[0]= (num/100)+'0';
    string[1]= ((num%100) / 10) + '0';
    string[2]= (num%10)+'0';
    string[3]= 0;
}

void draw_black_circles(Graphics_Context g_sContext)
{
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_drawCircle(&g_sContext, 64, 64,13);
    Graphics_drawCircle(&g_sContext, 64, 64,15);
}

void draw_white_circles(Graphics_Context g_sContext)
{
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawCircle(&g_sContext, 64, 64,13);
    Graphics_drawCircle(&g_sContext, 64, 64,15);
}

void note_detection_header_black( Graphics_Context g_sContext)
{
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_drawString(&g_sContext,(int8_t*) "NOTE DETECTION", -1, 25, 10, true);
}

void display_note_detection_black(Graphics_Context g_sContext, int maxIndex)
{
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_Rectangle Rec = { 20, 30, 128, 50 };
    Graphics_fillRectangle(&g_sContext, &Rec);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);


    if((maxIndex > 10) && (maxIndex <= 29))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "A0", -1, 20, 30, true);
    }
    else if((maxIndex >= 29) && (maxIndex < 31))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "B0", -1, 20, 30, true);
    }
    else if((maxIndex == 31) && (maxIndex < 33))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "C1", -1, 20, 30, true);
    }
    else if((maxIndex >= 33) && (maxIndex < 39))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "D1", -1, 20, 30, true);
    }
    else if((maxIndex >= 39) && (maxIndex < 42))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "E1", -1, 20, 30, true);
    }
    else if((maxIndex >= 42) && (maxIndex < 45))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "F1", -1, 20, 30, true);
    }
    else if((maxIndex >= 45) && (maxIndex < 50))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "G1", -1, 20, 30, true);
    }
    else if((maxIndex >= 50) && (maxIndex < 57))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "A1", -1, 20, 30, true);
    }
    else if((maxIndex >= 57) && (maxIndex < 63))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "B1", -1, 20, 30, true);
    }
    else if((maxIndex >= 63) && (maxIndex < 68))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "C2", -1, 20, 30, true);
    }
    else if((maxIndex >= 68) && (maxIndex < 76))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "D2", -1, 20, 30, true);
    }
    else if((maxIndex >= 76) && (maxIndex < 85))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "E2", -1, 20, 30, true);
    }
    else if((maxIndex >= 85) && (maxIndex < 89))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "F2", -1, 20, 30, true);
    }
    else if((maxIndex >= 89) && (maxIndex < 100))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "G2", -1, 20, 30, true);
    }
    else if((maxIndex >= 100) && (maxIndex < 114))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "A2", -1, 20, 30, true);
    }
    else if((maxIndex >= 114) && (maxIndex < 126))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "B2", -1, 20, 30, true);
    }
    else if((maxIndex >= 126) && (maxIndex < 135))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "C3", -1, 20, 30, true);
    }
    else if((maxIndex >= 135) && (maxIndex < 150))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "D3", -1, 20, 30, true);
    }
    else if((maxIndex >= 150) && (maxIndex < 170))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "E3", -1, 20, 30, true);
    }
    else if((maxIndex >= 170) && (maxIndex < 180))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "F3", -1, 20, 30, true);
    }
    else if((maxIndex >= 180) && (maxIndex < 200))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "G3", -1, 20, 30, true);
    }
    else if((maxIndex >= 200) && (maxIndex < 225))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "A3", -1, 20, 30, true);
    }
    else if((maxIndex >= 225) && (maxIndex < 250))
    {
        Graphics_drawString(&g_sContext,(int8_t*) "B3", -1, 20, 30, true);
    }
     if((maxIndex>259) && (maxIndex<276))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "C4", -1, 20, 30, true);
    }
    else if((maxIndex>=276)&&(maxIndex<279))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "C#4", -1, 20, 30, true);
    }
    else if((maxIndex>=279)&&(maxIndex<295))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "D4", -1, 20, 30, true);
    }
    else if((maxIndex>=295)&&(maxIndex<314))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "D#4", -1, 20, 30, true);
    }
    else if((maxIndex>=314)&&(maxIndex<331))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "E4", -1, 20, 30, true);
    }
    else if((maxIndex>=331)&&(maxIndex<351))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "F4", -1, 20, 30, true);
    }
    else if((maxIndex>=351)&&(maxIndex<371))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "F#4", -1, 20, 30, true);
    }
    else if((maxIndex>=371)&&(maxIndex<393))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "G4", -1, 20, 30, true);
    }
    else if((maxIndex>=393)&&(maxIndex<417))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "G#4", -1, 20, 30, true);
    }
    else if((maxIndex>=417)&&(maxIndex<452))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "A4", -1, 20, 30, true);
    }
    else if((maxIndex>=452)&&(maxIndex<468))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "A#4", -1, 20, 30, true);
    }
    else if((maxIndex>=468)&&(maxIndex<495))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "B4", -1, 20, 30, true);
    }
    else if((maxIndex>=495)&&(maxIndex<525))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "C5", -1, 20, 30, true);
    }
    else if((maxIndex>=525)&&(maxIndex<556))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "C#5", -1, 20, 30, true);
    }
    else if((maxIndex>=556)&&(maxIndex<589))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "D5", -1, 20, 30, true);
    }
    else if((maxIndex>=589)&&(maxIndex<624))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "D#5", -1, 20, 30, true);
    }
    else if((maxIndex>=624)&&(maxIndex<661))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "E5", -1, 20, 30, true);
    }
    else if((maxIndex>=661)&&(maxIndex<700))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "F5", -1, 20, 30, true);
    }
    else if((maxIndex>=700)&&(maxIndex<741))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "F#5", -1, 20, 30, true);
    }
    else if((maxIndex>=741)&&(maxIndex<785))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "G5", -1, 20, 30, true);
    }
    else if((maxIndex>=785)&&(maxIndex<832))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "G#5", -1, 20, 30, true);
    }
    else if((maxIndex>=832)&&(maxIndex<880))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "A5", -1, 20, 30, true);
    }
    else if((maxIndex>=880)&&(maxIndex<934))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "A#5", -1, 20, 30, true);
    }
    else if((maxIndex>=934)&&(maxIndex<989))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "B5", -1, 20, 30, true);
    }
    else if((maxIndex>=989)&&(maxIndex<1048))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "C6", -1, 20, 30, true);
    }
    else if((maxIndex>=1048)&&(maxIndex<1110))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "C#6", -1, 20, 30, true);
    }
    else if((maxIndex>=1110)&&(maxIndex<1176))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "D6", -1, 20, 30, true);
    }
    else if((maxIndex>=1176)&&(maxIndex<1246))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "D#6", -1, 20, 30, true);
    }
    else if((maxIndex>=1246)&&(maxIndex<1320))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "E6", -1, 20, 30, true);
    }
    else if((maxIndex>=1320)&&(maxIndex<1398))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "F6", -1, 20, 30, true);
    }
    else if((maxIndex>=1398)&&(maxIndex<1481))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "F#6", -1, 20, 30, true);
    }
    else if((maxIndex>=1481)&&(maxIndex<1569))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "G6", -1, 20, 30, true);
    }
    else if((maxIndex>=1569)&&(maxIndex<1663))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "G#6", -1, 20, 30, true);
    }
    else if((maxIndex>=1663)&&(maxIndex<1762))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "A6", -1, 20, 30, true);
    }
    else if((maxIndex>=1762)&&(maxIndex<1866))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "A#6", -1, 20, 30, true);
    }
    else if((maxIndex>=1866)&&(maxIndex<1976))
    {
      Graphics_drawString(&g_sContext,(int8_t*) "B6", -1, 20, 30, true);
    }
}

void FFT_display_black(Graphics_Context g_sContext)
{
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);


    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
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

}

void draw_black_rec(Graphics_Context g_sContext)
{
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_Rectangle Rec = { 0, 0, 128, 128 };
    Graphics_fillRectangle(&g_sContext, &Rec);
}
void draw_white_circle_fill(Graphics_Context g_sContext, int BPM)
{
    char string1[3];

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_fillCircle(&g_sContext, 64, 64, 10);
    Graphics_drawString(&g_sContext, (int8_t*) "BPM:", -1, 40, 10, true);
    make_3digit_NumString(BPM, string1);
    Graphics_drawString(&g_sContext, (int8_t*) string1, -1, 75, 10, true);
}

void note_detection_header_white(Graphics_Context g_sContext)
{
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_drawString(&g_sContext,(int8_t*) "NOTE DETECTION", -1, 25, 10, true);
}
void FFT_display_white(Graphics_Context g_sContext)
{
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);


        GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
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
}

