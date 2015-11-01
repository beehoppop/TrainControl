// TCLED.h

#ifndef _TCLED_h
#define _TCLED_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TCModule.h"

enum
{
	eMaxLEDCount = 64
};

class CLEDClass : public CModule
{
public:
	
	CLEDClass(
		);

	virtual void
	Setup(
		void);

	virtual void
	Update(
		uint32_t	inDeltaTimeUS);

	void
	SetColor(
		uint8_t	inLEDIndex,
		uint8_t	inRed,
		uint8_t	inGreen,
		uint8_t	inBlue,
		float	inTransitionTimeMS);

	void
	PulseOnOff(
		uint8_t		inLEDIndex,
		float		inPulsesPerSecond,
		bool		inPulseOn);

	void
	CycleAll(
		bool	inOn);
};

extern CLEDClass gLED;

#endif

