// TCModule.h

#ifndef _TCMODULE_h
#define _TCMODULE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


enum
{
	eMaxModuleCount = 16,
};

class CModule
{
public:
	
	CModule(
		uint32_t	inUID,
		uint16_t	inEEPROMSize,
		uint32_t	inUpdateTimeUS = 0);

	virtual void
	Setup(
		void);

	virtual void
	Update(
		uint32_t	inDeltaTimeUS);
	
	virtual void
	EEPROMInitialize(
		void);

	static void
	SetupAll(
		void);

	static void
	LoopAll(
		void);

	uint32_t		uid;

protected:

	uint16_t		eepromSize;
	uint16_t		eepromOffset;
	uint32_t		updateTimeUS;
	uint32_t		lastUpdateUS;
};

extern uint32_t		gCurTimeMS;
extern uint32_t		gCurTimeUS;

#endif
