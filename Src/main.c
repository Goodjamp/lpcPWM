#include "stdint.h"

#include "cmsis.h"

#include "chip.h"
#include "clock_11u6x.h"
#include "timer_11u6x.h"
#include "clock_11u6x.h"
#include "gpio_11u6x.h"
#include "iocon_11u6x.h"
#include "wwdt_11u6x.h"

#include "ledPinDesc.h"

const uint32_t OscRateIn = 12000000;
const uint32_t RTCOscRateIn = 32768;

volatile const uint16_t pwmPeriod  = 1000;

LPC_TIMER_T *sysTimeTimer = LPC_TIMER32_0;
#define PWM_MATCH_CH        0
#define PERIOD_MATCH_CH     1
#define OUT_PWM_CH          2


void sysTimeInit(void){
    uint32_t timerClockRate = Chip_Clock_GetSystemClockRate();
    // sys timer clock rate should be 1 kHz
    Chip_TIMER_Init(sysTimeTimer);
    Chip_TIMER_PrescaleSet(sysTimeTimer,  timerClockRate/1000000);
    Chip_TIMER_Enable(sysTimeTimer);
}



void pwmConfig(void)
{
    Chip_TIMER_SetMatch(sysTimeTimer, PERIOD_MATCH_CH, pwmPeriod);
    Chip_TIMER_ResetOnMatchEnable(sysTimeTimer, PERIOD_MATCH_CH);
    Chip_TIMER_MatchEnableInt(sysTimeTimer, PERIOD_MATCH_CH);

    Chip_TIMER_SetMatch(sysTimeTimer, PWM_MATCH_CH, 0);
    Chip_TIMER_MatchEnableInt(sysTimeTimer, PWM_MATCH_CH);

    NVIC_EnableIRQ(TIMER_32_0_IRQn);
}



void TIMER32_0_IRQHandler(void)
{
    volatile static uint16_t kPWM = 0;
    volatile static bool trig = true;

    if(Chip_TIMER_MatchPending(sysTimeTimer, PWM_MATCH_CH))
    {
        Chip_TIMER_ClearMatch(sysTimeTimer, PWM_MATCH_CH);

        Chip_GPIO_SetPinOutHigh(LPC_GPIO, GPIO_YELLOW_PORT, GPIO_YELLOW_PIN);

        Chip_TIMER_SetMatch(sysTimeTimer, PWM_MATCH_CH, (trig) ? (kPWM++) : (kPWM--) );
        if(kPWM == pwmPeriod)
        {
            trig = false;
        }
        if(kPWM == 0)
        {
            trig = true;
        }
    }

    //Chip_TIMER_SetMatch(sysTimeTimer, 0, Chip_TIMER_ReadCount(sysTimeTimer) + 10);
    if(Chip_TIMER_MatchPending(sysTimeTimer, PERIOD_MATCH_CH))
    {
        Chip_TIMER_ClearMatch(sysTimeTimer, PERIOD_MATCH_CH);

        Chip_GPIO_SetPinOutLow(LPC_GPIO, GPIO_YELLOW_PORT, GPIO_YELLOW_PIN);
    }
}


uint32_t getTime(void){
    return Chip_TIMER_ReadCount(sysTimeTimer);
}


void initGpio(void)
{
    //Chip_Clock_EnablePeriphClock
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_GPIO);
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);

    uint8_t pinDesc[2][2] =
    {
       [LED_GREEN]  = {GPIO_GREEN_PORT, GPIO_GREEN_PIN},
       [LED_YELLOW] = {GPIO_YELLOW_PORT, GPIO_YELLOW_PIN},
    };
    for(uint8_t k = 0; k < 2; k++)
    {
       Chip_IOCON_PinMux(LPC_IOCON, pinDesc[k][0], pinDesc[k][1], IOCON_DIGMODE_EN | IOCON_MODE_INACT, IOCON_FUNC0);
       Chip_GPIO_SetPinDIROutput(LPC_GPIO, pinDesc[k][0], pinDesc[k][1]);
       Chip_GPIO_SetPinState(LPC_GPIO, pinDesc[k][0], pinDesc[k][1], true);
    }
}

void switchOutPin(void)
{
    static uint8_t cnt = 0;
    Chip_GPIO_SetPinState(LPC_GPIO, GPIO_GREEN_PORT, GPIO_GREEN_PIN, (cnt++ % 2) ? (true) : (false));
    Chip_GPIO_SetPinToggle(LPC_GPIO, GPIO_GREEN_PORT, GPIO_GREEN_PIN);
}

#define BLUE_BLINK_PERIOD 1000000

int main(void)
{
    __enable_irq();
   uint32_t n = BLUE_BLINK_PERIOD;

   sysTimeInit();
   pwmConfig();
   initGpio();

   while(1)
   {
       while(n){
           n--;
       }
       n = BLUE_BLINK_PERIOD;
       switchOutPin();
       Chip_WWDT_Feed(LPC_WWDT);//LPC_WWDT
   }
}
