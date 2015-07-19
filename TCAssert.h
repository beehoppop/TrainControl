// TCAssert.h

#ifndef _TCASSERT_h
#define _TCASSERT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define MAssert(x) if(!(x)) AssertFailed(#x, __FILE__, __LINE__)

enum
{
	eDbgLevel_Off = 0,
	eDbgLevel_Basic,
	eDbgLevel_Medium,
	eDbgLevel_Verbose,
};

void
AssertFailed(
	char const*	inMsg,
	char const*	inFile,
	int			inLine);

void
DebugMsg(
	uint8_t		inLevel,
	char const*	inMsg,
	...);
	

#endif

