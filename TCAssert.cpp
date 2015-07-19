// 
// 
// 

#include "TCAssert.h"
#include "TCConfig.h"
#include "TCAction.h"

uint8_t	gLevel;

void
AssertFailed(
	char const*	inMsg,
	char const*	inFile,
	int			inLine)
{
	for(;;)
	{
		Serial.printf("ASSERT: %s %d %s\n", inFile, inLine, inMsg);
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
	#if 1
		gAction.SendSerialVA(gConfig.GetVal(eConfigVar_NodeID), inMsg, varArgs);	// XXX - Add a config var for the serial port node id and send messages to there
	#else
		char	buffer[512];
		vsnprintf(buffer, sizeof(buffer), inMsg, varArgs);
		Serial.print(buffer);
	#endif

	va_end(varArgs);
}
