// TCAction.h

#ifndef _TCACTION_h
#define _TCACTION_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TCCommon.h"
#include "TCModule.h"

class CAction
{
public:

	bool
	SendSerial(
		uint8_t		inDstNodeID,
		char const*	inMsg,
		...);
	
	bool
	SendSerialVA(
		uint8_t		inDstNodeID,
		char const*	inMsg,
		va_list		inVAList);

	bool
	Alive(
		uint8_t	inSrcNodeID,
		uint8_t	inDstNodeID);

	bool
	TableWrite(
		uint8_t				inSrcNodeID,
		uint8_t				inDstNodeID,
		SMsg_Table const&	inProgram);

	bool
	TableRead(
		uint8_t				inSrcNodeID,
		uint8_t				inDstNodeID,
		SMsg_Table const&	inProgram);

	bool
	TableUpdate(
		uint8_t				inSrcNodeID,
		uint8_t				inDstNodeID,
		SMsg_Table const&	inProgram);

private:

};

extern CAction gAction;

#endif

