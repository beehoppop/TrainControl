// 
// 
// 
#include <IntervalTimer.h>

#include "TCAssert.h"
#include "TCCommon.h"
#include "TCUtilities.h"
#include "TCConfig.h"
#include "TCState.h"
#include "TCAction.h"
#include "TCDCC.h"

enum
{
	eDCC_WaveformPeriod = 58,

	ePreambleBitCount = 12,

	eState_Preamble = 0,
	eState_StartBit,
	eState_Byte,
	eState_EndPacket,
};

CModule_DCC gDCC;

CModule_DCC::CModule_DCC(
	)
	:
	CModule(MMakeUID('d', 'c', 'c', 't'), sizeof(commandConfigList))
{

}

void
CModule_DCC::Setup(
	void)
{
return;

	SMsg_Table	dummy;
	TableUpdate(0, dummy);

	for(int i = 0; i < eMaxCommandStations; ++i)
	{
		commandStateList[i].Reset();
	}
}

void
CModule_DCC::Update(
	uint32_t	inDeltaTimeUS)
{

}

void
CModule_DCC::SetPowerState(
	uint8_t	inCommandID,	// 0xFF means all command stations
	bool	inPowerOn)
{
	uint8_t	index = FindIndexFromID(inCommandID);

	if(index >= eMaxCommandStations)
	{
		return;
	}

	if(inPowerOn)
	{
		commandStateList[inCommandID].Reset();
		digitalWriteFast(commandConfigList[inCommandID].powerPin, 0);
		masterTimer.begin(UpdateWaveform, eDCC_WaveformPeriod);
	}
	else
	{
		masterTimer.end();
		digitalWriteFast(commandConfigList[inCommandID].powerPin, 1);
	}
}

void
CModule_DCC::EmergencyAllStop(
	uint16_t	inCommandID)
{

}

void
CModule_DCC::ResetAllDecoders(
	uint16_t	inCommandID)
{

}

void
CModule_DCC::EmergencyStop(
	uint16_t	inCommandID,
	uint8_t		inAddress)
{

}
	
void
CModule_DCC::StandardDirection(
	uint16_t	inCommandID,
	uint8_t		inAddress,
	uint8_t		inDirection)
{
	uint8_t	index = FindIndexFromID(inCommandID);

	if(index >= eMaxCommandStations)
	{
		return;
	}

	SCommandState*	cs = commandStateList + index;
	SDecoder*		decoder = cs->FindDecoder(inAddress);

	if(decoder == NULL)
	{
		return;
	}

	decoder->direction = inDirection;
	cs->SendStandardSpeedAndDirectionPacket(decoder);
}
	
void
CModule_DCC::StandardSpeed(
	uint16_t	inCommandID,
	uint8_t		inAddress,
	uint8_t		inSpeed)
{
	uint8_t	index = FindIndexFromID(inCommandID);

	if(index >= eMaxCommandStations)
	{
		return;
	}

	SCommandState*	cs = commandStateList + index;
	SDecoder*		decoder = cs->FindDecoder(inAddress);

	if(decoder == NULL)
	{
		return;
	}

	decoder->speed = inSpeed;
	cs->SendStandardSpeedAndDirectionPacket(decoder);
}

void
CModule_DCC::SetOpsMode(
	uint8_t			inTrackID,
	bool			inOpsMode)
{

}

bool
CModule_DCC::TableRead(
	int8_t				inSrcNode,
	SMsg_Table const&	inProgram)
{
	if(inProgram.type == eTableType_DCC)
	{
		gAction.SendSerial(
			inSrcNode,
			"CC:%d dcc_command index=%d id=%d powerPin=%d waveformPin=%d currentSensePin=%d\n", 
			gConfig.GetVal(eConfigVar_NodeID), 
			inProgram.index, 
			commandConfigList[inProgram.index].commandID, 
			commandConfigList[inProgram.index].powerPin, 
			commandConfigList[inProgram.index].waveformPin,
			commandConfigList[inProgram.index].currentSensePin);

		return true;
	}

	return false;
}

