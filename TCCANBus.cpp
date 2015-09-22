// 
// 
// 

#include "TCConfig.h"
#include "TCCommon.h"
#include "TCCANBus.h"
#include "TCUtilities.h"
#include "TCAction.h"
#include "TCAssert.h"
#include "TCControlSwitch.h"
#include "TCTurnout.h"
#include "TCState.h"

enum
{
	eMaxDataSize = 8
};

CModule_CANBus gCANBus;

void
CANIDToComponents(
	uint32_t	inID,
	uint8_t&	outSrcNode,
	uint8_t&	outDstNode,
	uint8_t&	outMsgType,
	uint8_t&	outMsgFlags)
{
	outMsgType = (inID >> 24) & 0x1F;
	outDstNode = (inID >> 16) & 0xFF;
	outSrcNode = (inID >> 8) & 0xFF;
	outMsgFlags = (inID >> 0) & 0xFF;
}

uint32_t
CANIDFromComponents(
	uint8_t	inSrcNode,
	uint8_t	inDstNode,
	uint8_t	inMsgType,
	uint8_t	inMsgFlags)
{
	return ((uint32_t)inMsgType << 24) | ((uint32_t)inDstNode << 16) | ((uint32_t)inSrcNode << 8) | ((uint32_t)inMsgFlags << 0);
}

CModule_CANBus::CModule_CANBus(
	)
	:
	CModule(MMakeUID('c', 'a', 'n', 'b'), 0),
	canBus(500000)
{

}

void
CModule_CANBus::SendMsg(
	uint8_t		inDstNode,
	uint8_t		inMsgType,
	uint8_t		inMsgFlags,
	uint8_t		inMsgSize,
	void const*	inMsgData)
{
	CAN_message_t	msg;

	//DebugMsg(eDbgLevel_Basic, "CAN: %02x TXM src=0x%x dst=0x%x typ=0x%x flg=0x%x\n", gNodeID, gNodeID, inDstNode, inMsgType, inMsgFlags);

	MAssert(inMsgSize <= eMaxDataSize);

	msg.id = CANIDFromComponents(gConfig.GetVal(eConfigVar_NodeID), inDstNode, inMsgType, inMsgFlags);
	msg.ext = 1;
	msg.timeout = 100;
	msg.len = inMsgSize;
	if(inMsgData != NULL && inMsgSize > 0)
		memcpy(msg.buf, inMsgData, inMsgSize);

	canBus.write(msg);
}

void
CModule_CANBus::SendSerialMsg(
	uint8_t		inDstNode,
	char const*	inMsg,
	...)
{
	CAN_message_t	msg;

	msg.id = CANIDFromComponents(gConfig.GetVal(eConfigVar_NodeID), inDstNode, eMsgType_SerialOut, 0);
	msg.ext = 1;
	msg.timeout = 100;

	int	msgLen = 0;

	for(;;)
	{
		char c = *inMsg++;

		if(c == 0)
			break;

		msg.buf[msgLen++] = c;

		if(msgLen >= 8)
		{
			msg.len = 8;
			canBus.write(msg);
			msgLen = 0;
		}
	}

	if(msgLen > 0)
	{
		msg.len = msgLen;
		canBus.write(msg);
	}
}
	
void
CModule_CANBus::Setup(
	void)
{

}

void
ProcessCANMsg(
	CAN_message_t const&	inMsg)
{
	uint8_t	srcNode;
	uint8_t	dstNode;
	uint8_t	msgType;
	uint8_t	flags;

	CANIDToComponents(inMsg.id, srcNode, dstNode, msgType, flags);

	//Serial.printf("%02x RCV src=0x%x dst=0x%x typ=0x%x flg=0x%x\n", gNodeID, srcNode, dstNode, msgType, flags);

	if(dstNode != 0xFF && dstNode != gConfig.GetVal(eConfigVar_NodeID))
		return;

	switch(msgType)
	{
		case eMsgType_ControlSwitch:
		{
			SMsg_ControlSwitch&	msg = (SMsg_ControlSwitch&)inMsg;
			gControlSwitch.ControlSwitchActivated(msg.id, msg.direction);
			break;
		}

		case eMsgType_TrackTurnout:
		{
			SMsg_TrackTurnout&	msg = (SMsg_TrackTurnout&)inMsg;
			gTurnout.SetTurnoutDirection(msg.id, msg.direction);
			break;
		}

		case eMsgType_TurnoutControlSwitchTouch:
		{
			SMsg_TurnoutControlSwitchTouch& msg = (SMsg_TurnoutControlSwitchTouch&)inMsg;
			gTurnout.ControlSwitchTouchedTurnoutID(msg.turnoutID, msg.touched, false);
			break;
		}

		case eMsgType_TrackSensor:
			break;

		case eMsgType_TableWrite:
			gAction.TableWrite(srcNode, gConfig.GetVal(eConfigVar_NodeID), *(SMsg_Table*)inMsg.buf);
			break;

		case eMsgType_TableRead:
			gAction.TableRead(srcNode, gConfig.GetVal(eConfigVar_NodeID), *(SMsg_Table*)inMsg.buf);
			break;

		case eMsgType_TableUpdate:
			gAction.TableUpdate(srcNode, gConfig.GetVal(eConfigVar_NodeID), *(SMsg_Table*)inMsg.buf);
			break;

		case eMsgType_SerialOut:
			Serial.write(inMsg.buf, inMsg.len);
			break;

		case eMsgType_Alive:
			gAction.Alive(srcNode, gConfig.GetVal(eConfigVar_NodeID));
			break;

		case eMsgType_ConfigVar:
		{
			SMsg_ConfigVar&	msg = (SMsg_ConfigVar&)inMsg;

			if(msg.setVar)
			{
				gConfig.SetVal(msg.configVar, msg.value);
				gAction.SendSerial(
					gConfig.GetVal(eConfigVar_NodeID),
					"CC:%d config_var set %d %d\n", 
					gConfig.GetVal(eConfigVar_NodeID),
					msg.configVar,
					msg.value);
			}
			else
			{
				uint8_t	val = gConfig.GetVal(msg.configVar);
				gAction.SendSerial(
					gConfig.GetVal(eConfigVar_NodeID),
					"CC:%d config_var get %d %d\n", 
					gConfig.GetVal(eConfigVar_NodeID),
					msg.configVar,
					val);
			}
			break;
		}

		case eMsgType_StateVar:
		{
			SMsg_StateVar&	msg = (SMsg_StateVar&)inMsg;
			gState.SetVal(msg.stateVar, msg.value);
			break;
		}

		case eMsgType_Restart:
			CPU_RESTART();
			break;
	}
}

void
CModule_CANBus::Update(
	uint32_t	inDeltaTimeUS)
{
	CAN_message_t	msg;

	while(canBus.available())
	{
		canBus.read(msg);
		ProcessCANMsg(msg);
	}
}

void
CModule_CANBus::DumpMsg(
	CAN_message_t const&	inMsg)
{
	Serial.printf("id: 0x%08x\n", inMsg.id);
	Serial.printf("ext: 0x%x\n", inMsg.ext);
	Serial.printf("len: 0x%x\n", inMsg.len);
	Serial.printf("timeout: 0x%x\n", inMsg.timeout);
	Serial.printf("data: %02x %02x %02x %02x %02x %02x %02x %02x\n",
		inMsg.buf[0], 
		inMsg.buf[1], 
		inMsg.buf[2], 
		inMsg.buf[3], 
		inMsg.buf[4], 
		inMsg.buf[5], 
		inMsg.buf[6], 
		inMsg.buf[7]
		);
}
