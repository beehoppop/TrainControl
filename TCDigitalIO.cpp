// 
// 
// 

#include "TCCommon.h"
#include "TCDigitalIO.h"
#include "TCUtilities.h"
#include "TCAssert.h"

enum 
{
	eState_WaitingForChange,
	eState_WaitingForSettle,
	eState_WaitingForDeactive,

	eSettleTimeMS = 50,

	eUpdateTimeUS = 10000
};

CModule_DigitalIO	gDigitalIO;

CModule_DigitalIO::CModule_DigitalIO(
	)
	:
	CModule(MMakeUID('d', 'g', 'i', 'o'), 0, eUpdateTimeUS)
{
	memset(inputPinLastChange, 0, sizeof(inputPinLastChange));
	memset(inputPinLastState, 0, sizeof(inputPinLastState));
	memset(outputPinState, 0, sizeof(outputPinState));
}

void
CModule_DigitalIO::Update(
	uint32_t	inDeltaTimeUS)
{
	for(int i = 0; i < eDIOPinCount; ++i)
	{
		if(outputPinState[i] > 0 && gCurTimeMS >= outputPinState[i])
		{
			outputPinState[i] = 0;
			digitalWriteFast(i, 0);
		}
	}
}

bool
CModule_DigitalIO::CheckInputActivated(
	uint8_t	inPin)
{
	uint8_t	value = digitalReadFast(inPin);

	switch(inputPinLastState[inPin])
	{
		case eState_WaitingForChange:
			if(value == 0)
			{
				inputPinLastChange[inPin] = gCurTimeMS;
				inputPinLastState[inPin] = eState_WaitingForSettle;
			}
			return false;

		case eState_WaitingForSettle:
			if(value != 0)
			{
				inputPinLastState[inPin] = eState_WaitingForChange;
				return false;
			}

			if(gCurTimeMS - inputPinLastChange[inPin] >= eSettleTimeMS)
			{
				DebugMsg(eDbgLevel_Verbose, "dio pin %d activated\n", inPin);
				inputPinLastState[inPin] = eState_WaitingForDeactive;
				return true;
			}
			return false;

		case eState_WaitingForDeactive:
			if(value != 0)
			{
				inputPinLastState[inPin] = eState_WaitingForChange;
			}
			return false;
	}

	return false;
}

void
CModule_DigitalIO::SetOutputHighWithTimeout(
	uint8_t		inPin,
	uint32_t	inDurationMS)
{
	digitalWriteFast(inPin, 1);
	outputPinState[inPin] = gCurTimeMS + inDurationMS;
}

void
CModule_DigitalIO::SetOutputLow(
	uint8_t	inPin)
{
	digitalWriteFast(inPin, 0);
	outputPinState[inPin] = 0;
}
