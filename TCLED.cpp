// 
// 
// 
#include <FastLED.h>

#include "TCModule.h"
#include "TCUtilities.h"
#include "TCLED.h"
#include "TCAssert.h"
#include "TCConfig.h"
#include "TCCommon.h"

CLEDClass gLED;

enum
{
	eMaxLEDs = 128,
	eLEDDataPin = 23,
	eLEDDataSPIPin = 11,

	eUpdateTimeUS = 20000
};

struct SLEDState
{
	uint8_t	r, g, b;
	uint8_t	targetR, targetG, targetB;
	float	pulsesPerSecond;
	bool	pulsing;
	bool	pulsingTransitionOff;
	float	pulseStartTime;
	float	transitionStartTime;
	float	transitionFinishTime;
};

CRGB gFastLEDData[eMaxLEDs];

static SLEDState	gLEDState[eMaxLEDs];
static bool			gUpdateLEDs = false;
static uint32_t		gNumLEDs;

CLEDClass::CLEDClass(
	)
	:
	CModule("leds", 0, eUpdateTimeUS)
{

}

void
CLEDClass::Setup(
	void)
{
	gNumLEDs = gConfig.GetVal(eConfigVar_LEDCount);

	if(gNumLEDs == 0 || gNumLEDs >= eMaxLEDs)
	{
		return;
	}

	if(gConfig.GetVal(eConfigVar_LEDSPIPin))
	{
		FastLED.addLeds<WS2812, eLEDDataSPIPin>(gFastLEDData, gNumLEDs);
	}
	else
	{
		FastLED.addLeds<WS2812, eLEDDataPin>(gFastLEDData, gNumLEDs);
	}
}

void
CLEDClass::Update(
	uint32_t	inDeltaTimeUS)
{
	double	curMS = (double)gCurTimeMS;

	if(gNumLEDs == 0 || gNumLEDs >= eMaxLEDs)
	{
		return;
	}

	//DebugMsg(eDbgLevel_Verbose, "LED: updating %u\n", gCurTimeMS);

	for(uint32_t itr = 0; itr < gNumLEDs; ++itr)
	{
		SLEDState*	curState = gLEDState + itr;
		uint8_t	newR = curState->r;
		uint8_t newG = curState->g;
		uint8_t newB = curState->b;

		if(curState->transitionFinishTime > curState->transitionStartTime)
		{
			if(gCurTimeMS >= curState->transitionFinishTime)
			{
				newR = curState->r = curState->targetR;
				newG = curState->g = curState->targetG;
				newB = curState->b = curState->targetB;
				curState->transitionFinishTime = 0;
				curState->transitionStartTime = 0;
			}
			else
			{
				float transFactor = (float)(gCurTimeMS - curState->transitionStartTime) / (float)(curState->transitionFinishTime - curState->transitionStartTime);
				float transFactorInv = 1.0f - transFactor;
				newR = (uint8_t)((float)curState->r * transFactorInv + (float)curState->targetR * transFactor);
				newG = (uint8_t)((float)curState->g * transFactorInv + (float)curState->targetG * transFactor);
				newB = (uint8_t)((float)curState->b * transFactorInv + (float)curState->targetB * transFactor);
			}
			gUpdateLEDs = true;
		}

		if(curState->pulsing || curState->pulsingTransitionOff)
		{
			float	scaleFactor = (cos((curMS - curState->pulseStartTime) / 1000.0 * curState->pulsesPerSecond * PI * 2) + 1.0f) / 2.0f;
			
			//DebugMsg(eDbgLevel_Verbose, "LED: pulse scale %d %d %f\n", itr, inDeltaTimeUS, scaleFactor);

			if(curState->pulsingTransitionOff && scaleFactor > 0.99f)
			{
				DebugMsg(eDbgLevel_Verbose, "LED: pulse off %d\n", itr);
				curState->pulsing = false;
				curState->pulsingTransitionOff = false;
			}

			newR = (uint8_t)(scaleFactor * (float)newR);
			newG = (uint8_t)(scaleFactor * (float)newG);
			newB = (uint8_t)(scaleFactor * (float)newB);

			gUpdateLEDs = true;
		}

		gFastLEDData[itr].r = newR;
		gFastLEDData[itr].g = newG;
		gFastLEDData[itr].b = newB;
	}

	if(gUpdateLEDs)
	{
		FastLED.show();
		gUpdateLEDs = false;
	}
}

void
CLEDClass::SetColor(
	uint8_t	inLEDIndex,
	uint8_t	inRed,
	uint8_t	inGreen,
	uint8_t	inBlue,
	float	inTransitionTimeMS)
{
	DebugMsg(eDbgLevel_Basic, "LED: Setting %d to color %x %x %x trans %f\n", inLEDIndex, inRed, inGreen, inBlue, inTransitionTimeMS);

	if(inLEDIndex >= gNumLEDs)
	{
		return;
	}

	if(inTransitionTimeMS > 0)
	{
		gLEDState[inLEDIndex].targetR = inRed;
		gLEDState[inLEDIndex].targetG = inGreen;
		gLEDState[inLEDIndex].targetB = inBlue;
		gLEDState[inLEDIndex].transitionStartTime = gCurTimeMS;
		gLEDState[inLEDIndex].transitionFinishTime = gCurTimeMS + inTransitionTimeMS;
	}
	else
	{
		gLEDState[inLEDIndex].r = gFastLEDData[inLEDIndex].r = inRed;
		gLEDState[inLEDIndex].g = gFastLEDData[inLEDIndex].g = inGreen;
		gLEDState[inLEDIndex].b = gFastLEDData[inLEDIndex].b = inBlue;

		gUpdateLEDs = true;
	}
}

void
CLEDClass::PulseOnOff(
	uint8_t		inLEDIndex,
	float		inPulsesPerSecond,
	bool		inPulseOn)
{
	DebugMsg(eDbgLevel_Basic, "LED: Setting  %d to pulse %d %f\n", inLEDIndex, inPulseOn, inPulsesPerSecond);

	if(inLEDIndex >= gNumLEDs)
	{
		return;
	}

	if(inPulseOn)
	{
		gLEDState[inLEDIndex].pulseStartTime = gCurTimeMS;
		gLEDState[inLEDIndex].pulsesPerSecond = inPulsesPerSecond;
		gLEDState[inLEDIndex].pulsing = true;
		gLEDState[inLEDIndex].pulsingTransitionOff = false;
	}
	else
	{
		gLEDState[inLEDIndex].pulsingTransitionOff = true;
	}
}

