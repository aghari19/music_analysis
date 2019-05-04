#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include "ButtonLED_HAL.h"
#include "Timer.h"


void menu();
void initialization();
//void initialLUX();

int main(void)

{
    initialization();

    while(1)
    {

        menu();
    }
}

void initialization()
{

    Timer32_initModule(TIMER32_0_BASE, // There are two timers, we are using the one with the index 1
                       TIMER32_PRESCALER_1, // The prescaler value is 1; The clock is not divided before feeding the counter
                       TIMER32_32BIT, // The counter is used in 32-bit mode; the alternative is 16-bit mode
                       TIMER32_PERIODIC_MODE); //This options is irrelevant for a one-shot timer

    Timer32_setCount(TIMER32_0_BASE, 1);
    Timer32_startTimer(TIMER32_0_BASE, true);


    Timer32_initModule(TIMER32_1_BASE, // There are two timers, we are using the one with the index 1
                       TIMER32_PRESCALER_1, // The prescaler value is 1; The clock is not divided before feeding the counter
                       TIMER32_32BIT, // The counter is used in 32-bit mode; the alternative is 16-bit mode
                       TIMER32_PERIODIC_MODE); //This options is irrelevant for a one-shot timer

    Timer32_setCount(TIMER32_1_BASE, 1);
    Timer32_startTimer(TIMER32_1_BASE, true);

    initialize_LaunchpadLeftButton();
    initialize_LaunchpadRightButton();

    initialize_BoosterpackTopButton();
    initialize_BoosterpackBottomButton();

    initialize_LaunchpadLED1();
    initialize_LaunchpadLED2_red();
    initialize_LaunchpadLED2_green();
    initialize_LaunchpadLED2_blue();
    initialize_BoosterpackLED_red();
    initialize_BoosterpackLED_green();
    initialize_BoosterpackLED_blue();

    turnOff_LaunchpadLED1();
    turnOff_LaunchpadLED2_red();
    turnOff_LaunchpadLED2_green();
    turnOff_LaunchpadLED2_blue();
    turnOff_BoosterpackLED_red();
    turnOff_BoosterpackLED_green();
    turnOff_BoosterpackLED_blue();
    //initHWTimer1();
    initHWTimer0();

}
