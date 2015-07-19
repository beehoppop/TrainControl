// TCConfig.h

#ifndef _TCCONFIG_h
#define _TCCONFIG_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TCModule.h"

enum
{
	eConfigVar_NodeID,
	eConfigVar_DebugLevel,
	eConfigVar_TrackWaveformPin,
	eConfigVar_TrackPowerPin,
	eConfigVar_ProgramWaveformPin,
	eConfigVar_ProgramPowerPin,
	eConfigVar_ProgramCurrentSensePin,
	eConfigVar_LEDCount,

	eConfigVar_Max = 16
};

class CModule_Config : public CModule
{
public:
	
	CModule_Config(
		);

	virtual void
	Setup(
		void);
	
	uint8_t
	GetVal(
		uint8_t	inVar);

	void
	SetVal(
		uint8_t	inVar,
		uint8_t	inVal);

private:

	uint8_t	configVar[eConfigVar_Max];
};

extern CModule_Config	gConfig;

#endif
