/*
 * graphics.h
 *
 *  Created on: May 2, 2019
 *      Author: abarh
 */

#ifndef GRAPHICS_H_
#define GRAPHICS_H_

    //This method draws a white rectangle when the state of the FSM changes
    void draw_white_rec(Graphics_Context g_sContext);

    //This method draws a black circle for the metronome
    void draw_black_circle_fill(Graphics_Context g_sContext, int BPM);

    //This methos is the graphics for the metronome
    void draw_black_circles(Graphics_Context g_sContext);

    //This method is the graohics for masking the graphics of the metronome
    void draw_white_circles(Graphics_Context g_sContext);

    //This method is for drawing the header for note detection in black
    void note_detection_header_black( Graphics_Context g_sContext);

    //This method is for displaying the graphics of the FFT when the light sensor is not convered
    void FFT_display_black(Graphics_Context g_sContext);

    //This method is for displaying the graphics of the note detection when the light sensor is not covered
    void display_note_detection_black(Graphics_Context g_sContext, int maxIndex);

    //This method draws black rectangle when in dark mode and the user shifts from one state to the other state of the FSM
    void draw_black_rec(Graphics_Context g_sContext);

    //This is the metronome graphics when the dark mode on
    void draw_white_circle_fill(Graphics_Context g_sContext, int BPM);

    //This method display the note detection header in white when the light sensor is on
    void note_detection_header_white( Graphics_Context g_sContext);

    //This method is for displaying the graphics of the FFT when in dark mode
    void FFT_display_white(Graphics_Context g_sContext);

    //This method converts a digit number to a string
    void make_3digit_NumString(unsigned int num, char *string);

#endif /* GRAPHICS_H_ */
