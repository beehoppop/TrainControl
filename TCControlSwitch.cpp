// 
// 
// 

#include "TCUtilities.h"
#include "TCControlSwitch.h"
#include "TCTurnout.h"
#include "TCAction.h"
#include "TCConfig.h"
#include "TCDigitalIO.h"
#include "TCCANBus.h"

CModule_ControlSwitch	gControlSwitch;

CModule_ControlSwitch::CModule_ControlSwitch(
	)
	:
	CModule(MMakeUID('c', 'n', 's', 'w'), sizeof(controlSwitchArray) + sizeof(controlSwitchToTurnoutMapArray))
{

}

void
CModule_ControlSwitch::Setup(
	void)
{
	LoadDataFromEEPROM(controlSwitchArray, eepromOffset, sizeof(controlSwitchArray));
	LoadDataFromEEPROM(controlSwitchToTurnoutMapArray, eepromOffset + sizeof(controlSwitchArray), sizeof(controlSwitchToTurnoutMapArray));
	UpdateTables();
}

void
CModule_ControlSwitch::Update(
	void)
{
	for(int i = 0; i < eMaxControlSwitchCount; ++i)
	{
		if(controlSwitchArray[i].id == eInvalidID)
			continue;

		if(gDigitalIO.CheckInputActivated(controlSwitchArray[i].straightDInPin))
		{
			ControlSwitchActivated(controlSwitchArray[i].id, eTurnDir_Straight, true);
		}
		else if(gDigitalIO.CheckInputActivated(controlSwitchArray[i].turnDInPin))
		{
			ControlSwitchActivated(controlSwitchArray[i].id, eTurnDir_Turnout, true);
		}
	}	
}

void
CModule_ControlSwitch::ControlSwitchActivated(
	uint16_t	inID,
	uint8_t		inDirection,
	bool		inBroadcast)
{
	SMsg_ControlSwitch	msg;

	msg.timeMS = gCurTimeMS;
	msg.id = inID;
	msg.direction = inDirection;

	if(inBroadcast)
	{
		//SendSerialMsg(gNodeID, "CC:%d cs activated id=%d dir=%s\n", gNodeID, inID, inTurnDirection == eTurnDir_Straight ? "straight" : "turnout");
		gCANBus.SendMsg(0xFF, eMsgType_ControlSwitch, 0, sizeof(msg), &msg);
	}

	for(int i = 0; i < eMaxControlSwitchToTurnoutMapCount; ++i)
	{
		if(controlSwitchToTurnoutMapArray[i].controlSwitchID == eInvalidID || controlSwitchToTurnoutMapArray[i].trackTurnoutID == eInvalidID)
		{
			continue;
		}

		if(inID == controlSwitchToTurnoutMapArray[i].controlSwitchID)
		{
			if(controlSwitchToTurnoutMapArray[i].invert)
			{
				inDirection = inDirection == eTurnDir_Straight ? eTurnDir_Turnout : eTurnDir_Straight;
			}

			gTurnout.SetTurnoutDirection(controlSwitchToTurnoutMapArray[i].trackTurnoutID, inDirection);
		}
	}
}

bool
CModule_ControlSwitch::TableRead(
	int8_t				inSrcNode,
	SMsg_Table const&	inProgram)
{
	if(inProgram.type == eTableType_ControlSwitch)
	{
		if(inProgram.index >= eMaxControlSwitchCount)
		{
			return false;
		}

		gAction.SendSerial(
			inSrcNode,
			"CC:%d control_switch index=%d id=%d straightPin=%d turnPin=%d\n",
			gConfig.GetVal(eConfigVar_NodeID), 
			inProgram.index, 
			controlSwitchArray[inProgram.index].id, 
			controlSwitchArray[inProgram.index].straightDInPin, 
			controlSwitchArray[inProgram.index].turnDInPin);

		return true;
	}

	if(inProgram.type == eTableType_ControlSwitchToTurnoutMap)
	{
		if(inProgram.index >= eMaxControlSwitchToTurnoutMapCount)
		{
			return false;
		}

		gAction.SendSerial(
			inSrcNode,
			"CC:%d turnout_map index=%d cs_id=%d tt_id=%d invert=%d\n", 
			gConfig.GetVal(eConfigVar_NodeID), 
			inProgram.index, 
			controlSwitchToTurnoutMapArray[inProgram.index].controlSwitchID, 
			controlSwitchToTurnoutMapArray[inProgram.index].trackTurnoutID, 
			controlSwitchToTurnoutMapArray[inProgram.index].invert);

		return true;
	}

	return false;
}

bool
CModule_ControlSwitch::TableWrite(
	int8_t				inSrcNode,
	SMsg_Table const&	inProgram)
{
	if(inProgram.type == eTableType_ControlSwitch)
	{
		if(inProgram.index >= eMaxControlSwitchCount)
		{
			return false;
		}

		controlSwitchArray[inProgram.index].id = inProgram.controlSwitch.id;
		controlSwitchArray[inProgram.index].straightDInPin = inProgram.controlSwitch.straightDInPin;
		controlSwitchArray[inProgram.index].turnDInPin = inProgram.controlSwitch.turnDInPin;

		WriteDataToEEPROM(controlSwitchArray + inProgram.index, eepromOffset + inProgram.index * sizeof(SControlSwitchConfig), sizeof(SControlSwitchConfig));
	}

	else if(inProgram.type == eTableType_ControlSwitchToTurnoutMap)
	{
		if(inProgram.index >= eMaxControlSwitchToTurnoutMapCount)
		{
			return false;
		}

		controlSwitchToTurnoutMapArray[inProgram.index].controlSwitchID = inProgram.trackTurnoutMap.controlSwitchID;
		controlSwitchToTurnoutMapArray[inProgram.index].trackTurnoutID = inProgram.trackTurnoutMap.trackTurnoutID;
		controlSwitchToTurnoutMapArray[inProgram.index].invert = inProgram.trackTurnoutMap.invert;

		WriteDataToEEPROM(controlSwitchToTurnoutMapArray + inProgram.index, eepromOffset + sizeof(controlSwitchArray) + inProgram.index * sizeof(SControlSwitchToTurnoutMapConfig), sizeof(SControlSwitchToTurnoutMapConfig));
	}

	UpdateTables();

	return TableRead(inSrcNode, inProgram);
}

void
CModule_ControlSwitch::UpdateTables(
	void)
{
	for(int i = 0; i < eMaxControlSwitchCount; ++i)
	{
		if(controlSwitchArray[i].id == eInvalidID || controlSwitchArray[i].straightDInPin >= eDIOPinCount || controlSwitchArray[i].turnDInPin >= eDIOPinCount)
		{
			continue;
		}

		pinMode(controlSwitchArray[i].straightDInPin, INPUT_PULLUP);
		pinMode(controlSwitchArray[i].turnDInPin, INPUT_PULLUP);
	}
}
