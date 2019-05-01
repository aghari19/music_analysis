/*
 * Timer.h
 *
 *  Created on: Apr 29, 2019
 *      Author: abarh
 */

#ifndef TIMER_H_
#define TIMER_H_

typedef struct
{
    uint32_t timerID;
    uint32_t sourceClockF_inMHz;
    uint32_t prescalar;
    uint32_t loadValue;
    uint32_t rolloverCount;
    uint32_t interruptNumber;
} HWTimer_t;

typedef struct
{
    HWTimer_t   *hwtimer_p;             // hardware timer used as basis for this software timer
    uint32_t    waitCycles;             // wait "cycles" for the software timer
    uint32_t    startCounter;           // last counter value when the SW timer started (C1)
    uint32_t    startRolloverCount;     // The number of hardware counter rollovers at the start of the this software timer
} OneShotSWTimer_t;

void startOneShotTimer0(unsigned int LoadVal);
bool timer0Expired();

void startOneShotTimer1(unsigned int LoadVal);
bool timer1Expired();

void InitOneShotSWTimer(OneShotSWTimer_t* OST_p, HWTimer_t* hwtimer_p, uint32_t  waitTime);
void StartOneShotSWTimer(OneShotSWTimer_t* OST_p);
bool OneShotSWTimerExpired(OneShotSWTimer_t* OST_p);
void initHWTimer0();
void initHWTimer1();

#endif /* TIMER_H_ */
