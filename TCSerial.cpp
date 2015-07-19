// 
// 
// 

#include "TCCommon.h"
#include "TCModule.h"
#include "TCConfig.h"
#include "TCSerial.h"
#include "TCCANBus.h"
#include "TCAction.h"
#include "TCUtilities.h"
#include "TCState.h"
#include "TCSerial.h"
#include "TCAssert.h"
#include "TCDCCPacket.h"

/*
	Format: [node id] [msg type] [msg specific data]

	[node id] is the numeric node that the msg should be sent to. 255 is broadcast

	[msg type] is one of alive, node_id, control_switch, config_var, state_var, track_sensor, track_turnout, table_write, table_read

	[position] is either straight or turnout

	[table type] is one of control_switch, track_turnout, track_sensor, turnout_map, turnout_led_map

	Formats for each msg type:
		[node id] alive
		[node id] node_id set
		[node id] node_id get
		[node id] control_switch id [position]
		[node id] config_var [config var] [value]
		[node id] state_var [state var] [value]
		[node id] track_sensor id [1 or 0]
		[node id] track_turnout id [position]
		[node id] table_write [table type] [table index] [table specific data]
		[node id] table_read [table type] [table index]

	Formats for each table type:
		[node id] table_write control_switch [table index] [id] [straight pin] [turnout pin]
		[node id] table_write track_turnout [table index] [id] [straight pin] [turnout pin]
		[node id] table_write track_sensor [table index] [id] [pin]
		[node id] table_write turnout_map [table index] [control_switch id] [track_turnout id] [invert]
		[node id] table_write turnout_led_map [table index] [track_turnout id] [straight LED num] [turnout LED num]
*/

class CModule_SerialInput : public CModule
{
public:
	
	CModule_SerialInput(
		)
		:
		CModule(MMakeUID('s', 'r', 'i', 'l'), 0)
	{
	}

	virtual void
	Setup(
		void)
	{
	}

	virtual void
	Update(
		void)
	{
		int	bytesAvailable = Serial.available();
		char	tmpBuffer[256];

		if(bytesAvailable == 0)
		{
			return;
		}

		Serial.readBytes(tmpBuffer, bytesAvailable);

		for(int i = 0; i < bytesAvailable; ++i)
		{
			char c = tmpBuffer[i];

			if(c == '\n')
			{
				charBuffer[curIndex] = 0;
				curIndex = 0;

				if(ProcessSerialMsg(charBuffer) == false)
				{
					Serial.write("Failed\n");
				}
			}
			else
			{
				charBuffer[curIndex++] = c;
			}
		}
	}

	bool
	ProcessSerialMsg_Alive(
		int inNodeID)
	{
		return gAction.Alive(gConfig.GetVal(eConfigVar_NodeID), inNodeID);
	}

	bool
	ProcessSerialMsg_NodeIDWrite(
		int	inNodeID)
	{
		gConfig.SetVal(eConfigVar_NodeID, inNodeID);
		Serial.printf("CC:%d id\n", inNodeID);
		return true;
	}

	bool
	ProcessSerialMsg_NodeIDRead(
		int	inNodeID)
	{
		Serial.printf("CC:%d id\n", gConfig.GetVal(eConfigVar_NodeID));
		return true;
	}

	bool
	ProcessSerialMsg_ControlSwitch(
		int		inNodeID,
		char*	inComponents[])
	{
		SMsg_ControlSwitch	msg;

		if(!IsStrDigit(inComponents[0]))
		{
			Serial.printf("Expecting numeric ID but got \"%s\"\n", inComponents[0]);
			return false;
		}

		if(!IsStrAlpha(inComponents[1]))
		{
			Serial.printf("Expecting position but got \"%s\"\n", inComponents[1]);
			return false;
		}

		msg.timeMS = gCurTimeMS;
		msg.id = atoi(inComponents[0]);
		msg.direction = GetTurnoutDirection(inComponents[1]);
		if(msg.direction == 0xFF)
		{
			return false;
		}

		return gAction.ControlSwitch(gConfig.GetVal(eConfigVar_NodeID), inNodeID, msg);
	}