bool
CModule_DCC::TableWrite(
	int8_t				inSrcNode,
	SMsg_Table const&	inProgram)
{
	if(inProgram.type == eTableType_DCC)
	{
		commandConfigList[inProgram.index].commandID = inProgram.dccCommand.commandID;
		commandConfigList[inProgram.index].currentSensePin = inProgram.dccCommand.currentSensePin;
		commandConfigList[inProgram.index].powerPin = inProgram.dccCommand.powerPin;
		commandConfigList[inProgram.index].waveformPin = inProgram.dccCommand.waveformPin;

		WriteDataToEEPROM(commandConfigList + inProgram.index, eepromOffset + inProgram.index * sizeof(SMsg_DCCCommandConfig), sizeof(SMsg_DCCCommandConfig));

		return TableRead(inSrcNode, inProgram);
	}

	return false;
}

bool
CModule_DCC::TableUpdate(
	int8_t				inSrcNode,
	SMsg_Table const&	inProgram)
{
	for(int i = 0; i < eMaxCommandStations; ++i)
	{
		if(commandConfigList[i].commandID != eInvalidID && commandConfigList[i].powerPin < eDIOPinCount && commandConfigList[i].waveformPin < eDIOPinCount)
		{
			pinMode(commandConfigList[i].powerPin, OUTPUT);
			pinMode(commandConfigList[i].waveformPin, OUTPUT);
			digitalWriteFast(commandConfigList[i].powerPin, 1);
			digitalWriteFast(commandConfigList[i].waveformPin, 0);
		}
	}

	return true;
}

void
CModule_DCC::SCommandState::Reset(
	void)
{
	curPacketIndex = 0;
	curByteIndex = 0;
	curBitIndex = 0;
	curPhase = 0;
	nextPacketIndex = 0;

	curPreambleBitCount = ePreambleBitCount;
	curState = eState_Preamble;

	memset(transmitBuffer, 0, sizeof(transmitBuffer));
}

bool
CModule_DCC::SCommandState::AddPacket(
	uint8_t			inSize,
	uint8_t const*	inData)
{
	uint8_t	targetIndex;

	for(int i = 0; i < eMaxPacketCount; ++i)
	{
		targetIndex = (nextPacketIndex + i) % eMaxPacketCount;
		if(transmitBuffer[targetIndex].transmitCount == 0)
		{
			break;
		}
	}

	if(targetIndex >= eMaxPacketCount)
	{
		return false;
	}

	nextPacketIndex = targetIndex + 1;
	transmitBuffer[targetIndex].size = inSize;
	memcpy(transmitBuffer[targetIndex].data, inData, inSize);
	transmitBuffer[targetIndex].transmitCount = eTransmitCount;	// This must be set last since it triggers the interrupt track bit banger to start using data from this packet

	return true;
}

bool
CModule_DCC::SCommandState::DoServiceModePacket(
	uint8_t			inSize,
	uint8_t const*	inData)
{
	return true;
}

CModule_DCC::SDecoder*
CModule_DCC::SCommandState::FindDecoder(
	uint16_t	inAddress)
{
	int	availIndex = 0xFFFF;

	for(int i = 0; i < eMaxDecoders; ++i)
	{
		if(decoderArray[i].address == inAddress)
		{
			return decoderArray + i;
		}

		if(availIndex == 0xFFFF && decoderArray[i].address == 0xFFFF)
		{
			availIndex = i;
		}
	}

	if(availIndex == 0xFFFF)
	{
		return NULL;
	}

	SDecoder*	target = decoderArray + availIndex;
	target->address = inAddress;
	target->speed = 0;
	target->direction = 1;

	return target;
}

void
CModule_DCC::SCommandState::SendStandardSpeedAndDirectionPacket(
	SDecoder*	inDecoder)
{
	uint8_t	data[3];

	uint8_t	dir = inDecoder->direction & 1;
	uint8_t	speed = inDecoder->speed & 0x1F;

	data[0] = inDecoder->address;
	data[1] = (dir << 5) | ((speed & 1) << 4) | (speed >> 1);
	data[2] = data[0] ^ data[1];

	AddPacket(3, data);
}

