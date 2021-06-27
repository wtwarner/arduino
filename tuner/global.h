#ifndef GLOBAL_H_INC_
#define GLOBAL_H_INC_

static const int DAC_CLK = 10;
static const int DAC_DATA = 11;
static const int DAC_LOAD = 12;

static const int VFD_PIN_CLK = 10; // shared with DAC
static const int VFD_PIN_STROBE = 8;
static const int VFD_PIN_DATA = 11; // shared with DAC
static const int VFD_PIN_BLANK = 9;

static const int EYE_PIN_POT_CS = 2;
static const int EYE_PIN_POT_CLK = 0;
static const int EYE_PIN_POT_SDI = 7;
static const int EYE_PIN_PWM = 6; // for negative grid supply 

#endif