	bool
	ProcessSerialMsg_ConfigVar(
		int		inNodeID,
		char*	inComponents[])
	{
		SMsg_ConfigVar	msg;

		if(strcmp(inComponents[0], "debug_level") == 0)
		{
			msg.configVar = eConfigVar_DebugLevel;
		}
		else if(strcmp(inComponents[0], "track_waveform_pin") == 0)
		{
			msg.configVar = eConfigVar_TrackWaveformPin;
		}
		else if(strcmp(inComponents[0], "track_power_pin") == 0)
		{
			msg.configVar = eConfigVar_TrackPowerPin;
		}
		else if(strcmp(inComponents[0], "program_waveform_pin") == 0)
		{
			msg.configVar = eConfigVar_ProgramWaveformPin;
		}
		else if(strcmp(inComponents[0], "program_power_pin") == 0)
		{
			msg.configVar = eConfigVar_ProgramPowerPin;
		}
		else if(strcmp(inComponents[0], "program_currentsense_pin") == 0)
		{
			msg.configVar = eConfigVar_ProgramCurrentSensePin;
		}
		else
		{
			Serial.printf("Expecting valid config var but got \"%s\"\n", inComponents[0]);
			return false;
		}

		msg.value = atoi(inComponents[1]);

		return gAction.ConfigVar(gConfig.GetVal(eConfigVar_NodeID), inNodeID, msg);
	}

	bool
	ProcessSerialMsg_StateVar(
		int		inNodeID,
		char*	inComponents[])
	{
		SMsg_StateVar	msg;

		if(strcmp(inComponents[0], "track_power") == 0)
		{
			msg.stateVar = eStateVar_TrackPower;
		}
		else if(strcmp(inComponents[0], "program_power") == 0)
		{
			msg.stateVar = eStateVar_ProgramPower;
		}
		else
		{
			Serial.printf("Expecting valid state var but got \"%s\"\n", inComponents[0]);
			return false;
		}

		msg.value = atoi(inComponents[1]);

		return gAction.StateVar(gConfig.GetVal(eConfigVar_NodeID), inNodeID, msg);
	}

	bool
	ProcessSerialMsg_TrackSensor(
		int		inNodeID,
		char*	inComponents[])
	{
	
		return false;
	}

	bool
	ProcessSerialMsg_TrackTurnout(
		int		inNodeID,
		char*	inComponents[])
	{
		SMsg_TrackTurnout	msg;

		if(!IsStrDigit(inComponents[0]))
		{
			Serial.printf("Expecting numeric ID but got \"%s\"\n", inComponents[0]);
			return false;
		}

		if(!IsStrAlpha(inComponents[1]))
		{
			Serial.printf("Expecting position but got \"%s\"\n", inComponents[1]);
			return false;
		}

		msg.timeMS = gCurTimeMS;
		msg.id = atoi(inComponents[0]);
		msg.direction = GetTurnoutDirection(inComponents[1]);
		if(msg.direction == 0xFF)
		{
			return false;
		}

		return gAction.TrackTurnout(gConfig.GetVal(eConfigVar_NodeID), inNodeID, msg);
	}

