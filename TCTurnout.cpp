// 
// 
// 

#include <EEPROM.h>

#include "TCUtilities.h"
#include "TCTurnout.h"
#include "TCDigitalIO.h"
#include "TCAction.h"
#include "TCConfig.h"
#include "TCAssert.h"
#include "TCCANBus.h"
#include "TCLED.h"

enum
{
	eMotorOnTimeMS = 6000
};

const float	gTouchPulsesPerSecond = 2.0;
const uint8_t	cHighLEDChannel = 0x40;

CModule_Turnout gTurnout;

CModule_Turnout::CModule_Turnout(
	)
	:
	CModule("trno", sizeof(turnoutConfigArray) + sizeof(turnoutLEDMapConfigArray) + sizeof(turnoutDirectionArray), 20000)
{
	
}

void
CModule_Turnout::Setup(
	void)
{
	turnoutConfigOffset = eepromOffset;
	turnouLEDMapOffset = turnoutConfigOffset + sizeof(turnoutConfigArray);
	turnoutDirectionOffset = turnouLEDMapOffset + sizeof(turnoutLEDMapConfigArray);

	LoadDataFromEEPROM(turnoutConfigArray, turnoutConfigOffset, sizeof(turnoutConfigArray));
	LoadDataFromEEPROM(turnoutLEDMapConfigArray, turnouLEDMapOffset, sizeof(turnoutLEDMapConfigArray));
	LoadDataFromEEPROM(turnoutDirectionArray, turnoutDirectionOffset, sizeof(turnoutDirectionArray));

	SMsg_Table	dummy;
	TableUpdate(0, dummy);

	for(int i = 0; i < eMaxTrackTurnoutCount; ++i)
	{
		if(turnoutConfigArray[i].id < eMaxTurnoutID)
		{
			ActivateTurnout(turnoutConfigArray[i].id, turnoutDirectionArray[i]);
		}
	}

	turnoutTransmitIndex = 0;
}

void
CModule_Turnout::Update(
	uint32_t	inDeltaTimeUS)
{
	// Broadcast the current state of the switches so that other control panels can get updated
	// Only broadcast one turnout at a time to avoid overflowing any CAN bus buffers

	while(turnoutTransmitIndex < eMaxTrackTurnoutCount)
	{
		if(turnoutConfigArray[turnoutTransmitIndex].id < eMaxTurnoutID)
		{
			SMsg_TrackTurnout	turnoutMsg;
			turnoutMsg.timeMS = gCurTimeMS;
			turnoutMsg.id = turnoutConfigArray[turnoutTransmitIndex].id;
			turnoutMsg.direction = turnoutDirectionArray[turnoutTransmitIndex];
			gCANBus.SendMsg(0xFF, eMsgType_TrackTurnout, 0, sizeof(turnoutMsg), &turnoutMsg);
			++turnoutTransmitIndex;
			break;
		}

		++turnoutTransmitIndex;
	}
}

void
CModule_Turnout::ControlSwitchTouchedTurnoutID(
	uint16_t	inTurnoutID,
	bool		inTouched,
	bool		inBroadcast)
{
	DebugMsg(eDbgLevel_Verbose, "TO: touched %d %d\n", inTurnoutID, inTouched);

	if(inTurnoutID >= eMaxTurnoutID)
	{
		return;
	}

	if(inBroadcast)
	{
		SMsg_TurnoutControlSwitchTouch	msg;
		//SendSerialMsg(gNodeID, "CC:%d cs touched id=%d touch=%d\n", gNodeID, inID, inTouched);
		msg.timeMS = gCurTimeMS;
		msg.turnoutID = inTurnoutID;
		msg.touched = (uint8_t)inTouched;
		gCANBus.SendMsg(0xFF, eMsgType_TurnoutControlSwitchTouch, 0, sizeof(msg), &msg);
	}

	STurnoutIDToLEDNumList*	ledNumList = turnoutIDToLEDNumMap + inTurnoutID;

	for(int i = 0; i < ledNumList->count; ++i)
	{
		gLED.PulseOnOff(
			ledNumList->straightNumList[i],
			gTouchPulsesPerSecond,
			inTouched);

		gLED.PulseOnOff(
			ledNumList->turnNumList[i],
			gTouchPulsesPerSecond,
			inTouched);
	}
}

