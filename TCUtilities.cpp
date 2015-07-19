// 
// 
// 
#include <EEPROM.h>

#include "TCCommon.h"
#include "TCUtilities.h"


bool
IsStrDigit(
	char const*	inStr)
{
	if(inStr == NULL)
	{
		return false;
	}

	char const*	cp = inStr;

	for(;;)
	{
		char c = *cp++;

		if(c == 0)
		{
			return true;
		}

		if(!isdigit(c))
		{
			return false;
		}
	}

	return false;
}

bool
IsStrAlpha(
	char const*	inStr)
{
	if(inStr == NULL)
	{
		return false;
	}

	char const*	cp = inStr;

	for(;;)
	{
		char c = *cp++;

		if(c == 0)
		{
			return true;
		}

		if(!isalpha(c) && c != '_')
		{
			return false;
		}
	}

	return false;
}

uint8_t
GetTurnoutDirection(
	char const*	inStr)
{
	if(strcmp(inStr, "straight") == 0)
	{
		return eTurnDir_Straight;
	}
	else if(strcmp(inStr, "turnout") == 0)
	{
		return eTurnDir_Turnout;
	}

	return 0xFF;
}

void
LoadDataFromEEPROM(
	void*		inDst,
	uint16_t	inStartAddress,
	uint16_t	inSize)
{
	uint8_t*	cp = (uint8_t*)inDst;

	for(uint16_t idx = inStartAddress; idx < inStartAddress + inSize; ++idx)
	{
		*cp++ = EEPROM.read(idx);
	}
}

void
WriteDataToEEPROM(
	void*		inSrc,
	uint16_t	inStartAddress,
	uint16_t	inSize)
{
	uint8_t*	cp = (uint8_t*)inSrc;

	for(uint16_t idx = inStartAddress; idx < inStartAddress + inSize; ++idx)
	{
		EEPROM.write(idx, *cp++);
	}
}

char const*
StringizeUInt32(
	uint32_t	inValue)
{
	static char buffer[5];
	buffer[0] = (char)((inValue >> 24) & 0xFF);
	buffer[1] = (char)((inValue >> 16) & 0xFF);
	buffer[2] = (char)((inValue >> 8) & 0xFF);
	buffer[3] = (char)((inValue >> 0) & 0xFF);
	buffer[4] = 0;
	return buffer;
}