	bool
	ProcessSerialMsg_TableWrite(
		int		inNodeID,
		char*	inComponents[])
	{
		if(!IsStrAlpha(inComponents[0]))
		{
			Serial.printf("Expecting alpha table type but got \"%s\"\n", inComponents[0]);
			return false;
		}
	
		if(!IsStrDigit(inComponents[1]))
		{
			Serial.printf("Expecting numeric table index but got \"%s\"\n", inComponents[1]);
			return false;
		}
	
		SMsg_Table	tableMsg;

		tableMsg.index = atoi(inComponents[1]);

		if(strcmp(inComponents[0], "control_switch") == 0)
		{
			// control_switch [table index] [cs id] [straight pin] [turnout pin] 
			if(!IsStrDigit(inComponents[2]))
			{
				Serial.printf("Expecting numeric ID but got \"%s\"\n", inComponents[2]);
				return false;
			}
			if(!IsStrDigit(inComponents[3]))
			{
				Serial.printf("Expecting numeric pin but got \"%s\"\n", inComponents[3]);
				return false;
			}
			if(!IsStrDigit(inComponents[4]))
			{
				Serial.printf("Expecting numeric pin but got \"%s\"\n", inComponents[4]);
				return false;
			}

			tableMsg.type = eTableType_ControlSwitch;
			tableMsg.controlSwitch.id = atoi(inComponents[2]);
			tableMsg.controlSwitch.straightDInPin = atoi(inComponents[3]);
			tableMsg.controlSwitch.turnDInPin = atoi(inComponents[4]);
		}
		else if(strcmp(inComponents[0], "track_turnout") == 0)
		{
			// track_turnout [table index] [cs id] [straight pin] [turnout pin] 
			if(!IsStrDigit(inComponents[2]))
			{
				Serial.printf("Expecting numeric ID but got \"%s\"\n", inComponents[2]);
				return false;
			}
			if(!IsStrDigit(inComponents[3]))
			{
				Serial.printf("Expecting numeric pin but got \"%s\"\n", inComponents[3]);
				return false;
			}
			if(!IsStrDigit(inComponents[4]))
			{
				Serial.printf("Expecting numeric pin but got \"%s\"\n", inComponents[4]);
				return false;
			}

			tableMsg.type = eTableType_TrackTurnout;
			tableMsg.trackTurnout.id = atoi(inComponents[2]);
			tableMsg.trackTurnout.straightDOutPin = atoi(inComponents[3]);
			tableMsg.trackTurnout.turnDOutPin = atoi(inComponents[4]);
		}
		else if(strcmp(inComponents[0], "track_sensor") == 0)
		{
			// track_sensor [table index] [ts id] [pin] 
			if(!IsStrDigit(inComponents[2]) || !IsStrDigit(inComponents[3]))
			{
				return false;
			}

			tableMsg.type = eTableType_TrackSensor;
			tableMsg.trackSensor.id = atoi(inComponents[2]);
			tableMsg.trackSensor.dInPin = atoi(inComponents[3]);
		}
		else if(strcmp(inComponents[0], "turnout_map") == 0)
		{
			// turnout_map [table index] [cs id] [tt id] [invert]
			if(!IsStrDigit(inComponents[2]) || !IsStrDigit(inComponents[3]) || !IsStrDigit(inComponents[4]))
			{
				return false;
			}

			tableMsg.type = eTableType_ControlSwitchToTurnoutMap;
			tableMsg.trackTurnoutMap.controlSwitchID = atoi(inComponents[2]);
			tableMsg.trackTurnoutMap.trackTurnoutID = atoi(inComponents[3]);
			tableMsg.trackTurnoutMap.invert =  atoi(inComponents[4]) ? 1 : 0;
		}
		else if(strcmp(inComponents[0], "turnout_led_map") == 0)
		{
			// turnout_led_map [table index] [tt id] [straight LED num] [turnout LED num]
			if(!IsStrDigit(inComponents[2]) || !IsStrDigit(inComponents[3]) || !IsStrDigit(inComponents[4]))
			{
				return false;
			}

			tableMsg.type = eTableType_TrackTurnoutLEDMap;
			tableMsg.turnoutLED.trackTurnoutID = atoi(inComponents[2]);
			tableMsg.turnoutLED.straightLEDNum = atoi(inComponents[3]);
			tableMsg.turnoutLED.turnoutLEDNum = atoi(inComponents[4]);
		}
		else
		{
			Serial.printf("Expecting valid table type but got \"%s\"\n", inComponents[0]);
			return false;
		}

		return gAction.TableWrite(gConfig.GetVal(eConfigVar_NodeID), inNodeID, tableMsg);
	}

	bool
	ProcessSerialMsg_TableRead(
		int		inNodeID,
		char *	inComponents[])
	{
		if(!IsStrAlpha(inComponents[0]))
		{
			Serial.printf("Expecting alpha table type but got \"%s\"\n", inComponents[0]);
			return false;
		}
	
		if(!IsStrDigit(inComponents[1]))
		{
			Serial.printf("Expecting numeric table index but got \"%s\"\n", inComponents[1]);
			return false;
		}
	
		SMsg_Table	tableMsg;

		tableMsg.index = atoi(inComponents[1]);

		if(strcmp(inComponents[0], "control_switch") == 0)
		{
			tableMsg.type = eTableType_ControlSwitch;
		}
		else if(strcmp(inComponents[0], "track_turnout") == 0)
		{
			tableMsg.type = eTableType_TrackTurnout;
		}
		else if(strcmp(inComponents[0], "track_sensor") == 0)
		{
			tableMsg.type = eTableType_TrackSensor;
		}
		else if(strcmp(inComponents[0], "turnout_map") == 0)
		{
			tableMsg.type = eTableType_ControlSwitchToTurnoutMap;
		}
		else if(strcmp(inComponents[0], "turnout_led_map") == 0)
		{
			tableMsg.type = eTableType_TrackTurnoutLEDMap;
		}
		else
		{
			Serial.printf("Expecting valid table type but got \"%s\"\n", inComponents[0]);
			return false;
		}

		return gAction.TableRead(gConfig.GetVal(eConfigVar_NodeID), inNodeID, tableMsg);
	}