void
CModule_Turnout::SetTurnoutDirection(
	uint16_t	inTurnoutID,
	uint8_t		inDirection)
{
	if(inTurnoutID >= eMaxTurnoutID)
	{
		return;
	}

	ActivateTurnout(inTurnoutID, inDirection);

	uint8_t	tableIndex = turnoutIDToTableIndexMap[inTurnoutID];

	if(tableIndex < eMaxTrackTurnoutCount && inDirection != turnoutDirectionArray[tableIndex])
	{
		turnoutDirectionArray[tableIndex] = inDirection == eTurnDir_Straight ? eTurnDir_Straight : eTurnDir_Turnout;
		EEPROM.write(turnoutDirectionOffset + tableIndex, turnoutDirectionArray[tableIndex]);
	}
}

void
CModule_Turnout::ActivateTurnout(
	uint8_t	inTurnoutID,
	uint8_t	inDirection)
{
	uint8_t	tableIndex = turnoutIDToTableIndexMap[inTurnoutID];

	DebugMsg(eDbgLevel_Basic, "TO: id %d to %s\n", inTurnoutID, inDirection == eTurnDir_Straight ? "straight" : "turn");

	if(tableIndex < eMaxTrackTurnoutCount)
	{
		if(inDirection == eTurnDir_Straight)
		{
			DebugMsg(eDbgLevel_Basic, "TO: setting pin %d high\n", turnoutConfigArray[tableIndex].straightDOutPin);
			if(turnoutConfigArray[tableIndex].turnDOutPin < eDIOPinCount && turnoutConfigArray[tableIndex].straightDOutPin < eDIOPinCount)
			{
				gDigitalIO.SetOutputLow(turnoutConfigArray[tableIndex].turnDOutPin);
				gDigitalIO.SetOutputHighWithTimeout(turnoutConfigArray[tableIndex].straightDOutPin, eMotorOnTimeMS);
			}
		}
		else
		{
			DebugMsg(eDbgLevel_Basic, "TO: setting pin %d high\n", turnoutConfigArray[tableIndex].turnDOutPin);
			if(turnoutConfigArray[tableIndex].turnDOutPin < eDIOPinCount && turnoutConfigArray[tableIndex].straightDOutPin < eDIOPinCount)
			{
				gDigitalIO.SetOutputLow(turnoutConfigArray[tableIndex].straightDOutPin);
				gDigitalIO.SetOutputHighWithTimeout(turnoutConfigArray[tableIndex].turnDOutPin, eMotorOnTimeMS);
			}
		}
	}

	uint8_t	turnR, turnG;
	uint8_t	straightR, straightG;

	if(inDirection == eTurnDir_Straight)
	{
		turnR = cHighLEDChannel;
		turnG = 0;
		straightR = 0;
		straightG = cHighLEDChannel;
	}
	else
	{
		turnR = 0;
		turnG = cHighLEDChannel;
		straightR = cHighLEDChannel;
		straightG = 0;
	}

	STurnoutIDToLEDNumList*	ledNumList = turnoutIDToLEDNumMap + inTurnoutID;

	for(int i = 0; i < ledNumList->count; ++i)
	{
		gLED.SetColor(
			ledNumList->straightNumList[i],
			straightR,
			straightG,
			0,
			eMotorOnTimeMS - 1000);

		gLED.SetColor(
			ledNumList->turnNumList[i],
			turnR,
			turnG,
			0,
			eMotorOnTimeMS - 1000);
	}
}

bool
CModule_Turnout::TableRead(
	int8_t				inSrcNode,
	SMsg_Table const&	inProgram)
{
	if(inProgram.type == eTableType_TrackTurnout)
	{
		gAction.SendSerial(
			inSrcNode,
			"CC:%d track_turnout index=%d id=%d straightPin=%d turnPin=%d curState=%s\n", 
			gConfig.GetVal(eConfigVar_NodeID), 
			inProgram.index, 
			turnoutConfigArray[inProgram.index].id, 
			turnoutConfigArray[inProgram.index].straightDOutPin, 
			turnoutConfigArray[inProgram.index].turnDOutPin,
			turnoutDirectionArray[inProgram.index] == eTurnDir_Straight ? "straight" : "turnout");

		return true;
	}

	if(inProgram.type == eTableType_TrackTurnoutLEDMap)
	{
		gAction.SendSerial(
			inSrcNode,
			"CC:%d track_turnout_led_map index=%d turnout_id=%d straightLEDNum[0]=%d turnoutLEDNum[0]=%d straightLEDNum[1]=%d turnoutLEDNum[1]=%d\n", 
			gConfig.GetVal(eConfigVar_NodeID), 
			inProgram.index, 
			turnoutLEDMapConfigArray[inProgram.index].turnoutID, 
			turnoutLEDMapConfigArray[inProgram.index].straightLEDNum[0],
			turnoutLEDMapConfigArray[inProgram.index].turnoutLEDNum[0],
			turnoutLEDMapConfigArray[inProgram.index].straightLEDNum[1],
			turnoutLEDMapConfigArray[inProgram.index].turnoutLEDNum[1]);

		return true;
	}

	return false;
}

