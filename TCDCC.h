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
	TearDown(
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
		eTransmitCount = 10,
		eMaxCommandStations = 2,
		eMaxDecoders = 64,
		eMaxCommandStationID = 64,
	};
	
	struct SDecoder
	{
		uint16_t	address;
		uint8_t		direction;
		uint8_t		speed;

		uint8_t				transmitCount;	// The number of remaining times to transmit the current packet
		uint8_t volatile	bufferIndex;	// The buffer index currently being used to put data on the track
		uint8_t				size[2];		// The size of the packet
		uint8_t				data[2][eMaxPacketSize];	// The data buffers for the packet
	};

	struct SCommandState
	{
		bool	opsMode;	// true if in normal operations mode, false if in programming mode
		bool	power;		// true if power to the track is on

		uint8_t		curDecoderTranxIndex;
		uint8_t		curByteIndex;
		uint8_t		curBitIndex;
		uint8_t		curPreambleBitCount;
		uint8_t		curByte;
		uint8_t		curPhase;
		uint8_t		curState;
		uint8_t		curBit;
		uint8_t		curBufferIndex;
		SDecoder	decoderArray[eMaxDecoders];

		SDecoder*	curDecoderTranx;

		void
		Reset(
			void);

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