	bool
	ProcessSerialMsg_Throttle(
		int		inNodeID,
		char *	inComponents[])
	{
		uint16_t	address;
		uint16_t	value;

		if(strcmp(inComponents[0], "speed") == 0)
		{
			address = atoi(inComponents[1]);
			value = atoi(inComponents[2]);
			if(address < 256)
			{
				gDCCPacket.StandardSpeed((uint8_t)address, (uint8_t)value);
			}
		}
		else if(strcmp(inComponents[0], "dir") == 0)
		{
			address = atoi(inComponents[1]);
			if(strcmp(inComponents[2], "for") == 0)
			{
				value = 1;
			}
			else if(strcmp(inComponents[2], "rev") == 0)
			{
				value = 0;
			}
			else
			{
				return false;
			}
			if(address < 256)
			{
				gDCCPacket.StandardDirection((uint8_t)address, (uint8_t)value);
			}
		}
		else if(strcmp(inComponents[0], "stop") == 0)
		{
		
		}
		else if(strcmp(inComponents[0], "allstop") == 0)
		{
		
		}
		
		return false;
	}

	bool
	ProcessSerialMsg(
		char*	inStr)
	{
		int strLen = strlen(inStr);
		char*	components[64];
		char*	cp = inStr;
		char*	ep = inStr + strLen;
		int		curCompIndex = 0;

		DebugMsg(eDbgLevel_Verbose, "Serial Str: %s\n", inStr);

		while(cp < ep)
		{
			components[curCompIndex++] = cp;
			cp = strchr(cp, ' ');
			if(cp == NULL)
			{
				break;
			}
			*cp = 0;
			++cp;
		}

		components[curCompIndex] = NULL;

		if(!IsStrDigit(components[0]))
		{
			Serial.printf("expecting node id but got \"%s\"\n", components[0]);
			return false;
		}

		int nodeID = atoi(components[0]);

		if(strcmp(components[1], "alive") == 0)
		{
			return ProcessSerialMsg_Alive(nodeID);
		}
		else if(strcmp(components[1], "node_id") == 0)
		{
			if(strcmp(components[2], "set") == 0)
			{
				return ProcessSerialMsg_NodeIDWrite(nodeID);
			}
			else if(strcmp(components[2], "get") == 0)
			{
				return ProcessSerialMsg_NodeIDRead(nodeID);
			}
		}
		else if(strcmp(components[1], "control_switch") == 0)
		{
			return ProcessSerialMsg_ControlSwitch(nodeID, components + 2);
		}
		else if(strcmp(components[1], "config_var") == 0)
		{
			return ProcessSerialMsg_ConfigVar(nodeID, components + 2);
		}
		else if(strcmp(components[1], "state_var") == 0)
		{
			return ProcessSerialMsg_ConfigVar(nodeID, components + 2);
		}
		else if(strcmp(components[1], "track_sensor") == 0)
		{
			return ProcessSerialMsg_TrackSensor(nodeID, components + 2);
		}
		else if(strcmp(components[1], "track_turnout") == 0)
		{
			return ProcessSerialMsg_TrackTurnout(nodeID, components + 2);
		}
		else if(strcmp(components[1], "table_write") == 0)
		{
			return ProcessSerialMsg_TableWrite(nodeID, components + 2);
		}
		else if(strcmp(components[1], "table_read") == 0)
		{
			return ProcessSerialMsg_TableRead(nodeID, components + 2);
		}
		else if(strcmp(components[1], "throttle") == 0)
		{
			return ProcessSerialMsg_Throttle(nodeID, components + 2);
		}

		Serial.printf("expecting valid command but got \"%s\"\n", components[1]);

		return false;
	}

	char charBuffer[256];
	int	curIndex;
};

static CModule_SerialInput	gSerialInput;
