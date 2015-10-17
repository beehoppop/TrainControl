
#include "TCAssert.h"
#include "TCModule.h"
#include "TCUtilities.h"
#include "TCTeensyTouch.h"

enum
{
	eMaxInterfaces = 4,

	ePinCount = 32,
};

struct SSensorInterface
{
	ITouchSensor*	touchSensor;
};

class CModule_TeensyTouch : public CModule
{
public:
	
	CModule_TeensyTouch(
		);

	void
	Setup(
		void);
	
	void
	Update(
		uint32_t	inDeltaTimeUS);
};

static int					gInterfaceCount;
static SSensorInterface		gInterfaceList[eMaxInterfaces];
static CModule_TeensyTouch	gModuleMPR121;
static uint8_t				gTouchState[ePinCount];
static uint32_t				gLastTouchTime[ePinCount];
static uint32_t				gTouchInputBV;

enum 
{
	eState_WaitingForChangeToTouched,
	eState_WaitingForTouchSettle,
	eState_WaitingForChangeToRelease,
	eState_WaitingForReleaseSettle,

	eUpdateTimeUS = 50000,
	eSettleTimeMS = 200,

	eTouchThreshold = 2000,
};

CModule_TeensyTouch::CModule_TeensyTouch(
	)
	:
	CModule("tytc", 0, eUpdateTimeUS)
{

}

void
CModule_TeensyTouch::Setup(
	void)
{

}

void
CModule_TeensyTouch::Update(
	uint32_t	inDeltaTimeUS)
{
	for(int itr = 0; itr < ePinCount; ++itr)
	{
		if(!(gTouchInputBV & (1 << itr)))
		{
			continue;
		}

		int	readValue = touchRead(itr);
		//DebugMsg(eDbgLevel_Basic, "TTch: %d %d\n", itr, readValue);

		bool touched = readValue > eTouchThreshold;

		switch(gTouchState[itr])
		{
			case eState_WaitingForChangeToTouched:
				if(touched)
				{
					gLastTouchTime[itr] = gCurTimeMS;
					gTouchState[itr] = eState_WaitingForTouchSettle;
				}
				break;

			case eState_WaitingForTouchSettle:
				if(!touched)
				{
					gTouchState[itr] = eState_WaitingForChangeToTouched;
					break;
				}

				if(gCurTimeMS - gLastTouchTime[itr] >= eSettleTimeMS)
				{
					gTouchState[itr] = eState_WaitingForChangeToRelease;
					DebugMsg(eDbgLevel_Verbose, "TTch: Touched %d\n", itr);
					for(int itr2 = 0; itr2 < gInterfaceCount; ++itr2)
					{
						gInterfaceList[itr2].touchSensor->Touch(itr);
					}
				}
				break;

			case eState_WaitingForChangeToRelease:
				if(!touched)
				{
					//DebugMsg(eDbgLevel_Verbose, "TeensyTouch: Going to release settle %d\n", itr);
					gLastTouchTime[itr] = gCurTimeMS;
					gTouchState[itr] = eState_WaitingForReleaseSettle;
				}
				break;

			case eState_WaitingForReleaseSettle:
				if(touched)
				{
					//DebugMsg(eDbgLevel_Verbose, "MPR121: Going to release waiting %d\n", itr);
					gTouchState[itr] = eState_WaitingForChangeToRelease;
					break;
				}

				if(gCurTimeMS - gLastTouchTime[itr] >= eSettleTimeMS)
				{
					gTouchState[itr] = eState_WaitingForChangeToTouched;
					DebugMsg(eDbgLevel_Verbose, "TTch: Release %d\n", itr);
					for(int itr2 = 0; itr2 < gInterfaceCount; ++itr2)
					{
						gInterfaceList[itr2].touchSensor->Release(itr);
					}
				}
				break;
		}
	}
}

void
TeensyRegisterTouchSensor(
	uint32_t		inTouchInputBV,
	ITouchSensor*	inTouchSensor)
{
	MAssert(gInterfaceCount < eMaxInterfaces);

	gTouchInputBV |= inTouchInputBV;

	for(int i = 0; i < gInterfaceCount; ++i)
	{
		if(gInterfaceList[i].touchSensor == inTouchSensor)
		{
			// Don't re register
			return;
		}
	}

	SSensorInterface*	newInterface = gInterfaceList + gInterfaceCount++;
	newInterface->touchSensor = inTouchSensor;
}

void
TeensyUnRegisterTouchSensor(
	ITouchSensor*	inTouchSensor)
{
	for(int i = 0; i < gInterfaceCount; ++i)
	{
		if(gInterfaceList[i].touchSensor == inTouchSensor)
		{
			memmove(gInterfaceList + i, gInterfaceList + i + 1, (gInterfaceCount - i - 1) * sizeof(SSensorInterface));
			--gInterfaceCount;
			return;
		}
	}
}

void
TeensyAddTouchInputs(
	uint32_t		inTouchInputBV)
{
	gTouchInputBV |= inTouchInputBV;
}

void
TeensyRemoveTouchInputs(
	uint32_t		inTouchInputBV)
{
	gTouchInputBV &= ~inTouchInputBV;
}

