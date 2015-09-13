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

CModule_Turnout gTurnout;

CModule_Turnout::CModule_Turnout(
	)
	:
	CModule(MMakeUID('t', 'r', 'n', 'o'), sizeof(turnoutConfigArray) + sizeof(turnoutLEDMapConfigArray) + sizeof(turnoutDirectionArray))
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
		if(turnoutConfigArray[i].id >= eMaxTurnoutID || turnoutConfigArray[i].straightDOutPin >= eDIOPinCount || turnoutConfigArray[i].turnDOutPin >= eDIOPinCount)
		{
			continue;
		}

		ActivateTurnout(i, turnoutDirectionArray[i]);
	}
}

void
CModule_Turnout::ControlSwitchTouchedTurnoutID(
	uint16_t	inTurnoutID,
	bool		inTouched,
	bool		inBroadcast)
{
	//DebugMsg(eDbgLevel_Verbose, "Turnout touched %d %d\n", inTurnoutID, inTouched);

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
	uint8_t	tableIndex = turnoutIDToTableIndexMap[inTurnoutID];

	ActivateTurnout(tableIndex, inDirection);
	EEPROM.write(turnoutDirectionOffset + tableIndex, inDirection == eTurnDir_Straight ? eTurnDir_Straight : eTurnDir_Turnout);
}

void
CModule_Turnout::ActivateTurnout(
	uint8_t	inTableIndex,
	uint8_t	inDirection)
{
	if(inDirection == eTurnDir_Straight)
	{
		DebugMsg(eDbgLevel_Basic, "setting %d high\n", turnoutConfigArray[inTableIndex].straightDOutPin);
		gDigitalIO.SetOutputLow(turnoutConfigArray[inTableIndex].turnDOutPin);
		gDigitalIO.SetOutputHighWithTimeout(turnoutConfigArray[inTableIndex].straightDOutPin, eMotorOnTimeMS);
	}
	else
	{
		DebugMsg(eDbgLevel_Basic, "setting %d high\n", turnoutConfigArray[inTableIndex].turnDOutPin);
		gDigitalIO.SetOutputLow(turnoutConfigArray[inTableIndex].straightDOutPin);
		gDigitalIO.SetOutputHighWithTimeout(turnoutConfigArray[inTableIndex].turnDOutPin, eMotorOnTimeMS);
	}

	uint8_t	turnR, turnG;
	uint8_t	straightR, straightG;

	if(inDirection == eTurnDir_Straight)
	{
		turnR = 0xFF;
		turnG = 0;
		straightR = 0;
		straightG = 0xFF;
	}
	else
	{
		turnR = 0;
		turnG = 0xFF;
		straightR = 0xFF;
		straightG = 0;
	}

	STurnoutIDToLEDNumList*	ledNumList = turnoutIDToLEDNumMap + turnoutConfigArray[inTableIndex].id;

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

	for(int i = 0; i < eMaxTrackTurnoutCount; ++i)
	{
		if(turnoutConfigArray[i].id < eMaxTurnoutID)
		{
			turnoutIDToTableIndexMap[turnoutConfigArray[i].id] = i;
			pinMode(turnoutConfigArray[i].straightDOutPin, OUTPUT);
			pinMode(turnoutConfigArray[i].turnDOutPin, OUTPUT);
		}

		if(turnoutLEDMapConfigArray[i].turnoutID < eMaxTurnoutID)
		{
			STurnoutIDToLEDNumList*	ledNumList = turnoutIDToLEDNumMap + turnoutLEDMapConfigArray[i].turnoutID;

			for(int j = 0; j < 2; ++j)
			{
				if(turnoutLEDMapConfigArray[i].straightLEDNum[j] < eMaxLEDCount
					&& turnoutLEDMapConfigArray[i].turnoutLEDNum[j] < eMaxLEDCount)
				{
					if(ledNumList->count >= eMaxTurnoutsPerSwitch)
					{
						DebugMsg(eDbgLevel_Basic, "Too many turnout leds mapped to one turnout\n");
						break;
					}
					ledNumList->straightNumList[ledNumList->count] = turnoutLEDMapConfigArray[i].straightLEDNum[j];
					ledNumList->turnNumList[ledNumList->count] = turnoutLEDMapConfigArray[i].turnoutLEDNum[j];
					++ledNumList->count;
				}
			}
		}
	}

	return true;
}