bool
CModule_DCC::SCommandState::GetNextWaveformBit(
	void)
{
	if(curState == eState_Preamble)
	{
		if(curPreambleBitCount > 0)
		{
			--curPreambleBitCount;
		}
		else
		{
			if(transmitBuffer[curPacketIndex].transmitCount > 0)
			{
				// We have a packet to transmit so start it going
				curState = eState_StartBit;
			}
			else
			{
				// There is no packet to transmit so just go to the end packet state to look for another packet
				curState = eState_EndPacket;
			}
		}

		return true;	// Send a 1 bit as part of the preamble
	}

	if(curState == eState_StartBit)
	{
		// Get the next byte ready to go
		curByte = transmitBuffer[curPacketIndex].data[curByteIndex++];
		curBitIndex = 0;
		curState = eState_Byte;

		return false;	// Send a 0 bit to indicate the start of a byte
	}

	if(curState == eState_Byte)
	{
		bool	result = (curByte & 0x80) ? true : false;

		if(curBitIndex >= 7)
		{
			// We are done transmitting all 8 bits

			if(curByteIndex >= transmitBuffer[curPacketIndex].size)
			{
				// There are no more bytes so handle end of packet
				curState = eState_EndPacket;
			}
			else
			{
				// There are more bytes so start transmitting the next byte
				curState = eState_StartBit;
			}
		}
		else
		{
			// Move the next bit into the high bit for transmition
			++curBitIndex;
			curByte <<= 1;
		}

		return result;	// Return the next bit in the byte
	}

	// We must be in the eState_EndPacket, if we are not in any other state just assume this state

	// Try to find another packet
	for(int i = 0; i < eMaxPacketCount; ++i)
	{
		int	curIndex = (curPacketIndex + i) % eMaxPacketCount;
		if(transmitBuffer[curIndex].transmitCount > 0)
		{
			--transmitBuffer[curIndex].transmitCount;
			curPacketIndex = curIndex + 1;
			break;
		}
	}

	curByteIndex = 0;
	curState = eState_Preamble;
	curPreambleBitCount = ePreambleBitCount;

	return true;	// return the packet end bit
}

void
CModule_DCC::UpdateWaveform(
	void)
{
	for(int i = 0; i < eMaxCommandStations; ++i)
	{
		SCommandState*	commandState = gDCC.commandStateList + i;
		SMsg_DCCCommandConfig*	commandConfig = gDCC.commandConfigList + i;

		if(commandConfig->commandID == eInvalidID || commandConfig->powerPin >= eDIOPinCount || commandConfig->waveformPin >= eDIOPinCount || commandState->power == false)
		{
			continue;
		}

		switch(commandState->curPhase)
		{
			case 0:
				digitalWriteFast(commandConfig->waveformPin, LOW);	// Set waveform LOW voltage to start bit transmission
				commandState->curBit = commandState->GetNextWaveformBit();
				commandState->curPhase = 1;
				break;

			case 1:
				if(commandState->curBit)
				{
					digitalWriteFast(commandConfig->waveformPin, HIGH);	// Set waveform to HIGH voltage to indicate a digital 1
					commandState->curPhase = 0;
				}
				else
				{
					// leave waveform at a LOW voltage to indicate a digital 0
					commandState->curPhase = 2;
				}
				break;

			case 2:
				digitalWriteFast(commandConfig->waveformPin, HIGH);
				commandState->curPhase = 3;
				break;

			case 3:
				// Leave high
				commandState->curPhase = 0;
				break;
		}
	}
}

int
CModule_DCC::FindIndexFromID(
	uint16_t	inCommandID)
{
	for(int i = 0; i < eMaxCommandStations; ++i)
	{
		if(commandConfigList[i].commandID == inCommandID)
		{
			return i;
		}
	}

	return 0xFF;
}

