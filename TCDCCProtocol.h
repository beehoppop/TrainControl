// TCDCCProtocol.h

#ifndef _TCDCCPROTOCOL_h
#define _TCDCCPROTOCOL_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TCModule.h"

class CModule_DCCProtocol : CModule
{
public:

	CModule_DCCProtocol(
		);

	virtual void
	Setup(
		void);

	virtual void
	Update(
		void);
	
	bool
	AddPacket(
		uint8_t			inSize,
		uint8_t const*	inData);

private:
	
	enum
	{
		eMaxPacketSize = 6,
		eMaxPacketCount = 64,
		eTransmitCount = 10,
	};

	struct SPacket
	{
		uint8_t	transmitCount;
		uint8_t	size;
		uint8_t	data[eMaxPacketSize];
	};

	uint8_t	curPacketIndex;
	uint8_t	curByteIndex;
	uint8_t	curBitIndex;
	uint8_t	curPreambleBitCount;
	uint8_t	curByte;
	uint8_t	curPhase;
	uint8_t	curState;
	uint8_t	trackWaveformPin;
	uint8_t	trackPowerPin;
	uint8_t	nextPacketIndex;
	bool	trackPower;

	SPacket	transmitBuffer[eMaxPacketCount];

	bool
	GetNextWaveformBit(
		void);

	void
	SetupNextByte(
		void);

	void
	SetupNextPacket(
		void);

	static void
	UpdateWaveform(
		void);
};

extern CModule_DCCProtocol gDCCProtocol;

#endif
