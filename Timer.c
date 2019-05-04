/*
 * Timer.c
 *
 *  Created on: Apr 29, 2019
 *      Author: abarh
 */

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include "Timer.h"

#define SYS_CLOCK_F_MHz 48

HWTimer_t timer1, timer0;

void startOneShotTimer0(unsigned int LoadVal)
{
    Timer32_setCount(TIMER32_0_BASE, LoadVal);
    Timer32_startTimer(TIMER32_0_BASE, true);
}

bool timer0Expired()
{
    return (Timer32_getValue(TIMER32_0_BASE) == 0);
}

void startOneShotTimer1(unsigned int LoadVal)
{
    Timer32_setCount(TIMER32_1_BASE, LoadVal);
    Timer32_startTimer(TIMER32_1_BASE, true);
}

bool timer1Expired()
{
    return (Timer32_getValue(TIMER32_1_BASE) == 0);
}

void InitOneShotSWTimer(OneShotSWTimer_t* OST_p, HWTimer_t* hwtimer_p, uint32_t  waitTime_us)
{
    OST_p->hwtimer_p = hwtimer_p;
    OST_p->waitCycles = WaitCycles(OST_p->hwtimer_p, waitTime_us);
}

void StartOneShotSWTimer(OneShotSWTimer_t* OST_p)
{
    OST_p->startCounter = Timer32_getValue((OST_p->hwtimer_p)->timerID);
    OST_p->startRolloverCount = OST_p->hwtimer_p->rolloverCount;
}

bool OneShotSWTimerExpired(OneShotSWTimer_t* OST_p)
{
    bool expired = false;

    // HW timer period
    uint64_t hwPeriodInCycle = (uint64_t) OST_p->hwtimer_p->loadValue + (uint64_t) 1;

    //This is C2 from notes, while OST->startCounter is C1
    uint64_t currentCounter = Timer32_getValue((OST_p->hwtimer_p)->timerID);
    uint64_t startCounter = OST_p->startCounter;
    uint32_t rollovers = OST_p->hwtimer_p->rolloverCount - OST_p->startRolloverCount;
    uint64_t rolloverCycles =  (uint64_t) rollovers * (uint64_t) hwPeriodInCycle;
    uint64_t ElapsedCycles = rolloverCycles  + startCounter  -  currentCounter;

    if (ElapsedCycles >= OST_p->waitCycles)
        expired = true;
    else
        expired = false;

    return expired;
}

void initHWTimer0()
{
    timer0.loadValue = 0xffffffff;
    timer0.prescalar = 1; // This is different from the flag  TIMER32_PRESCALER_1 which happens to be 0
    timer0.rolloverCount = 0;
    timer0.sourceClockF_inMHz = SYS_CLOCK_F_MHz;
    timer0.timerID = TIMER32_0_BASE;
    timer0.interruptNumber = INT_T32_INT1;
    startHWTimer(timer0);
}

void initHWTimer1()
{
    timer1.loadValue = 0xffffffff;
    timer1.prescalar = 256; // This is different from the flag  TIMER32_PRESCALER_1 which happens to be 0
    timer1.rolloverCount = 0;
    timer1.sourceClockF_inMHz = SYS_CLOCK_F_MHz;
    timer1.timerID = TIMER32_1_BASE;
    timer1.interruptNumber = INT_T32_INT2;
    startHWTimer(timer1);
}

void startHWTimer(HWTimer_t hwTimer)
{
    // Pay careful attention to how the flag is different from the prescaler value itself
    uint32_t prescalerFlag;
    switch(hwTimer.prescalar)
    {
    case 1:
        prescalerFlag = TIMER32_PRESCALER_1;
        break;
    case 16:
        prescalerFlag = TIMER32_PRESCALER_16;
        break;
    case 256:
        prescalerFlag = TIMER32_PRESCALER_256;
        break;
    }

    Timer32_initModule(hwTimer.timerID,
                       prescalerFlag,
                       TIMER32_32BIT, // The counter is used in 32-bit mode; the alternative is 16-bit mode
                       TIMER32_PERIODIC_MODE); //This options is irrelevant for a one-shot timer

   Interrupt_enableInterrupt(hwTimer.interruptNumber);

   // Set the load value and start. Since the counter is in periodic mode, it is the only time it needs starting
   Timer32_setCount(hwTimer.timerID, hwTimer.loadValue);
   Timer32_startTimer(hwTimer.timerID, false); //false indicates "This is not one-shot mode"

}

void T32_INT1_IRQHandler()
{
    timer0.rolloverCount++;
    Timer32_clearInterruptFlag(TIMER32_0_BASE);
}

uint64_t WaitCycles(HWTimer_t *hwTimer_p, uint32_t waitTime_us)
{
    uint64_t waitCycles;
    waitCycles = waitTime_us * SYS_CLOCK_F_MHz / hwTimer_p->prescalar;
    return waitCycles;
}
