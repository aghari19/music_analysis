/*
 * graphics.h
 *
 *  Created on: May 2, 2019
 *      Author: abarh
 */

#ifndef GRAPHICS_H_
#define GRAPHICS_H_

    void draw_white_rec(Graphics_Context g_sContext);
    void draw_black_circle_fill(Graphics_Context g_sContext, int BPM);
    void draw_black_circles(Graphics_Context g_sContext);
    void draw_white_circles(Graphics_Context g_sContext);
    void note_detection_header_black( Graphics_Context g_sContext);
    void FFT_display_black(Graphics_Context g_sContext);
    void display_note_detection_black(Graphics_Context g_sContext, int maxIndex);

    void draw_black_rec(Graphics_Context g_sContext);
    void draw_white_circle_fill(Graphics_Context g_sContext, int BPM);
    void note_detection_header_white( Graphics_Context g_sContext);
    void FFT_display_white(Graphics_Context g_sContext);


    void make_3digit_NumString(unsigned int num, char *string);

#endif /* GRAPHICS_H_ */
