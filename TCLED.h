// TCLED.h

#ifndef _TCLED_h
#define _TCLED_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TCModule.h"

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
		void);

	void
	SetLEDColor(
		uint8_t		inLEDIndex,
		uint32_t	inColor);
};

extern CLEDClass gLED;

#endif

