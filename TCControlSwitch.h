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
#include "TCMPR121.h"

class CModule_ControlSwitch : public CModule, public ITouchSensor
{
public:
	
	CModule_ControlSwitch(
		);

	void
	Setup(
		void);

	void
	TearDown(
		void);
	
	void
	Update(
		uint32_t	inDeltaTimeUS);

	virtual void
	Touch(
		int	inTouchID);

	virtual void
	Release(
		int	inTouchID);

	void
	ControlSwitchActivated(
		uint16_t	inControlSwitchID,
		uint8_t		inDirection,
		bool		inBroadcast = false);
	
	void
	ControlSwitchTouchedID(
		uint16_t	inControlSwitchID,
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
	
	struct SControlSwitchToTurnoutIDList
	{
		uint8_t		count;
		uint16_t	turnoutIDList[eMaxTurnoutsPerSwitch];
	};

	struct SCSState
	{
		bool		touchDown;
		bool		touchActivated;
		uint32_t	touchDownTimeMS;
	};

	bool								touchMode;
	SControlSwitchConfig				controlSwitchArray[eMaxControlSwitchCount];
	SControlSwitchToTurnoutMapConfig	controlSwitchToTurnoutMapArray[eMaxControlSwitchToTurnoutMapCount];
	uint8_t								touchIDToControlSwitchIndexMap[eDIOPinCount];
	SControlSwitchToTurnoutIDList		controlSwitchIDToTurnoutIDMap[eMaxControlSwitchID];
	SCSState							state[eMaxControlSwitchCount];
};

extern CModule_ControlSwitch	gControlSwitch;

#endif
