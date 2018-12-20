
static uint32_t vMaxAnalog;
static uint32_t vMaxDigital;


void adcInit(uint32_t vMaxAndlogIn, uint32_t vMaxDigitalIn)
{
    vMaxAnalog  = vMaxAndlogIn;
    vMaxDigital = vMaxDigitalIn;
}

uint32_t adcGetValue(void)
{

}
