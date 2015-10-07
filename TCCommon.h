#ifndef _TCCOMMON_H_
#define _TCCOMMON_H_

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART() (*CPU_RESTART_ADDR = CPU_RESTART_VAL)

enum
{
	eMaxControlSwitchCount = 12,
	eMaxTrackTurnoutCount = 16,
	eMaxControlSwitchToTurnoutMapCount = 64,

	eMaxControlSwitchID = 512,
	eMaxTurnoutID		= 512,

	eMaxTurnoutsPerSwitch	= 8,
	eMaxLEDsPerTurnout		= 4,

	eInvalidID = 0xFFFF,

	eEEPROMVersion = 9,

	eTurnDir_Straight = 0,
	eTurnDir_Turnout,

	eMsgType_Alive = 1,
	eMsgType_SerialOut,
	eMsgType_TableWrite,
	eMsgType_TableRead,
	eMsgType_TableUpdate,
	eMsgType_TrackSensor,
	eMsgType_TrackTurnout,
	eMsgType_ControlSwitch,
	eMsgType_TurnoutControlSwitchTouch,
	eMsgType_StateVar,
	eMsgType_ConfigVar,
	eMsgType_SoftRestart,
	eMsgType_ResetAllState,

	eMsgType_DCCTrack,
	eMsgType_DCCThrottle,
	eMsgType_DCCService,

	eTableType_ControlSwitch = 1,
	eTableType_TrackTurnout,
	eTableType_TrackTurnoutLEDMap,
	eTableType_TrackSensor,
	eTableType_ControlSwitchToTurnoutMap,
	eTableType_DCC,
	

	eDCCThrottle_Power = 1,
	eDCCThrottle_EmergencyAllStop,
	eDCCThrottle_ResetDecoder,
	eDCCThrottle_EmergencyStop,
	eDCCThrottle_SetSpeedAndDirection,

	eDIOPinCount = 34,
};

struct SControlSwitchConfig
{
	uint16_t	id;
	uint8_t		straightDInPin;
	uint8_t		turnDInPin;
	uint8_t		touchID;
};

struct STrackTurnoutConfig
{
	uint16_t	id;
	uint8_t		straightDOutPin;
	uint8_t		turnDOutPin;
};

struct STrackTurnoutLEDMapConfig
{
	uint16_t	turnoutID;
	uint8_t		straightLEDNum[2];
	uint8_t		turnoutLEDNum[2];
};

struct STrackSensorConfig
{
	uint16_t	id;
	uint8_t		dInPin;
};

struct SControlSwitchToTurnoutMapConfig
{
	uint16_t	controlSwitchID;
	uint16_t	turnout1ID;
	uint16_t	turnout2ID;
};

struct SMsg_DCCCommandConfig
{
	uint16_t	commandID;
	uint8_t		waveformPin;
	uint8_t		powerPin;
	uint8_t		currentSensePin;
};

struct SMsg_ControlSwitch
{
	uint32_t	timeMS;
	uint16_t	id;
	uint8_t		direction;
};

struct SMsg_TrackTurnout
{
	uint32_t	timeMS;
	uint16_t	id;
	uint8_t		direction;
};

struct SMsg_TurnoutControlSwitchTouch
{
	uint32_t	timeMS;
	uint16_t	turnoutID;
	uint8_t		touched;	// 1 is touch, 0 is release
};

struct SMsg_TrackSesnor
{
	uint32_t	timeMS;
	uint16_t	id;
	uint16_t	trigger;
};

struct SMsg_Table
{
	uint8_t	type;
	uint8_t	index;

	union
	{
		SControlSwitchConfig				controlSwitch;
		STrackTurnoutConfig					trackTurnout;
		STrackSensorConfig					trackSensor;
		SControlSwitchToTurnoutMapConfig	trackTurnoutMap;
		STrackTurnoutLEDMapConfig			trackTurnoutLEDMap;
		SMsg_DCCCommandConfig				dccCommand;
	};
};

struct SMsg_ConfigVar
{
	uint8_t	setVar;		// 1 is set, 0 is get
	uint8_t	configVar;
	uint8_t	value;
};

struct SMsg_StateVar
{
	uint8_t	stateVar;
	uint8_t	value;
};

struct SMsg_DCCCommand
{
	uint16_t	address;
	uint8_t		type;
	uint8_t		data;
};

struct SMsg_DCCServiceCommand
{
	uint8_t	type;
	uint8_t	data;
};

class ITouchSensor
{
public:

	virtual void
	Touch(
		int	inTouchID) = 0;

	virtual void
	Release(
		int	inTouchID) = 0;

};

extern char const*	gVersionStr;

#endif /* _TCCOMMON_H_ */
