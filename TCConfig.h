// TCConfig.h

#ifndef _TCCONFIG_h
#define _TCCONFIG_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TCModule.h"

enum ETouchMode
{
	eTouchMode_Off,
	eTouchMode_BuiltIn,
	eTouchMode_MPR121
};

enum
{
	eConfigVar_NodeID,
	eConfigVar_DebugLevel,
	eConfigVar_LEDCount,
	eConfigVar_LEDSPIPin,
	eConfigVar_TouchMode,
	eConfigVar_DebugNode,	// Serial port if 0xFF
	eConfigVar_BlinkTeensyLED,

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
	
	virtual void
	ResetState(
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
	bool	setupComplete;
};

extern CModule_Config	gConfig;

#endif
