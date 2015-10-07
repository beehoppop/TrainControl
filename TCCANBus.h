// TCCANBus.h

#ifndef _TCCANBUS_h
#define _TCCANBUS_h

#include <FlexCAN.h>

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


class CModule_CANBus : public CModule
{
public:

	CModule_CANBus(
		);
	
	virtual void
	Setup(
		void);

	virtual void
	TearDown(
		void);

	virtual void
	Update(
		uint32_t	inDeltaTimeUS);

	void
	SendMsg(
		uint8_t		inDstNode,
		uint8_t		inMsgType,
		uint8_t		inMsgFlags,
		uint8_t		inMsgSize,
		void const*	inMsgData);

	void
	SendSerialMsg(
		uint8_t		inDstNode,
		char const*	inMsg,
		...);


private:

	void
	ProcessCANMsg(
		CAN_message_t const&	inMsg);

	void
	DumpMsg(
		CAN_message_t const&	inMsg);

	FlexCAN	canBus;
};

extern CModule_CANBus gCANBus;

#endif
