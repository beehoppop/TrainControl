// TCDCCProtocol.h

#ifndef _TCDCCPROTOCOL_h
#define _TCDCCPROTOCOL_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <IntervalTimer.h>

#include "TCModule.h"

class CModule_DCC : CModule
{
public:

	CModule_DCC(
		);

	virtual void
	Setup(
		void);

	virtual void
	Update(
		uint32_t	inDeltaTimeUS);

	void
	SetPowerState(
		uint8_t	inCommandID,	// 0xFF means all command stations
		bool	inPowerOn);
	
	void
	EmergencyAllStop(
		uint16_t	inCommandID);

	void
	ResetAllDecoders(
		uint16_t	inCommandID);

	void
	EmergencyStop(
		uint16_t	inCommandID,
		uint8_t		inAddress);
	
	void
	StandardDirection(
		uint16_t	inCommandID,
		uint8_t		inAddress,
		uint8_t		inDirectionForward);
	
	void
	StandardSpeed(
		uint16_t	inCommandID,
		uint8_t		inAddress,
		uint8_t		inSpeed);

	void
	SetOpsMode(
		uint8_t			inTrackID,
		bool			inOpsMode);	// false is service mode

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
	
	enum
	{
		eMaxPacketSize = 6,
		eMaxPacketCount = 64,
		eTransmitCount = 10,
		eMaxCommandStations = 2,
		eMaxDecoders = 64,
	};
	
	struct SDecoder
	{
		uint16_t	address;
		uint8_t		direction;
		uint8_t		speed;
	};

	struct SPacket
	{
		uint8_t	transmitCount;
		uint8_t	size;
		uint8_t	data[eMaxPacketSize];
	};

	struct SCommandState
	{
		bool	opsMode;
		bool	power;
		bool	reverse;

		uint8_t		curPacketIndex;
		uint8_t		curByteIndex;
		uint8_t		curBitIndex;
		uint8_t		curPreambleBitCount;
		uint8_t		curByte;
		uint8_t		curPhase;
		uint8_t		curState;
		uint8_t		curBit;
		uint8_t		nextPacketIndex;
		SPacket		transmitBuffer[eMaxPacketCount];
		SDecoder	decoderArray[eMaxDecoders];

		void
		Reset(
			void);

		bool
		AddPacket(
			uint8_t			inSize,
			uint8_t const*	inData);

		bool
		DoServiceModePacket(
			uint8_t			inSize,
			uint8_t const*	inData);

		SDecoder*
		FindDecoder(
			uint16_t	inAddress);

		void
		SendStandardSpeedAndDirectionPacket(
			SDecoder*	inDecoder);

		bool
		GetNextWaveformBit(
			void);
	};


	static void
	UpdateWaveform(
		void);

	int
	FindIndexFromID(
		uint16_t	inCommandID);

	uint8_t	signalPulse;
	bool	isMaster;

	IntervalTimer			masterTimer;
	SMsg_DCCCommandConfig	commandConfigList[eMaxCommandStations];
	SCommandState			commandStateList[eMaxCommandStations];
};

extern CModule_DCC gDCC;

#endif
