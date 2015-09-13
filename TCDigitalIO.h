// TCDigitalInput.h

#ifndef _TCDIGITALINPUT_h
#define _TCDIGITALINPUT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TCModule.h"

class CModule_DigitalIO : public CModule
{
public:
	
	CModule_DigitalIO(
		);

	virtual void
	Update(
		uint32_t	inDeltaTimeUS);

	bool
	CheckInputActivated(
		uint8_t	inPin);

	void
	SetOutputHighWithTimeout(
		uint8_t		inPin,
		uint32_t	inDurationMS);

	void
	SetOutputLow(
		uint8_t	inPin);

private:
	uint32_t	inputPinLastChange[eDIOPinCount];
	uint8_t		inputPinLastState[eDIOPinCount];
	uint32_t	outputPinState[eDIOPinCount];	// 0 if the pin should be low, if > 0 pin should be high and value is millis for when pin should be low
};

extern CModule_DigitalIO	gDigitalIO;

#endif
