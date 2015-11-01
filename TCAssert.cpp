// 
// 
// 

#include "TCAssert.h"
#include "TCConfig.h"
#include "TCAction.h"
#include "TCCANBus.h"

uint8_t	gLevel = eDbgLevel_Verbose;

void
AssertFailed(
	char const*	inMsg,
	char const*	inFile,
	int			inLine)
{
	for(;;)
	{
		DebugMsg(0, "ASSERT: %s %d %s\n", inFile, inLine, inMsg);
	}
}

void
DebugMsg(
	uint8_t		inLevel,
	char const*	inMsg,
	...)
{
	if(inLevel > gLevel)
		return;

	va_list	varArgs;
	va_start(varArgs, inMsg);
	char	buffer[512];
	vsnprintf(buffer, sizeof(buffer), inMsg, varArgs);
	va_end(varArgs);

	char	timestamp[32];
	uint32_t	remaining = gCurTimeMS / 1000;
	uint32_t	hours = remaining / (60 * 60);
	remaining -= hours * 60 * 60;
	uint32_t	mins = remaining / 60;
	remaining -= mins * 60;
	uint32_t	secs = remaining;

	snprintf(timestamp, sizeof(timestamp), "%02lu:%02lu:%02lu:%03lu", hours, mins, secs, gCurTimeMS % 1000);

	Serial.printf("[%02d|%s] %s", gConfig.GetVal(eConfigVar_NodeID), timestamp, buffer);

	uint8_t	debugNode = gConfig.GetVal(eConfigVar_DebugNode);

	if(debugNode != 0xFF && gCANBus.Ready())
	{
		gAction.SendSerial(debugNode, "[%d|%s] %s", gConfig.GetVal(eConfigVar_NodeID), timestamp, buffer);
	}
}
