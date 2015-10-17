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

	Serial.printf("[%d] %s", gConfig.GetVal(eConfigVar_NodeID), buffer);

	uint8_t	debugNode = gConfig.GetVal(eConfigVar_DebugNode);

	if(debugNode != 0xFF && gCANBus.Ready())
	{
		gAction.SendSerial(debugNode, "[%d] %s", gConfig.GetVal(eConfigVar_NodeID), buffer);
	}
}
