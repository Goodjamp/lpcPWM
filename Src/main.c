#include "stdint.h"

#include "cmsis.h"

#include "chip.h"
#include "clock_11u6x.h"
#include "timer_11u6x.h"
#include "clock_11u6x.h"
#include "gpio_11u6x.h"
#include "iocon_11u6x.h"
#include "wwdt_11u6x.h"

#include "xpressLedDesc.h"


typedef uint32_t timeoutProcessing;

typedef void (*sysTimeTickCallbak)(void);
uint32_t getTime(void);
const uint32_t OscRateIn = 12000000;
const uint32_t RTCOscRateIn = 32768;

LPC_TIMER_T *sysTimeTimer = LPC_TIMER32_0;
#define PWM_MATCH_CH        0
#define PERIOD_MATCH_CH     1
#define OUT_PWM_CH          2
uint32_t currentTime;
uint32_t tempTime;

void sysTimeInit(void){
    uint32_t timerClockRate = Chip_Clock_GetSystemClockRate();
    // sys timer clock rate should be 1 kHz
    Chip_TIMER_Init(sysTimeTimer);
    Chip_TIMER_PrescaleSet(sysTimeTimer,  timerClockRate/1000000);
    Chip_TIMER_Enable(sysTimeTimer);
}


void pwmConfig(void)
{
    uint16_t kPWM = 0;
    uint16_t kOutPwm = 1000;
    uint16_t period  = 20000;
    Chip_TIMER_SetMatch(sysTimeTimer, PERIOD_MATCH_CH, period);
    Chip_TIMER_ResetOnMatchEnable(sysTimeTimer, PERIOD_MATCH_CH);
    Chip_TIMER_MatchEnableInt(sysTimeTimer, PERIOD_MATCH_CH);

    Chip_TIMER_SetMatch(sysTimeTimer, PWM_MATCH_CH, kPWM);
    Chip_TIMER_MatchEnableInt(sysTimeTimer, PWM_MATCH_CH);

    Chip_TIMER_SetMatch(sysTimeTimer, OUT_PWM_CH, kOutPwm);
    Chip_TIMER_MatchEnableInt(sysTimeTimer, OUT_PWM_CH);

    NVIC_EnableIRQ(TIMER_32_0_IRQn);
}



void TIMER32_0_IRQHandler(void)
{
    volatile static uint16_t kPWM = 0;
    uint16_t period = 2000;
    volatile static uint8_t trig = 1;

    if(Chip_TIMER_MatchPending(sysTimeTimer, PWM_MATCH_CH))
    {
        Chip_GPIO_SetPinOutHigh(LPC_GPIO, GPIO_GREEN_PORT, GPIO_GREEN_PIN);
        Chip_GPIO_SetPinOutHigh(LPC_GPIO, GPIO_RED_PORT, GPIO_RED_PIN);


        //Chip_GPIO_SetPinOutHigh(LPC_GPIO, GPIO_BLUE_PORT, GPIO_BLUE_PIN);

        Chip_TIMER_ClearMatch(sysTimeTimer, PWM_MATCH_CH);
        Chip_TIMER_SetMatch(sysTimeTimer, PWM_MATCH_CH, (trig) ? (kPWM++) : (kPWM--) );
        if(kPWM == period)
        {
            trig = 0;
        }
        if(kPWM == 0)
        {
            trig = 1;
        }
    }
    //Chip_TIMER_SetMatch(sysTimeTimer, 0, Chip_TIMER_ReadCount(sysTimeTimer) + 10);
    if(Chip_TIMER_MatchPending(sysTimeTimer, PERIOD_MATCH_CH))
    {
        Chip_GPIO_SetPinOutLow(LPC_GPIO, GPIO_GREEN_PORT, GPIO_GREEN_PIN);
        Chip_GPIO_SetPinOutLow(LPC_GPIO, GPIO_RED_PORT, GPIO_RED_PIN);
        //Chip_GPIO_SetPinOutLow(LPC_GPIO, GPIO_BLUE_PORT, GPIO_BLUE_PIN);

        Chip_GPIO_SetPinOutHigh(LPC_GPIO, GPIO_OUT1_PORT, GPIO_OUT1_PIN);
        Chip_GPIO_SetPinOutHigh(LPC_GPIO, GPIO_OUT2_PORT, GPIO_OUT2_PIN);

        Chip_TIMER_ClearMatch(sysTimeTimer, PERIOD_MATCH_CH);
    }

    if(Chip_TIMER_MatchPending(sysTimeTimer, OUT_PWM_CH))
    {
        Chip_GPIO_SetPinOutLow(LPC_GPIO, GPIO_OUT1_PORT, GPIO_OUT1_PIN);
        Chip_GPIO_SetPinOutLow(LPC_GPIO, GPIO_OUT2_PORT, GPIO_OUT2_PIN);
        //Chip_GPIO_SetPinOutLow(LPC_GPIO, GPIO_BLUE_PORT, GPIO_BLUE_PIN);

        Chip_TIMER_ClearMatch(sysTimeTimer, OUT_PWM_CH);
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

    uint8_t pinDesc[3][2] =
    {
        {GPIO_RED_PORT, GPIO_RED_PIN},
        {GPIO_GREEN_PORT, GPIO_GREEN_PIN},
        {GPIO_BLUE_PORT, GPIO_BLUE_PIN},
    };
    for(uint8_t k = 0; k < 3; k++)
    {
       Chip_IOCON_PinMux(LPC_IOCON, pinDesc[k][0], pinDesc[k][1], IOCON_DIGMODE_EN | IOCON_MODE_INACT, IOCON_FUNC0);
       Chip_GPIO_SetPinDIROutput(LPC_GPIO, pinDesc[k][0], pinDesc[k][1]);
       Chip_GPIO_SetPinState(LPC_GPIO, pinDesc[k][0], pinDesc[k][1], true);
    }

    //PWM out
    uint8_t pinOutDesc[2][2] =
    {
        {GPIO_OUT1_PORT, GPIO_OUT1_PIN},
        {GPIO_OUT2_PORT, GPIO_OUT2_PIN}
    };
    for(uint8_t k = 0; k < 2; k++)
    {
       Chip_IOCON_PinMux(LPC_IOCON, pinOutDesc[k][0], pinOutDesc[k][1], IOCON_DIGMODE_EN | IOCON_MODE_INACT, IOCON_FUNC0);
       Chip_GPIO_SetPinDIROutput(LPC_GPIO, pinOutDesc[k][0], pinOutDesc[k][1]);
       Chip_GPIO_SetPinState(LPC_GPIO, pinOutDesc[k][0], pinOutDesc[k][1], true);
    }

}

#define BLUE_BLINK_PERIOD 1000000

int main(void)
{
    __enable_irq();

   sysTimeInit();
   pwmConfig();
   uint32_t n = BLUE_BLINK_PERIOD;

   currentTime = getTime() + 100;
   initGpio();
   while(1)
   {
       while(n){
           n--;
       }
       n = BLUE_BLINK_PERIOD;
       //Chip_GPIO_SetPinState(LPC_GPIO, GPIO_BLUE_PORT, GPIO_BLUE_PIN, (cnt++) ? (true) : (false));
       //Chip_GPIO_SetPinToggle(LPC_GPIO, GPIO_BLUE_PORT, GPIO_BLUE_PIN);
       Chip_WWDT_Feed(LPC_WWDT);//LPC_WWDT
   }
}
