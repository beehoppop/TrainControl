// 
// 
// 
#include <IntervalTimer.h>

#include "TCAssert.h"
#include "TCCommon.h"
#include "TCUtilities.h"
#include "TCConfig.h"
#include "TCState.h"
#include "TCDCC.h"

enum
{
	eDCC_WaveformPeriodOneUS = 58,
	eDCC_WaveformPeriodZeroUS = 100,

	ePreambleBitCount = 14,

	eState_Preamble = 0,
	eState_TransmitStartByte,
	eState_TransmitByte,
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
#if 0
	curPacketIndex = 0;
	curByteIndex = 0;
	curBitIndex = 0;
	curPhase = 0;
	nextPacketIndex = 0;

	curPreambleBitCount = ePreambleBitCount;
	curState = eState_Preamble;

	trackWaveformPin = 0xFF;
	trackPowerPin = 0xFF;

	memset(transmitBuffer, 0, sizeof(transmitBuffer));
#endif
}

void
CModule_DCC::Update(
	uint32_t	inDeltaTimeUS)
{
#if 0
	if(trackWaveformPin != gConfig.GetVal(eConfigVar_TrackWaveformPin) || trackPowerPin != gConfig.GetVal(eConfigVar_TrackPowerPin))
	{
		trackWaveformPin = gConfig.GetVal(eConfigVar_TrackWaveformPin);
		trackPowerPin = gConfig.GetVal(eConfigVar_TrackPowerPin);

		if(trackWaveformPin < eDIOPinCount && trackPowerPin < eDIOPinCount)
		{
			pinMode(trackWaveformPin, OUTPUT);
			pinMode(trackPowerPin, OUTPUT);
			Timer1.initialize(eDCC_WaveformPeriodOneUS);
			Timer1.attachInterrupt(UpdateWaveform);
		}
		else
		{
			Timer1.stop();
			return;
		}
	}

	if(trackWaveformPin < eDIOPinCount && trackPowerPin < eDIOPinCount)
	{
		if(trackPower != gState.GetVal(eStateVar_TrackPower))
		{
			trackPower = gState.GetVal(eStateVar_TrackPower);
			digitalWriteFast(trackPowerPin, trackPower);
			if(trackPower)
			{
				Timer1.restart();
			}
			else
			{
				Timer1.stop();
				memset(transmitBuffer, 0, sizeof(transmitBuffer));
			}
		}
	}
#endif
}

void
CModule_DCC::SetPowerState(
	uint8_t	inCommandID,	// 0xFF means all command stations
	bool	inPowerOn)
{

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
#if 0
	SDecoder*	decoder = FindDecoder(inAddress);

	decoder->direction = inDirection;
	SendStandardSpeedAndDirectionPacket(decoder);
#endif
}
	
void
CModule_DCC::StandardSpeed(
	uint16_t	inCommandID,
	uint8_t		inAddress,
	uint8_t		inSpeed)
{
#if 0
	SDecoder*	decoder = FindDecoder(inAddress);

	decoder->speed = inSpeed;
	SendStandardSpeedAndDirectionPacket(decoder);
#endif
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
	return false;
}

bool
CModule_DCC::TableWrite(
	int8_t				inSrcNode,
	SMsg_Table const&	inProgram)
{
	return false;
}

bool
CModule_DCC::TableUpdate(
	int8_t				inSrcNode,
	SMsg_Table const&	inProgram)
{
	return false;
}

bool
CModule_DCC::AddPacket(
	uint8_t			inCommandID,
	uint8_t			inSize,
	uint8_t const*	inData)
{
#if 0
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

	transmitBuffer[targetIndex].transmitCount = eTransmitCount;
	transmitBuffer[targetIndex].size = inSize;
	memcpy(transmitBuffer[targetIndex].data, inData, inSize);

#endif
	return true;
}

bool
CModule_DCC::DoServiceModePacket(
	uint8_t			inTrackID,
	uint8_t			inSize,
	uint8_t const*	inData)
{
	return true;

}

CModule_DCC::SDecoder*
CModule_DCC::FindDecoder(
	uint16_t	inCommandID,
	uint16_t	inAddress)
{
#if 0
	int	availIndex = 0xFFFF;

	for(int i = 0; i < eMaxCommandStates; ++i)
	{

	}

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
#endif

	return NULL;
}

void
CModule_DCC::SendStandardSpeedAndDirectionPacket(
	uint16_t	inCommandID,
	SDecoder*	inDecoder)
{
#if 0
	uint8_t	data[3];

	uint8_t	dir = inDecoder->direction & 1;
	uint8_t	speed = inDecoder->speed & 0x1F;

	data[0] = inDecoder->address;
	data[1] = (dir << 5) | ((speed & 1) << 4) | (speed >> 1);
	data[2] = data[0] ^ data[1];

	gDCCProtocol.AddPacket(3, data);
#endif
}

bool
CModule_DCC::GetNextWaveformBit(
	void)
{
#if 0
	if(curState == eState_Preamble)
	{
		if(curPreambleBitCount > 0)
		{
			--curPreambleBitCount;
		}
		else
		{
			curState = eState_TransmitStartByte;
		}
		return true;
	}

	if(curState == eState_TransmitStartByte)
	{
		SetupNextByte();
		return false;
	}

	if(curState == eState_TransmitByte)
	{
		bool	result = (curByte & 0x80) ? true : false;

		if(curBitIndex >= 7)
		{
			if(curByteIndex >= transmitBuffer[curPacketIndex].size)
			{
				curState = eState_EndPacket;
			}
			else
			{
				curState = eState_TransmitStartByte;
			}
		}
		else
		{
			++curBitIndex;
			curByte <<= 1;
		}

		return result;
	}

	if(curState == eState_EndPacket)
	{
		SetupNextPacket();
		return true;
	}

	MAssert(0);
#endif

	return true;
}

void
CModule_DCC::SetupNextByte(
	void)
{
#if 0
	if(curByteIndex >= transmitBuffer[curPacketIndex].size)
	{
		SetupNextPacket();
		return;
	}

	curByte = transmitBuffer[curPacketIndex].data[curByteIndex++];
	curBitIndex = 0;

	curState = eState_TransmitByte;
#endif
}

void
CModule_DCC::SetupNextPacket(
	void)
{
#if 0
	curState = eState_Preamble;
	curPreambleBitCount = ePreambleBitCount;
	curByteIndex = 0;

	for(int i = 0; i < eMaxPacketCount - 1; ++i)
	{
		int	curIndex = (curPacketIndex + i + 1) % eMaxPacketCount;
		if(transmitBuffer[curIndex].transmitCount > 0)
		{
			--transmitBuffer[curIndex].transmitCount;
			curPacketIndex = curIndex;
			return;
		}
	}

	curPacketIndex = 0;
#endif
}

void
CModule_DCC::UpdateWaveform(
	void)
{
#if 0
	if(gDCCProtocol.curPhase == 0)
	{
		digitalWriteFast(gDCCProtocol.trackWaveformPin, LOW);
		gDCCProtocol.curPhase = 1;

		bool	curBit = gDCCProtocol.GetNextWaveformBit();

		Timer1.setPeriod(curBit ? eDCC_WaveformPeriodOneUS : eDCC_WaveformPeriodZeroUS);
	}
	else
	{
		digitalWriteFast(gDCCProtocol.trackWaveformPin, HIGH);
		gDCCProtocol.curPhase = 0;
	}
#endif
}
