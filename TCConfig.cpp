// 
// 
// 
#include <EEPROM.h>

#include "TCAssert.h"
#include "TCCommon.h"
#include "TCConfig.h"
#include "TCUtilities.h"

CModule_Config::CModule_Config(
	)
	:
	CModule("cnfg", sizeof(configVar), 0, 255)
{
	setupComplete = false;
	configVar[eConfigVar_DebugLevel] = eDbgLevel_Basic;
}

void
CModule_Config::Setup(
	void)
{
	if(setupComplete)
	{
		return;
	}

	MAssert(eepromOffset > 0);

	LoadDataFromEEPROM(configVar, eepromOffset, sizeof(configVar));

	configVar[eConfigVar_DebugLevel] = eDbgLevel_Basic;

	setupComplete = true;
}

void
CModule_Config::ResetState(
	void)
{
}

uint8_t
CModule_Config::GetVal(
	uint8_t	inVar)
{
	Setup();
	MAssert(inVar < eConfigVar_Max);
	return configVar[inVar];
}

void
CModule_Config::SetVal(
	uint8_t	inVar,
	uint8_t	inVal)
{
	Setup();
	MAssert(inVar < eConfigVar_Max);
	configVar[inVar] = inVal;
	EEPROM.write(eepromOffset + inVar, inVal);
}

CModule_Config	gConfig;