bool
CModule_Turnout::TableWrite(
	int8_t				inSrcNode,
	SMsg_Table const&	inProgram)
{
	if(inProgram.type == eTableType_TrackTurnout)
	{
		turnoutConfigArray[inProgram.index].id = inProgram.trackTurnout.id;
		turnoutConfigArray[inProgram.index].straightDOutPin = inProgram.trackTurnout.straightDOutPin;
		turnoutConfigArray[inProgram.index].turnDOutPin = inProgram.trackTurnout.turnDOutPin;

		WriteDataToEEPROM(turnoutConfigArray + inProgram.index, turnoutConfigOffset + inProgram.index * sizeof(STrackTurnoutConfig), sizeof(STrackTurnoutConfig));
	}

	else if(inProgram.type == eTableType_TrackTurnoutLEDMap)
	{
		turnoutLEDMapConfigArray[inProgram.index].turnoutID = inProgram.trackTurnoutLEDMap.turnoutID;
		turnoutLEDMapConfigArray[inProgram.index].straightLEDNum[0] = inProgram.trackTurnoutLEDMap.straightLEDNum[0];
		turnoutLEDMapConfigArray[inProgram.index].turnoutLEDNum[0] = inProgram.trackTurnoutLEDMap.turnoutLEDNum[0];
		turnoutLEDMapConfigArray[inProgram.index].straightLEDNum[1] = inProgram.trackTurnoutLEDMap.straightLEDNum[1];
		turnoutLEDMapConfigArray[inProgram.index].turnoutLEDNum[1] = inProgram.trackTurnoutLEDMap.turnoutLEDNum[1];

		WriteDataToEEPROM(turnoutLEDMapConfigArray + inProgram.index, turnouLEDMapOffset + inProgram.index * sizeof(STrackTurnoutLEDMapConfig), sizeof(STrackTurnoutLEDMapConfig));
	}

	return TableRead(inSrcNode, inProgram);
}

bool
CModule_Turnout::TableUpdate(
	int8_t				inSrcNode,
	SMsg_Table const&	inProgram)
{
	memset(turnoutIDToLEDNumMap, 0, sizeof(turnoutIDToLEDNumMap));
	memset(turnoutIDToTableIndexMap, 0xFF, sizeof(turnoutIDToTableIndexMap));

	int	numLEDs = gConfig.GetVal(eConfigVar_LEDCount);

	for(int i = 0; i < eMaxTrackTurnoutCount; ++i)
	{
		if(turnoutConfigArray[i].id < eMaxTurnoutID)
		{
			turnoutIDToTableIndexMap[turnoutConfigArray[i].id] = i;
			if(turnoutConfigArray[i].straightDOutPin < eDIOPinCount && turnoutConfigArray[i].turnDOutPin < eDIOPinCount)
			{
				gDigitalIO.SetPinMode(turnoutConfigArray[i].straightDOutPin, ePinMode_Output);
				gDigitalIO.SetPinMode(turnoutConfigArray[i].turnDOutPin, ePinMode_Output);
			}
		}

		if(turnoutLEDMapConfigArray[i].turnoutID < eMaxTurnoutID)
		{
			STurnoutIDToLEDNumList*	ledNumList = turnoutIDToLEDNumMap + turnoutLEDMapConfigArray[i].turnoutID;

			for(int j = 0; j < 2; ++j)
			{
				if(turnoutLEDMapConfigArray[i].straightLEDNum[j] < numLEDs
					&& turnoutLEDMapConfigArray[i].turnoutLEDNum[j] < numLEDs)
				{
					if(ledNumList->count >= eMaxTurnoutsPerSwitch)
					{
						DebugMsg(eDbgLevel_Basic, "TO: Too many turnout leds mapped to one turnout\n");
						break;
					}
					ledNumList->straightNumList[ledNumList->count] = turnoutLEDMapConfigArray[i].straightLEDNum[j];
					ledNumList->turnNumList[ledNumList->count] = turnoutLEDMapConfigArray[i].turnoutLEDNum[j];
					++ledNumList->count;
				}
			}
		}
	}

	gAction.SendSerial(
		inSrcNode,
		"CC:%d table_update turnout\n", 
		gConfig.GetVal(eConfigVar_NodeID));

	return true;
}
