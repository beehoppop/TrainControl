// TCState.h

#ifndef _TCSTATE_h
#define _TCSTATE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TCModule.h"

enum
{
	eStateVar_TrackPower = 0,
	eStateVar_ProgramPower,
	eStateVar_Max = 16,

};

class CModule_State : public CModule
{
public:
	
	CModule_State(
		);

	uint8_t
	GetVal(
		uint8_t	inVar);

	void
	SetVal(
		uint8_t	inVar,
		uint8_t	inVal);

private:

	uint8_t	stateVar[eStateVar_Max];
};

extern CModule_State	gState;

#endif
