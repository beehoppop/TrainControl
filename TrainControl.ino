#include <EEPROM.h>
#include <TimerOne.h>
#include <FlexCAN.h>
#include <FastLED.h>
#include <Wire.h>

#include "TCModule.h"

char const*	gVersionStr = "0.4.0";

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
	CModule::LoopAll();
}
