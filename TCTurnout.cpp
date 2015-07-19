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

enum
{
	eMotorOnTimeMS = 6000
};

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
	turnoutLEDMapConfigOffset = turnoutConfigOffset + sizeof(turnoutConfigArray);
	turnoutDirectionOffset = turnoutLEDMapConfigOffset + sizeof(turnoutLEDMapConfigArray);

	LoadDataFromEEPROM(turnoutConfigArray, turnoutConfigOffset, sizeof(turnoutConfigArray));
	LoadDataFromEEPROM(turnoutLEDMapConfigArray, turnoutLEDMapConfigOffset, sizeof(turnoutLEDMapConfigArray));
	LoadDataFromEEPROM(turnoutDirectionArray, turnoutDirectionOffset, sizeof(turnoutDirectionArray));

	for(int i = 0; i < eMaxTrackTurnoutCount; ++i)
	{
		if(turnoutConfigArray[i].id == eInvalidID || turnoutConfigArray[i].straightDOutPin >= eDIOPinCount || turnoutConfigArray[i].turnDOutPin >= eDIOPinCount)
		{
			continue;
		}

		pinMode(turnoutConfigArray[i].straightDOutPin, OUTPUT);
		pinMode(turnoutConfigArray[i].turnDOutPin, OUTPUT);

		if(turnoutDirectionArray[i] == eTurnDir_Straight)
		{
			DebugMsg(eDbgLevel_Basic, "setting %d high\n", turnoutConfigArray[i].straightDOutPin);
			gDigitalIO.SetOutputLow(turnoutConfigArray[i].turnDOutPin);
			gDigitalIO.SetOutputHighWithTimeout(turnoutConfigArray[i].straightDOutPin, eMotorOnTimeMS);
		}
		else
		{
			DebugMsg(eDbgLevel_Basic, "setting %d high\n", turnoutConfigArray[i].turnDOutPin);
			gDigitalIO.SetOutputLow(turnoutConfigArray[i].straightDOutPin);
			gDigitalIO.SetOutputHighWithTimeout(turnoutConfigArray[i].turnDOutPin, eMotorOnTimeMS);
		}
	}
}

void
CModule_Turnout::SetTurnoutDirection(
	uint8_t	inID,
	uint8_t	inDirection)
{
	for(int i = 0; i < eMaxTrackTurnoutCount; ++i)
	{
		if(turnoutConfigArray[i].id == inID)
		{
			if(inDirection == eTurnDir_Straight)
			{
				DebugMsg(eDbgLevel_Basic, "setting %d high\n", turnoutConfigArray[i].straightDOutPin);
				gDigitalIO.SetOutputLow(turnoutConfigArray[i].turnDOutPin);
				gDigitalIO.SetOutputHighWithTimeout(turnoutConfigArray[i].straightDOutPin, eMotorOnTimeMS);
			}
			else
			{
				DebugMsg(eDbgLevel_Basic, "setting %d high\n", turnoutConfigArray[i].turnDOutPin);
				gDigitalIO.SetOutputLow(turnoutConfigArray[i].straightDOutPin);
				gDigitalIO.SetOutputHighWithTimeout(turnoutConfigArray[i].turnDOutPin, eMotorOnTimeMS);
			}

			EEPROM.write(turnoutDirectionOffset + i, inDirection == eTurnDir_Straight ? eTurnDir_Straight : eTurnDir_Turnout);

			return;
		}
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
			"CC:%d track_turnout_ledmap index=%d turnout_id=%d straightLEDNum=%d turnLEDNum=%d\n", 
			gConfig.GetVal(eConfigVar_NodeID),
			inProgram.index,
			turnoutLEDMapConfigArray[inProgram.index].trackTurnoutID,
			turnoutLEDMapConfigArray[inProgram.index].straightLEDNum,
			turnoutLEDMapConfigArray[inProgram.index].turnoutLEDNum);

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

		pinMode(inProgram.trackTurnout.straightDOutPin, OUTPUT);
		pinMode(inProgram.trackTurnout.turnDOutPin, OUTPUT);

		WriteDataToEEPROM(turnoutConfigArray + inProgram.index, turnoutConfigOffset + inProgram.index * sizeof(STrackTurnoutConfig), sizeof(STrackTurnoutConfig));
	}

	if(inProgram.type == eTableType_TrackTurnoutLEDMap)
	{
		turnoutLEDMapConfigArray[inProgram.index].trackTurnoutID = inProgram.turnoutLED.trackTurnoutID;
		turnoutLEDMapConfigArray[inProgram.index].straightLEDNum = inProgram.turnoutLED.straightLEDNum;
		turnoutLEDMapConfigArray[inProgram.index].turnoutLEDNum = inProgram.turnoutLED.turnoutLEDNum;

		WriteDataToEEPROM(turnoutConfigArray + inProgram.index, turnoutConfigOffset + inProgram.index * sizeof(STrackTurnoutConfig), sizeof(STrackTurnoutConfig));
	}

	return TableRead(inSrcNode, inProgram);
}
