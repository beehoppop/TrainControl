// TCUtilities.h

#ifndef _TCUTILITIES_h
#define _TCUTILITIES_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define MMakeUID(c0, c1, c2, c3) (((uint32_t)(c0) << 24) | ((uint32_t)(c1) << 16) | ((uint32_t)(c2) << 8) | ((uint32_t)(c3) << 0))

bool
IsStrDigit(
	char const*	inStr);

bool
IsStrAlpha(
	char const*	inStr);

uint8_t
GetTurnoutDirection(
	char const*	inStr);

void
LoadDataFromEEPROM(
	void*		inDst,
	uint16_t	inStartAddress,
	uint16_t	inSize);

void
WriteDataToEEPROM(
	void*		inSrc,
	uint16_t	inStartAddress,
	uint16_t	inSize);

char const*
StringizeUInt32(
	uint32_t	inValue);

#endif

