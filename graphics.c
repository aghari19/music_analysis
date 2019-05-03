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
    Graphics_fillCircle(&g_sContext, 64, 64, 10);
    Graphics_drawString(&g_sContext,(int8_t*) "BPM:", -1, 40, 10, true);
    make_3digit_NumString(BPM, string1);
    Graphics_drawString(&g_sContext,(int8_t*)string1, -1, 75, 10, true);

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

