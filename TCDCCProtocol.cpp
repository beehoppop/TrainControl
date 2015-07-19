// 
// 
// 
#include <TimerOne.h>

#include "TCAssert.h"
#include "TCCommon.h"
#include "TCUtilities.h"
#include "TCConfig.h"
#include "TCState.h"
#include "TCDCCProtocol.h"

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

CModule_DCCProtocol gDCCProtocol;

CModule_DCCProtocol::CModule_DCCProtocol(
	)
	:
	CModule(MMakeUID('d', 'c', 'p', 't'), 0)
{

}

void
CModule_DCCProtocol::Setup(
	void)
{
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
}

void
CModule_DCCProtocol::Update(
	void)
{
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
}

bool
CModule_DCCProtocol::AddPacket(
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

	transmitBuffer[targetIndex].transmitCount = eTransmitCount;
	transmitBuffer[targetIndex].size = inSize;
	memcpy(transmitBuffer[targetIndex].data, inData, inSize);

	return true;
}

bool
CModule_DCCProtocol::GetNextWaveformBit(
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

	return true;
}

void
CModule_DCCProtocol::SetupNextByte(
	void)
{
	if(curByteIndex >= transmitBuffer[curPacketIndex].size)
	{
		SetupNextPacket();
		return;
	}

	curByte = transmitBuffer[curPacketIndex].data[curByteIndex++];
	curBitIndex = 0;

	curState = eState_TransmitByte;
}

void
CModule_DCCProtocol::SetupNextPacket(
	void)
{
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
}

void
CModule_DCCProtocol::UpdateWaveform(
	void)
{
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
}
