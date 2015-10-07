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
#include "TCMPR121.h"
#include "TCLED.h"
#include "TCAssert.h"
#include "TCTeensyTouch.h"

#define TOU_THRESH	0x06
#define	REL_THRESH	0x0A

enum
{
	eUpdateTimeUS = 50000,
	eDownDownDelayMS = 1000
};

CModule_ControlSwitch	gControlSwitch;

CModule_ControlSwitch::CModule_ControlSwitch(
	)
	:
	CModule(MMakeUID('c', 'n', 's', 'w'), sizeof(controlSwitchArray) + sizeof(controlSwitchToTurnoutMapArray), eUpdateTimeUS)
{

}

void
CModule_ControlSwitch::Setup(
	void)
{
	usingBuiltInTouch = gConfig.GetVal(eConfigVar_BuiltInTouch) != 0;

	LoadDataFromEEPROM(controlSwitchArray, eepromOffset, sizeof(controlSwitchArray));
	LoadDataFromEEPROM(controlSwitchToTurnoutMapArray, eepromOffset + sizeof(controlSwitchArray), sizeof(controlSwitchToTurnoutMapArray));

	SMsg_Table	dummy;
	TableUpdate(0, dummy);

	if(usingBuiltInTouch)
	{
		uint32_t	touchBV = 0;

		for(int i = 0; i < eMaxControlSwitchCount; ++i)
		{
			if(controlSwitchArray[i].touchID != 0xFF)
			{
				touchBV |= 1 << controlSwitchArray[i].touchID;
			}
		}

		TeensyRegisterTouchSensor(touchBV, this);
	}
	else
	{
		MPRRegisterTouchSensor(0, this);
	}
}

void
CModule_ControlSwitch::TearDown(
	void)
{
	if(usingBuiltInTouch)
	{
		TeensyUnRegisterTouchSensor(this);
	}
	else
	{
		MPRUnRegisterTouchSensor(0, this);
	}
}

void
CModule_ControlSwitch::Update(
	uint32_t	inDeltaTimeUS)
{
	for(int i = 0; i < eMaxControlSwitchCount; ++i)
	{
		if(controlSwitchArray[i].id >= eMaxControlSwitchID || controlSwitchArray[i].straightDInPin >= eDIOPinCount || controlSwitchArray[i].turnDInPin >= eDIOPinCount)
			continue;

		if(gDigitalIO.CheckInputActivated(controlSwitchArray[i].straightDInPin))
		{
			ControlSwitchActivated(controlSwitchArray[i].id, eTurnDir_Straight, true);
			state[i].touchDown = false;
		}
		else if(gDigitalIO.CheckInputActivated(controlSwitchArray[i].turnDInPin))
		{
			ControlSwitchActivated(controlSwitchArray[i].id, eTurnDir_Turnout, true);
			state[i].touchDown = false;
		}

		if(!state[i].touchActivated && state[i].touchDown && gCurTimeMS - state[i].touchDownTimeMS > eDownDownDelayMS)
		{
			state[i].touchActivated = true;
			ControlSwitchTouchedID(controlSwitchArray[i].id, true, true);
		}
		else if(state[i].touchActivated && !state[i].touchDown)
		{
			state[i].touchActivated = false;
			ControlSwitchTouchedID(controlSwitchArray[i].id, false, true);
		}
	}
}

void
CModule_ControlSwitch::Touch(
	int	inTouchID)
{
	uint8_t	index = touchIDToControlSwitchIndexMap[inTouchID];

	DebugMsg(eDbgLevel_Verbose, "CS: Touch cs id %d index %d\n", inTouchID, index);

	if(index < eMaxControlSwitchCount)
	{
		state[index].touchDown = true;
		state[index].touchDownTimeMS = gCurTimeMS;
	}
}

void
CModule_ControlSwitch::Release(
	int	inTouchID)
{
	uint8_t	index = touchIDToControlSwitchIndexMap[inTouchID];

	DebugMsg(eDbgLevel_Verbose, "CS: Release cs id %d index %d\n", inTouchID, index);

	if(index < eMaxControlSwitchCount)
	{
		state[index].touchDown = false;
		state[index].touchDownTimeMS = 0;
	}
}
	
void
CModule_ControlSwitch::ControlSwitchTouchedID(
	uint16_t	inControlSwitchID,
	bool		inTouched,
	bool		inBroadcast)
{
	if(inControlSwitchID >= eMaxControlSwitchID)
	{
		return;
	}

	SControlSwitchToTurnoutIDList*	turnoutIDList = controlSwitchIDToTurnoutIDMap + inControlSwitchID;

	for(int i = 0; i < turnoutIDList->count; ++i)
	{
		gTurnout.ControlSwitchTouchedTurnoutID(turnoutIDList->turnoutIDList[i], inTouched, inBroadcast);
	}
}

