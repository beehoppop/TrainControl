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
#include "TCDCC.h"
#include "TCControlSwitch.h"
#include "TCTurnout.h"
#include "TCLED.h"

/*
	Format: [msg type] [msg specific data]

	[msg type] is one of alive, node_id, control_switch, config_var, state_var, track_sensor, track_turnout, table_write, table_read


	Formats for each msg type:
		alive [node id]
		node_id set [node id]
		node_id get
		control_switch id [position]
		track_sensor id [1 or 0]
		track_turnout id [position]
		config_var set|get [node id] [config var] [value]
		state_var [node id] [state var] [value]
		table_write [table type] [node id] [table index] [table specific data]
		table_read [table type] [node id] [table index]
		table_update [table type] [node id]
		led id [led num] on|off
		dcc_command [command id] power [on|off]
		dcc_command [command id] allstop
		dcc_command [command id] dir [address] [forward|reverse]
		dcc_command [command id] speed [address] [speed]
		dcc_command [command id] stop [address]

		dcc_command [command id] mode [ops|service]

	Formats for each table write type:
		table_write control_switch [node id] [table index] [id] [straight pin] [turnout pin] [touch id]
		table_write track_turnout [node id] [table index] [id] [straight pin] [turnout pin]
		table_write track_turnout_led_map [node id] [table index] [id] [led num straight 1] [led num turnout 1] [led num straight 2] [led num turnout 2]
		table_write track_sensor [node id] [table index] [id] [pin]
		table_write turnout_map [node id] [table index] [control_switch id] [track_turnout id 1] [track_turnout id 2]
		table_write dcc_command [node id] [table index] [command id] [waveform pin] [power pin] [current sense pin]

	[node id] is the numeric node that the msg should be sent to. 255 is broadcast

	[position] is either straight or turnout

	[table type] is one of control_switch, track_turnout, track_sensor, turnout_map, turnout_led_map

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
		uint32_t	inDeltaTimeUS)
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
					Serial.write("Command Failed\n");
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
		char*	inComponents[])
	{
		int nodeID = atoi(inComponents[0]);

		return gAction.Alive(gConfig.GetVal(eConfigVar_NodeID), nodeID);
	}

	bool
	ProcessSerialMsg_NodeID(
		char*	inComponents[])
	{
		if(strcmp(inComponents[0], "set") == 0)
		{
			int nodeID = atoi(inComponents[1]);
			gConfig.SetVal(eConfigVar_NodeID, nodeID);
			Serial.printf("CC:%d id\n", nodeID);
			return true;
		}
		else if(strcmp(inComponents[0], "get") == 0)
		{
			Serial.printf("CC:%d id\n", gConfig.GetVal(eConfigVar_NodeID));
			return true;
		}
		else
		{
			return false;
		}
	}

	bool
	ProcessSerialMsg_ControlSwitch(
		char*	inComponents[])
	{
		SMsg_ControlSwitch	msg;

		if(!IsStrDigit(inComponents[0]))
		{
			Serial.printf("Expecting numeric ID but got \"%s\"\n", inComponents[0]);
			return false;
		}

		msg.timeMS = gCurTimeMS;
		msg.id = atoi(inComponents[0]);
		msg.direction = GetTurnoutDirection(inComponents[1]);
		if(msg.direction == 0xFF)
		{
			Serial.printf("Expecting position but got \"%s\"\n", inComponents[1]);
			return false;
		}
		gControlSwitch.ControlSwitchActivated(msg.id, msg.direction);
		gCANBus.SendMsg(0xFF, eMsgType_ControlSwitch, 0, sizeof(msg), &msg);

		return true;
	}

	bool
	ProcessSerialMsg_ConfigVar(
		char*	inComponents[])
	{
		SMsg_ConfigVar	msg;

		if(strcmp(inComponents[2], "debug_level") == 0)
		{
			msg.configVar = eConfigVar_DebugLevel;
		}
		else if(strcmp(inComponents[2], "led_count") == 0)
		{
			msg.configVar = eConfigVar_LEDCount;
		}
		else
		{
			Serial.printf("Expecting valid config var but got \"%s\"\n", inComponents[1]);
			return false;
		}

		int nodeID = atoi(inComponents[1]);
		msg.value = atoi(inComponents[3]);

		if(strcmp(inComponents[0], "set") == 0)
		{
			msg.setVar = 1;
			if(nodeID == gConfig.GetVal(eConfigVar_NodeID))
			{
				gConfig.SetVal(msg.configVar, msg.value);
				Serial.printf(
					"CC:%d config_var set %d %d\n", 
					nodeID,
					msg.configVar,
					msg.value);
			}
			else
			{
				gCANBus.SendMsg(nodeID, eMsgType_ConfigVar, 0, sizeof(msg), &msg);
			}
		}
		else if(strcmp(inComponents[0], "get") == 0)
		{
			msg.setVar = 0;
			if(nodeID == gConfig.GetVal(eConfigVar_NodeID))
			{
				uint8_t val = gConfig.GetVal(msg.configVar);
				Serial.printf(
					"CC:%d config_var get %d %d\n", 
					nodeID,
					msg.configVar,
					val);
			}
			else
			{
				gCANBus.SendMsg(nodeID, eMsgType_ConfigVar, 0, sizeof(msg), &msg);
			}
		}
		else
		{
			return false;
		}

		return true;
	}


	bool
	ProcessSerialMsg_LED(
		char*	inComponents[])
	{
		if(strcmp(inComponents[0], "id") == 0)
		{
			int ledNum = atoi(inComponents[1]);

			if(strcmp(inComponents[2], "on") == 0)
			{
				gLED.SetColor(ledNum, 0xff, 0xff, 0xff, 1000.0f);
				gLED.PulseOnOff(ledNum, 4.0, true);
			}
			else if(strcmp(inComponents[2], "off") == 0)
			{
				gLED.SetColor(ledNum, 0, 0, 0, 1000.0f);
				gLED.PulseOnOff(ledNum, 4.0, false);
			}
			else
			{
				Serial.printf("invalid led command, expected on or off \"%s\"\n", inComponents[2]);
				return false;
			}
		}
		else
		{
			Serial.printf("invalid led command \"%s\"\n", inComponents[0]);
			return false;
		}

		return true;
	}

	bool
	ProcessSerialMsg_StateVar(
		char*	inComponents[])
	{
		SMsg_StateVar	msg;

		if(strcmp(inComponents[1], "some_state_var") == 0)
		{
			//msg.stateVar = eStateVar_SomeVar;
		}
		else
		{
			Serial.printf("Expecting valid state var but got \"%s\"\n", inComponents[1]);
			return false;
		}

		int nodeID = atoi(inComponents[0]);
		msg.value = atoi(inComponents[2]);

		if(nodeID == gConfig.GetVal(eConfigVar_NodeID))
		{
			gConfig.SetVal(msg.stateVar, msg.value);
		}
		else
		{
			gCANBus.SendMsg(nodeID, eMsgType_StateVar, 0, sizeof(msg), &msg);
		}

		return true;
	}

	bool
	ProcessSerialMsg_TrackSensor(
		char*	inComponents[])
	{
	
		return false;
	}

	bool
	ProcessSerialMsg_TrackTurnout(
		char*	inComponents[])
	{
		SMsg_TrackTurnout	msg;

		if(!IsStrDigit(inComponents[0]))
		{
			Serial.printf("Expecting numeric ID but got \"%s\"\n", inComponents[0]);
			return false;
		}

		msg.timeMS = gCurTimeMS;
		msg.id = atoi(inComponents[0]);
		msg.direction = GetTurnoutDirection(inComponents[1]);
		if(msg.direction == 0xFF)
		{
			Serial.printf("Expecting position but got \"%s\"\n", inComponents[1]);
			return false;
		}
		gTurnout.SetTurnoutDirection(msg.id, msg.direction);
		gCANBus.SendMsg(0xFF, eMsgType_TrackTurnout, 0, sizeof(msg), &msg);

		return true;
	}

	bool
	ProcessSerialMsg_TableWrite(
		char*	inComponents[])
	{
		if(!IsStrAlpha(inComponents[0]))
		{
			Serial.printf("Expecting alpha table type but got \"%s\"\n", inComponents[0]);
			return false;
		}
	
		if(!IsStrDigit(inComponents[1]))
		{
			Serial.printf("Expecting numeric node id but got \"%s\"\n", inComponents[1]);
			return false;
		}
	
		if(!IsStrDigit(inComponents[2]))
		{
			Serial.printf("Expecting numeric table index but got \"%s\"\n", inComponents[2]);
			return false;
		}
	
		int nodeID = atoi(inComponents[1]);

		SMsg_Table	tableMsg;

		tableMsg.index = atoi(inComponents[2]);

		if(strcmp(inComponents[0], "control_switch") == 0)
		{
			// control_switch [table index] [cs id] [straight pin] [turnout pin] [touch id]
			if(!IsStrDigit(inComponents[3]))
			{
				Serial.printf("Expecting numeric ID but got \"%s\"\n", inComponents[3]);
				return false;
			}
			if(!IsStrDigit(inComponents[4]))
			{
				Serial.printf("Expecting numeric pin but got \"%s\"\n", inComponents[4]);
				return false;
			}
			if(!IsStrDigit(inComponents[5]))
			{
				Serial.printf("Expecting numeric pin but got \"%s\"\n", inComponents[5]);
				return false;
			}
			if(!IsStrDigit(inComponents[6]))
			{
				Serial.printf("Expecting numeric pin but got \"%s\"\n", inComponents[5]);
				return false;
			}

			tableMsg.type = eTableType_ControlSwitch;
			tableMsg.controlSwitch.id = atoi(inComponents[3]);
			tableMsg.controlSwitch.straightDInPin = atoi(inComponents[4]);
			tableMsg.controlSwitch.turnDInPin = atoi(inComponents[5]);
			tableMsg.controlSwitch.touchID = atoi(inComponents[6]);
		}
		else if(strcmp(inComponents[0], "track_turnout") == 0)
		{
			// track_turnout [table index] [cs id] [straight pin] [turnout pin] 
			if(!IsStrDigit(inComponents[3]))
			{
				Serial.printf("Expecting numeric ID but got \"%s\"\n", inComponents[3]);
				return false;
			}
			if(!IsStrDigit(inComponents[4]))
			{
				Serial.printf("Expecting numeric pin but got \"%s\"\n", inComponents[4]);
				return false;
			}
			if(!IsStrDigit(inComponents[5]))
			{
				Serial.printf("Expecting numeric pin but got \"%s\"\n", inComponents[5]);
				return false;
			}

			tableMsg.type = eTableType_TrackTurnout;
			tableMsg.trackTurnout.id = atoi(inComponents[3]);
			tableMsg.trackTurnout.straightDOutPin = atoi(inComponents[4]);
			tableMsg.trackTurnout.turnDOutPin = atoi(inComponents[5]);
		}
		else if(strcmp(inComponents[0], "track_sensor") == 0)
		{
			// track_sensor [table index] [ts id] [pin] 
			if(!IsStrDigit(inComponents[3]) || !IsStrDigit(inComponents[4]))
			{
				return false;
			}

			tableMsg.type = eTableType_TrackSensor;
			tableMsg.trackSensor.id = atoi(inComponents[3]);
			tableMsg.trackSensor.dInPin = atoi(inComponents[4]);
		}
		else if(strcmp(inComponents[0], "turnout_map") == 0)
		{
			// turnout_map [table index] [cs id] [tt id] [invert]
			if(!IsStrDigit(inComponents[3]) || !IsStrDigit(inComponents[4]) || !IsStrDigit(inComponents[5]))
			{
				return false;
			}

			tableMsg.type = eTableType_ControlSwitchToTurnoutMap;
			tableMsg.trackTurnoutMap.controlSwitchID = atoi(inComponents[3]);
			tableMsg.trackTurnoutMap.turnout1ID = atoi(inComponents[4]);
			tableMsg.trackTurnoutMap.turnout2ID =  atoi(inComponents[5]);
		}
		else if(strcmp(inComponents[0], "track_turnout_led_map") == 0)
		{
			// track_turnout_led_map [table index] [id] [led num straight 1] [led num turnout 1] [led num straight 2] [led num turnout 2]
			if(!IsStrDigit(inComponents[3]) || !IsStrDigit(inComponents[4]) || !IsStrDigit(inComponents[5]) || !IsStrDigit(inComponents[6]) || !IsStrDigit(inComponents[7]))
			{
				return false;
			}

			tableMsg.type = eTableType_TrackTurnoutLEDMap;
			tableMsg.trackTurnoutLEDMap.turnoutID = atoi(inComponents[3]);
			tableMsg.trackTurnoutLEDMap.straightLEDNum[0] = atoi(inComponents[4]);
			tableMsg.trackTurnoutLEDMap.turnoutLEDNum[0] = atoi(inComponents[5]);
			tableMsg.trackTurnoutLEDMap.straightLEDNum[1] = atoi(inComponents[6]);
			tableMsg.trackTurnoutLEDMap.turnoutLEDNum[1] = atoi(inComponents[7]);
		}
		else if(strcmp(inComponents[0], "dcc_command") == 0)
		{
			// dcc_command [node id] [table index] [command id]
			if(!IsStrDigit(inComponents[3]))
			{
				return false;
			}

			tableMsg.type = eTableType_DCC;
			tableMsg.dccCommand.commandID = atoi(inComponents[3]);
			tableMsg.dccCommand.waveformPin = atoi(inComponents[4]);
			tableMsg.dccCommand.powerPin = atoi(inComponents[5]);
			tableMsg.dccCommand.currentSensePin = atoi(inComponents[6]);
		}
		else
		{
			Serial.printf("Expecting valid table type but got \"%s\"\n", inComponents[0]);
			return false;
		}

		return gAction.TableWrite(gConfig.GetVal(eConfigVar_NodeID), nodeID, tableMsg);
	}

	bool
	ProcessSerialMsg_TableRead(
		char *	inComponents[])
	{
		if(!IsStrAlpha(inComponents[0]))
		{
			Serial.printf("Expecting alpha table type but got \"%s\"\n", inComponents[0]);
			return false;
		}
	
		if(!IsStrDigit(inComponents[1]))
		{
			Serial.printf("Expecting numeric node id but got \"%s\"\n", inComponents[1]);
			return false;
		}
	
		if(!IsStrDigit(inComponents[2]))
		{
			Serial.printf("Expecting numeric table index but got \"%s\"\n", inComponents[2]);
			return false;
		}
	
		int nodeID = atoi(inComponents[1]);

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
		else if(strcmp(inComponents[0], "dcc_command") == 0)
		{
			tableMsg.type = eTableType_DCC;
		}
		else
		{
			Serial.printf("Expecting valid table type but got \"%s\"\n", inComponents[0]);
			return false;
		}

		return gAction.TableRead(gConfig.GetVal(eConfigVar_NodeID), nodeID, tableMsg);
	}

	bool
	ProcessSerialMsg_TableUpdate(
		char *	inComponents[])
	{
		if(!IsStrAlpha(inComponents[0]))
		{
			Serial.printf("Expecting alpha table type but got \"%s\"\n", inComponents[0]);
			return false;
		}
	
		if(!IsStrDigit(inComponents[1]))
		{
			Serial.printf("Expecting numeric node id but got \"%s\"\n", inComponents[1]);
			return false;
		}
	
		int nodeID = atoi(inComponents[1]);

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
		else if(strcmp(inComponents[0], "dcc_command") == 0)
		{
			tableMsg.type = eTableType_DCC;
		}
		else
		{
			Serial.printf("Expecting valid table type but got \"%s\"\n", inComponents[0]);
			return false;
		}

		return gAction.TableUpdate(gConfig.GetVal(eConfigVar_NodeID), nodeID, tableMsg);
	}

	bool
	ProcessSerialMsg_DCCCommand(
		char *	inComponents[])
	{
		uint16_t	address;
		uint16_t	value;

		if(!IsStrDigit(inComponents[0]))
		{
			Serial.printf("Expecting numeric command buffer ID but got \"%s\"\n", inComponents[0]);
			return false;
		}

		int	commandID = atoi(inComponents[0]);

		if(strcmp(inComponents[1], "power") == 0)
		{
			if(strcmp(inComponents[2], "on") == 0)
			{
				gDCC.SetPowerState(commandID, true);
				return true;
			}
			else if(strcmp(inComponents[2], "off") == 0)
			{
				gDCC.SetPowerState(commandID, false);
				return true;
			}
		}
		else if(strcmp(inComponents[1], "allstop") == 0)
		{
		
		}
		else if(strcmp(inComponents[1], "dir") == 0)
		{
			address = atoi(inComponents[2]);
			if(strcmp(inComponents[3], "for") == 0)
			{
				value = 1;
			}
			else if(strcmp(inComponents[3], "rev") == 0)
			{
				value = 0;
			}
			else
			{
				return false;
			}
			if(address < 256)
			{
				gDCC.StandardDirection(commandID, (uint8_t)address, (uint8_t)value);
			}
		}
		else if(strcmp(inComponents[1], "speed") == 0)
		{
			address = atoi(inComponents[2]);
			value = atoi(inComponents[3]);
			if(address < 256)
			{
				gDCC.StandardSpeed(commandID, (uint8_t)address, (uint8_t)value);
			}
		}
		else if(strcmp(inComponents[1], "stop") == 0)
		{
		
		}
		else if(strcmp(inComponents[1], "mode") == 0)
		{
			if(strcmp(inComponents[2], "ops") == 0)
			{
				gDCC.SetOpsMode(commandID, true);
				return true;
			}
			else if(strcmp(inComponents[2], "service") == 0)
			{
				gDCC.SetOpsMode(commandID, false);
				return true;
			}
		}
		else if(strcmp(inComponents[1], "program") == 0)
		{

		}
		
		return false;
	}

	void
	ProcessSerialMsg_RestartCommand(
		char *	inComponents[])
	{
		int nodeID = atoi(inComponents[0]);

		if(nodeID == gConfig.GetVal(eConfigVar_NodeID))
		{
			CPU_RESTART();
		}
		else
		{
			gCANBus.SendMsg(nodeID, eMsgType_Restart, 0, 0, NULL);
		}
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

		DebugMsg(eDbgLevel_Verbose, "Serial: Received %s\n", inStr);

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

		if(!IsStrAlpha(components[0]))
		{
			Serial.printf("expecting string but got \"%s\"\n", components[0]);
			return false;
		}

		if(strcmp(components[0], "alive") == 0)
		{
			return ProcessSerialMsg_Alive(components + 1);
		}
		else if(strcmp(components[0], "node_id") == 0)
		{
			return ProcessSerialMsg_NodeID(components + 1);
		}
		else if(strcmp(components[0], "control_switch") == 0)
		{
			return ProcessSerialMsg_ControlSwitch(components + 1);
		}
		else if(strcmp(components[0], "config_var") == 0)
		{
			return ProcessSerialMsg_ConfigVar(components + 1);
		}
		else if(strcmp(components[0], "led") == 0)
		{
			return ProcessSerialMsg_LED(components + 1);
		}
		else if(strcmp(components[0], "state_var") == 0)
		{
			return ProcessSerialMsg_StateVar(components + 1);
		}
		else if(strcmp(components[0], "track_sensor") == 0)
		{
			return ProcessSerialMsg_TrackSensor(components + 1);
		}
		else if(strcmp(components[0], "track_turnout") == 0)
		{
			return ProcessSerialMsg_TrackTurnout(components + 1);
		}
		else if(strcmp(components[0], "table_write") == 0)
		{
			return ProcessSerialMsg_TableWrite(components + 1);
		}
		else if(strcmp(components[0], "table_read") == 0)
		{
			return ProcessSerialMsg_TableRead(components + 1);
		}
		else if(strcmp(components[0], "table_update") == 0)
		{
			return ProcessSerialMsg_TableUpdate(components + 1);
		}
		else if(strcmp(components[0], "dcc_command") == 0)
		{
			return ProcessSerialMsg_DCCCommand(components + 1);
		}
		else if(strcmp(components[0], "restart") == 0)
		{
			ProcessSerialMsg_RestartCommand(components + 1);
		}

		Serial.printf("expecting valid command but got \"%s\"\n", components[0]);

		return false;
	}

	char charBuffer[256];
	int	curIndex;
};

static CModule_SerialInput	gSerialInput;
