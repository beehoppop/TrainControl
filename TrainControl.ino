#include <EEPROM.h>
#include <TimerOne.h>
#include <FlexCAN.h>
#include <FastLED.h>

#include "TCModule.h"

uint32_t	gCurTimeMS;
char const*	gVersionStr = "0.2.0";

void
setup(
	void)
{
	CModule::SetupAll();
}

void 
loop(
	void)
{
	gCurTimeMS = millis();

	CModule::LoopAll();
}