void
CModule_ControlSwitch::ControlSwitchActivated(
	uint16_t	inControlSwitchID,
	uint8_t		inDirection,
	bool		inBroadcast)
{
	DebugMsg(eDbgLevel_Basic, "CS: act %d %s\n", inControlSwitchID, inDirection == eTurnDir_Straight ? "straight" : "turn");

	if(inControlSwitchID >= eMaxControlSwitchID)
	{
		return;
	}

	if(inBroadcast)
	{
		SMsg_ControlSwitch	msg;
		//SendSerialMsg(gNodeID, "CC:%d cs activated id=%d dir=%s\n", gNodeID, inID, inTurnDirection == eTurnDir_Straight ? "straight" : "turnout");
		msg.timeMS = gCurTimeMS;
		msg.id = inControlSwitchID;
		msg.direction = inDirection;
		gCANBus.SendMsg(0xFF, eMsgType_ControlSwitch, 0, sizeof(msg), &msg);
	}

	SControlSwitchToTurnoutIDList*	turnoutIDList = controlSwitchIDToTurnoutIDMap + inControlSwitchID;

	for(int i = 0; i < turnoutIDList->count; ++i)
	{
		gTurnout.SetTurnoutDirection(turnoutIDList->turnoutIDList[i], inDirection);
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
			"CC:%d control_switch index=%d id=%d straightPin=%d turnPin=%d touchID=%d\n",
			gConfig.GetVal(eConfigVar_NodeID), 
			inProgram.index, 
			controlSwitchArray[inProgram.index].id, 
			controlSwitchArray[inProgram.index].straightDInPin, 
			controlSwitchArray[inProgram.index].turnDInPin,
			controlSwitchArray[inProgram.index].touchID);

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
			"CC:%d turnout_map index=%d cs_id=%d tt_id1=%d tt_id2=%d\n", 
			gConfig.GetVal(eConfigVar_NodeID), 
			inProgram.index, 
			controlSwitchToTurnoutMapArray[inProgram.index].controlSwitchID, 
			controlSwitchToTurnoutMapArray[inProgram.index].turnout1ID, 
			controlSwitchToTurnoutMapArray[inProgram.index].turnout2ID);

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
		controlSwitchArray[inProgram.index].touchID = inProgram.controlSwitch.touchID;

		WriteDataToEEPROM(controlSwitchArray + inProgram.index, eepromOffset + inProgram.index * sizeof(SControlSwitchConfig), sizeof(SControlSwitchConfig));
	}

	else if(inProgram.type == eTableType_ControlSwitchToTurnoutMap)
	{
		if(inProgram.index >= eMaxControlSwitchToTurnoutMapCount)
		{
			return false;
		}

		controlSwitchToTurnoutMapArray[inProgram.index].controlSwitchID = inProgram.trackTurnoutMap.controlSwitchID;
		controlSwitchToTurnoutMapArray[inProgram.index].turnout1ID = inProgram.trackTurnoutMap.turnout1ID;
		controlSwitchToTurnoutMapArray[inProgram.index].turnout2ID = inProgram.trackTurnoutMap.turnout2ID;

		WriteDataToEEPROM(controlSwitchToTurnoutMapArray + inProgram.index, eepromOffset + sizeof(controlSwitchArray) + inProgram.index * sizeof(SControlSwitchToTurnoutMapConfig), sizeof(SControlSwitchToTurnoutMapConfig));
	}

	return TableRead(inSrcNode, inProgram);
}

bool
CModule_ControlSwitch::TableUpdate(
	int8_t				inSrcNode,
	SMsg_Table const&	inProgram)
{
	memset(touchIDToControlSwitchIndexMap, 0xFF, sizeof(touchIDToControlSwitchIndexMap));
	memset(controlSwitchIDToTurnoutIDMap, 0, sizeof(controlSwitchIDToTurnoutIDMap));

	for(int i = 0; i < eMaxControlSwitchCount; ++i)
	{
		if(controlSwitchArray[i].id >= eMaxControlSwitchID || controlSwitchArray[i].straightDInPin >= eDIOPinCount || controlSwitchArray[i].turnDInPin >= eDIOPinCount)
		{
			continue;
		}

		if(controlSwitchArray[i].touchID < eDIOPinCount)
		{
			touchIDToControlSwitchIndexMap[controlSwitchArray[i].touchID] = i;
		}

		pinMode(controlSwitchArray[i].straightDInPin, INPUT_PULLUP);
		pinMode(controlSwitchArray[i].turnDInPin, INPUT_PULLUP);
	}

	for(int i = 0; i < eMaxControlSwitchToTurnoutMapCount; ++i)
	{
		if(controlSwitchToTurnoutMapArray[i].controlSwitchID >= eMaxControlSwitchID)
		{
			continue;
		}

		SControlSwitchToTurnoutIDList*	csToTurnoutIDList = controlSwitchIDToTurnoutIDMap + controlSwitchToTurnoutMapArray[i].controlSwitchID;

		if(controlSwitchToTurnoutMapArray[i].turnout1ID < eMaxTurnoutID)
		{
			if(csToTurnoutIDList->count >= eMaxTurnoutsPerSwitch)
			{
				DebugMsg(eDbgLevel_Basic, "CS: Too many turnouts mapped to one switch\n");
				continue;
			}
			csToTurnoutIDList->turnoutIDList[csToTurnoutIDList->count++] = controlSwitchToTurnoutMapArray[i].turnout1ID;
		}

		if(controlSwitchToTurnoutMapArray[i].turnout2ID < eMaxTurnoutID)
		{
			if(csToTurnoutIDList->count >= eMaxTurnoutsPerSwitch)
			{
				DebugMsg(eDbgLevel_Basic, "CS: Too many turnouts mapped to one switch\n");
				continue;
			}
			csToTurnoutIDList->turnoutIDList[csToTurnoutIDList->count++] = controlSwitchToTurnoutMapArray[i].turnout2ID;
		}
	}

	gAction.SendSerial(
		inSrcNode,
		"CC:%d table_update control_switch\n", 
		gConfig.GetVal(eConfigVar_NodeID));

	return true;
}
