#ifndef _TCCOMMON_H_
#define _TCCOMMON_H_

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

enum
{
	eMaxControlSwitchCount = 16,
	eMaxTrackTurnoutCount = 16,
	eMaxControlSwitchToTurnoutMapCount = 64,

	eInvalidID = 0xFFFF,

	eEEPROMVersion = 6,

	eTurnDir_Straight = 0,
	eTurnDir_Turnout,

	eMsgType_Alive = 1,
	eMsgType_SerialOut,
	eMsgType_TableWrite,
	eMsgType_TableRead,
	eMsgType_TrackSensor,
	eMsgType_TrackTurnout,
	eMsgType_ControlSwitch,
	eMsgType_StateVar,
	eMsgType_ConfigVar,
	eMsgType_Throttle,

	eTableType_ControlSwitch = 1,
	eTableType_TrackTurnout,
	eTableType_TrackSensor,
	eTableType_ControlSwitchToTurnoutMap,
	eTableType_TrackTurnoutLEDMap,
	
	eThrottle_EmergencyAllStop,
	eThrottle_ResetDecoder,
	eThrottle_EmergencyStop,
	eThrottle_SetSpeedAndDirection,

	eDIOPinCount = 34,

};

struct SControlSwitchConfig
{
	uint16_t	id;
	uint8_t		straightDInPin;
	uint8_t		turnDInPin;
};

struct STrackTurnoutConfig
{
	uint16_t	id;
	uint8_t		straightDOutPin;
	uint8_t		turnDOutPin;
};

struct STrackSensorConfig
{
	uint16_t	id;
	uint8_t		dInPin;
};

struct SControlSwitchToTurnoutMapConfig
{
	uint16_t	controlSwitchID;
	uint16_t	trackTurnoutID;
	uint8_t		invert;
};

struct STrackTurnoutLEDMapConfig
{
	uint16_t	trackTurnoutID;
	uint16_t	straightLEDNum;
	uint16_t	turnoutLEDNum;
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
		SControlSwitchConfig		controlSwitch;
		STrackTurnoutConfig			trackTurnout;
		STrackSensorConfig			trackSensor;
		SControlSwitchToTurnoutMapConfig		trackTurnoutMap;
		STrackTurnoutLEDMapConfig	turnoutLED;
	};
};

struct SMsg_ConfigVar
{
	uint8_t	configVar;
	uint8_t	value;
};

struct SMsg_StateVar
{
	uint8_t	stateVar;
	uint8_t	value;
};

struct SMsg_Throttle
{
	uint16_t	address;
	uint8_t		type;
	uint8_t		data;
};

extern uint32_t		gCurTimeMS;
extern char const*	gVersionStr;

#endif /* _TCCOMMON_H_ */
