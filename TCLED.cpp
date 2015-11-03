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
	uint32_t	pulseStartTime;
	uint32_t	transitionStartTime;
	uint32_t	transitionFinishTime;
};

static uint32_t		gNumLEDs;
static SLEDState	gLEDState[eMaxLEDs];
static CRGB			gFastLEDData[eMaxLEDs];
static bool			gCycle;

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

	memset(gLEDState, 0, sizeof(gLEDState));
	memset(gFastLEDData, 0, sizeof(gFastLEDData));

	FastLED.show();
}

void
CLEDClass::Update(
	uint32_t	inDeltaTimeUS)
{
	if(gNumLEDs == 0 || gNumLEDs >= eMaxLEDs)
	{
		return;
	}

	uint8_t	newR;
	uint8_t newG;
	uint8_t newB;

	if(gCycle)
	{
		for(uint32_t itr = 0; itr < gNumLEDs; ++itr)
		{
			uint32_t	offset = itr * 1000 / gNumLEDs;
			float	timeVal = float((gCurTimeMS + offset) % 1000) / 1000.0;

			newR = (cos((timeVal * 1.0f) * PI * 2) + 1.0f) / 2.0f * 255.0;
			newG = (cos((timeVal * 2.0f) * PI * 2) + 1.0f) / 2.0f * 255.0;
			newB = (cos((timeVal * 3.0f) * PI * 2) + 1.0f) / 2.0f * 255.0;
			gFastLEDData[itr].r = newR;
			gFastLEDData[itr].g = newG;
			gFastLEDData[itr].b = newB;
		}

		FastLED.show();
		return;
	}

	bool	updateLEDs = false;

	//DebugMsg(eDbgLevel_Verbose, "LED: updating %u\n", gCurTimeMS);

	for(uint32_t itr = 0; itr < gNumLEDs; ++itr)
	{
		SLEDState*	curState = gLEDState + itr;

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
			updateLEDs = true;
		}
		else
		{
			newR = curState->r;
			newG = curState->g;
			newB = curState->b;
		}

		if(curState->pulsing || curState->pulsingTransitionOff)
		{
			float	scaleFactor = (cos((float)(gCurTimeMS - curState->pulseStartTime) / 1000.0 * curState->pulsesPerSecond * PI * 2) + 1.0f) / 2.0f;
			
			//DebugMsg(eDbgLevel_Verbose, "LED: pulse scale %d %d %f\n", itr, inDeltaTimeUS, scaleFactor);

			if(curState->pulsingTransitionOff && scaleFactor > 0.99f)
			{
				DebugMsg(eDbgLevel_Verbose, "LED: pulse off %d\n", itr);
				curState->pulsing = false;
				curState->pulsingTransitionOff = false;
				scaleFactor = 1.0f;
			}

			newR = (uint8_t)(scaleFactor * (float)newR);
			newG = (uint8_t)(scaleFactor * (float)newG);
			newB = (uint8_t)(scaleFactor * (float)newB);

			updateLEDs = true;
		}

		gFastLEDData[itr].r = newR;
		gFastLEDData[itr].g = newG;
		gFastLEDData[itr].b = newB;
	}

	if(updateLEDs)
	{
		FastLED.show();
		updateLEDs = false;
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

	if(inTransitionTimeMS < 1)
	{
		inTransitionTimeMS = 1;
	}

	SLEDState*	curState = gLEDState + inLEDIndex;

	if(curState->transitionFinishTime > curState->transitionStartTime && curState->transitionFinishTime > gCurTimeMS)
	{
		// We are still transitioning to a new color - set the rgb fields to our in progress result so that we transition from there
		float transFactor = (float)(gCurTimeMS - curState->transitionStartTime) / (float)(curState->transitionFinishTime - curState->transitionStartTime);
		float transFactorInv = 1.0f - transFactor;
		curState->r = (uint8_t)((float)curState->r * transFactorInv + (float)curState->targetR * transFactor);
		curState->g = (uint8_t)((float)curState->g * transFactorInv + (float)curState->targetG * transFactor);
		curState->b = (uint8_t)((float)curState->b * transFactorInv + (float)curState->targetB * transFactor);
	}

	curState->targetR = inRed;
	curState->targetG = inGreen;
	curState->targetB = inBlue;
	curState->transitionStartTime = gCurTimeMS;
	curState->transitionFinishTime = gCurTimeMS + inTransitionTimeMS;
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

void
CLEDClass::CycleAll(
	bool	inOn)
{
	gCycle = inOn;
}

