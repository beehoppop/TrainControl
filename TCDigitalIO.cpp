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

	eUpdateTimeUS = 20000
};

CModule_DigitalIO	gDigitalIO;

CModule_DigitalIO::CModule_DigitalIO(
	)
	:
	CModule("dgio", 0, eUpdateTimeUS, 1)
{
}

void
CModule_DigitalIO::Setup(
	void)
{
	memset(pinInOutMode, 0, sizeof(pinInOutMode));
	memset(pinTime, 0, sizeof(pinTime));
	memset(inputPinLastState, 0, sizeof(inputPinLastState));
	memset(pinActivated, 0, sizeof(pinActivated));
}

void
CModule_DigitalIO::Update(
	uint32_t	inDeltaTimeUS)
{
	for(int i = 0; i < eDIOPinCount; ++i)
	{
		if(pinInOutMode[i] == ePinMode_Input)
		{
			uint8_t	value = digitalReadFast(i);

			switch(inputPinLastState[i])
			{
				case eState_WaitingForChange:
					if(value == 0)
					{
						pinTime[i] = gCurTimeMS;
						inputPinLastState[i] = eState_WaitingForSettle;
					}
					break;

				case eState_WaitingForSettle:
					if(value != 0)
					{
						inputPinLastState[i] = eState_WaitingForChange;
						break;
					}

					if(gCurTimeMS - pinTime[i] >= eSettleTimeMS)
					{
						DebugMsg(eDbgLevel_Verbose, "dio pin %d activated\n", i);
						inputPinLastState[i] = eState_WaitingForDeactive;
						pinActivated[i] = true;
					}
					break;

				case eState_WaitingForDeactive:
					if(value != 0 && pinActivated[i] == false)
					{
						inputPinLastState[i] = eState_WaitingForChange;
					}
					break;
			}
		}
		else if(pinInOutMode[i] == ePinMode_Output)
		{
			if(pinTime[i] > 0 && gCurTimeMS >= pinTime[i])
			{
				pinTime[i] = 0;
				digitalWriteFast(i, 0);
			}
		}
	}
}

void
CModule_DigitalIO::SetPinMode(
	uint8_t			inPin,
	EPinInOutMode	inMode)
{
	MAssert(inPin < eDIOPinCount);

	pinInOutMode[inPin] = inMode;
	if(inMode == ePinMode_Input)
	{
		pinMode(inPin, INPUT_PULLUP);
	}
	else if(inMode == ePinMode_Output)
	{
		pinMode(inPin, OUTPUT);
	}
}

bool
CModule_DigitalIO::CheckInputActivated(
	uint8_t	inPin)
{
	MAssert(inPin < eDIOPinCount);

	bool	result = pinActivated[inPin];

	pinActivated[inPin] = false;

	return result;
}

void
CModule_DigitalIO::SetOutputHighWithTimeout(
	uint8_t		inPin,
	uint32_t	inDurationMS)
{
	digitalWriteFast(inPin, 1);
	pinTime[inPin] = gCurTimeMS + inDurationMS;
}

void
CModule_DigitalIO::SetOutputLow(
	uint8_t	inPin)
{
	digitalWriteFast(inPin, 0);
	pinTime[inPin] = 0;
}
