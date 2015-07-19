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

	void
	Setup(
		void);
	
	void
	SetTurnoutDirection(
		uint8_t	inID,
		uint8_t	inDirection);

	bool
	TableRead(
		int8_t				inSrcNode,
		SMsg_Table const&	inProgram);

	bool
	TableWrite(
		int8_t				inSrcNode,
		SMsg_Table const&	inProgram);

private:
	STrackTurnoutConfig			turnoutConfigArray[eMaxTrackTurnoutCount];
	STrackTurnoutLEDMapConfig	turnoutLEDMapConfigArray[eMaxTrackTurnoutCount];
	uint8_t						turnoutDirectionArray[eMaxTrackTurnoutCount];

	uint16_t	turnoutConfigOffset;
	uint16_t	turnoutLEDMapConfigOffset;
	uint16_t	turnoutDirectionOffset;
};

extern CModule_Turnout gTurnout;

#endif

