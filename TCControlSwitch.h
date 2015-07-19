// TCControlSwitch.h

#ifndef _TCCONTROLSWITCH_h
#define _TCCONTROLSWITCH_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TCCommon.h"
#include "TCModule.h"

class CModule_ControlSwitch : public CModule
{
public:
	
	CModule_ControlSwitch(
		);

	void
	Setup(
		void);
	
	void
	Update(
		void);

	void
	ControlSwitchActivated(
		uint16_t	inID,
		uint8_t		inDirection,
		bool		inBroadcast = false);

	bool
	TableRead(
		int8_t				inSrcNode,
		SMsg_Table const&	inProgram);

	bool
	TableWrite(
		int8_t				inSrcNode,
		SMsg_Table const&	inProgram);

private:
	void
	UpdateTables(
		void);

	SControlSwitchConfig				controlSwitchArray[eMaxControlSwitchCount];
	SControlSwitchToTurnoutMapConfig	controlSwitchToTurnoutMapArray[eMaxControlSwitchToTurnoutMapCount];
};

extern CModule_ControlSwitch	gControlSwitch;

#endif

