// 
// 
// 
#include <stdio.h>

#include "TCCommon.h"
#include "TCAction.h"
#include "TCCANBus.h"
#include "TCConfig.h"
#include "TCUtilities.h"
#include "TCControlSwitch.h"
#include "TCTurnout.h"
#include "TCDCC.h"

CAction gAction;

bool
CAction::SendSerial(
	uint8_t		inDstNodeID,
	char const*	inMsg,
	...)
{
	char buffer[512];

	va_list	varArgs;
	va_start(varArgs, inMsg);
	vsnprintf(buffer, sizeof(buffer), inMsg, varArgs);
	va_end(varArgs);
	
	if(inDstNodeID == gConfig.GetVal(eConfigVar_NodeID))
	{
		Serial.write(buffer);
	}
	else
	{
		gCANBus.SendSerialMsg(inDstNodeID, buffer);
	}

	return true;
}

bool
CAction::SendSerialVA(
	uint8_t		inDstNodeID,
	char const*	inMsg,
	va_list		inVAList)
{
	char buffer[512];

	vsnprintf(buffer, sizeof(buffer), inMsg, inVAList);
	
	if(inDstNodeID == gConfig.GetVal(eConfigVar_NodeID))
	{
		Serial.write(buffer);
	}
	else
	{
		gCANBus.SendSerialMsg(inDstNodeID, buffer);
	}

	return true;
}

bool
CAction::Alive(
	uint8_t	inSrcNodeID,
	uint8_t	inDstNodeID)
{
	if(inDstNodeID == gConfig.GetVal(eConfigVar_NodeID))
	{
		SendSerial(inSrcNodeID, "CC:%d ALIVE v=%s date=%s %s\n", gConfig.GetVal(eConfigVar_NodeID), gVersionStr, __DATE__, __TIME__);
	}
	else
	{
		gCANBus.SendMsg(inDstNodeID, eMsgType_Alive, 0, 0, NULL);
	}

	return true;
}

bool
CAction::TableWrite(
	uint8_t				inSrcNodeID,
	uint8_t				inDstNodeID,
	SMsg_Table const&	inProgram)
{
	if(inDstNodeID == gConfig.GetVal(eConfigVar_NodeID))
	{
		switch(inProgram.type)
		{
			case eTableType_ControlSwitch:
			case eTableType_ControlSwitchToTurnoutMap:
				return gControlSwitch.TableWrite(inSrcNodeID, inProgram);

			case eTableType_TrackTurnout:
			case eTableType_TrackTurnoutLEDMap:
				return gTurnout.TableWrite(inSrcNodeID, inProgram);

			case eTableType_TrackSensor:
				break;

			case eTableType_DCC:
				return gDCC.TableWrite(inSrcNodeID, inProgram);

		}

		return false;
	}

	gCANBus.SendMsg(inDstNodeID, eMsgType_TableWrite, 0, sizeof(inProgram), &inProgram);

	return true;
}

bool
CAction::TableRead(
	uint8_t				inSrcNodeID,
	uint8_t				inDstNodeID,
	SMsg_Table const&	inProgram)
{
	if(inDstNodeID == gConfig.GetVal(eConfigVar_NodeID))
	{
		switch(inProgram.type)
		{
			case eTableType_ControlSwitch:
			case eTableType_ControlSwitchToTurnoutMap:
				return gControlSwitch.TableRead(inSrcNodeID, inProgram);

			case eTableType_TrackTurnout:
			case eTableType_TrackTurnoutLEDMap:
				return gTurnout.TableRead(inSrcNodeID, inProgram);

			case eTableType_TrackSensor:
				break;

			case eTableType_DCC:
				return gDCC.TableRead(inSrcNodeID, inProgram);
		}

		return false;
	}

	gCANBus.SendMsg(inDstNodeID, eMsgType_TableRead, 0, sizeof(inProgram), &inProgram);

	return true;
}

bool
CAction::TableUpdate(
	uint8_t				inSrcNodeID,
	uint8_t				inDstNodeID,
	SMsg_Table const&	inProgram)
{
	if(inDstNodeID == gConfig.GetVal(eConfigVar_NodeID))
	{
		switch(inProgram.type)
		{
			case eTableType_ControlSwitch:
			case eTableType_ControlSwitchToTurnoutMap:
				return gControlSwitch.TableUpdate(inSrcNodeID, inProgram);

			case eTableType_TrackTurnout:
			case eTableType_TrackTurnoutLEDMap:
				return gTurnout.TableUpdate(inSrcNodeID, inProgram);

			case eTableType_TrackSensor:
				break;

			case eTableType_DCC:
				return gDCC.TableUpdate(inSrcNodeID, inProgram);
		}

		return false;
	}

	gCANBus.SendMsg(inDstNodeID, eMsgType_TableUpdate, 0, sizeof(inProgram), &inProgram);

	return true;
}
