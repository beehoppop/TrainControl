// TCDigitalInput.h

#ifndef _TCDIGITALINPUT_h
#define _TCDIGITALINPUT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TCModule.h"

enum EPinInOutMode
{
	ePinMode_Unused,
	ePinMode_Input,
	ePinMode_Output,
};

class CModule_DigitalIO : public CModule
{
public:
	
	CModule_DigitalIO(
		);

	virtual void
	Update(
		uint32_t	inDeltaTimeUS);

	virtual void
	Setup(
		void);

	void
	SetPinMode(
		uint8_t			inPin,
		EPinInOutMode	inMode);

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
	uint8_t		pinInOutMode[eDIOPinCount];
	uint32_t	pinTime[eDIOPinCount];
	uint8_t		inputPinLastState[eDIOPinCount];
	bool		pinActivated[eDIOPinCount];
};

extern CModule_DigitalIO	gDigitalIO;

#endif
