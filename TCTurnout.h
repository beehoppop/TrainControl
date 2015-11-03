// TCTurnout.h

#ifndef _TCTURNOUT_h
#define _TCTURNOUT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TCCommon.h"
#include "TCModule.h"

class CModule_Turnout : public CModule
{
public:
	
	CModule_Turnout(
		);

	virtual void
	Setup(
		void);

	virtual void
	Update(
		uint32_t	inDeltaTimeUS);
	
	void
	SetTurnoutDirection(
		uint16_t	inTurnoutID,
		uint8_t		inDirection);
	
	void
	ControlSwitchTouchedTurnoutID(
		uint16_t	inTurnoutID,
		bool		inTouched,
		bool		inBroadcast);

	bool
	TableRead(
		int8_t				inSrcNode,
		SMsg_Table const&	inProgram);

	bool
	TableWrite(
		int8_t				inSrcNode,
		SMsg_Table const&	inProgram);

	bool
	TableUpdate(
		int8_t				inSrcNode,
		SMsg_Table const&	inProgram);

private:

	struct STurnoutIDToLEDNumList
	{
		uint8_t	count;
		uint8_t	straightNumList[eMaxLEDsPerTurnout];
		uint8_t	turnNumList[eMaxLEDsPerTurnout];
	};

	void
	ActivateTurnout(
		uint8_t	inTurnoutID,
		uint8_t	inDirection);

	STrackTurnoutConfig			turnoutConfigArray[eMaxTrackTurnoutCount];
	STrackTurnoutLEDMapConfig	turnoutLEDMapConfigArray[eMaxTrackTurnoutCount];
	uint8_t						turnoutDirectionArray[eMaxTrackTurnoutCount];

	uint16_t	turnoutConfigOffset;
	uint16_t	turnoutDirectionOffset;
	uint16_t	turnouLEDMapOffset;

	STurnoutIDToLEDNumList	turnoutIDToLEDNumMap[eMaxTurnoutID];
	uint8_t					turnoutIDToTableIndexMap[eMaxTurnoutID];

	uint8_t	turnoutTransmitIndex;
};

extern CModule_Turnout gTurnout;

#endif

