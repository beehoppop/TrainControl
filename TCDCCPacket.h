// TCDCCPacket.h

#ifndef _TCDCCPACKET_h
#define _TCDCCPACKET_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

enum
{
	eMaxDecoders = 128,

};

class CDCCPacketClass
{
public:
	
	CDCCPacketClass(
		);

	void
	EmergencyAllStop(
		void);

	void
	ResetAllDecoders(
		void);

	void
	EmergencyStop(
		uint8_t	inAddress);
	
	void
	StandardDirection(
		uint8_t	inAddress,
		uint8_t	inDirectionForward);
	
	void
	StandardSpeed(
		uint8_t	inAddress,
		uint8_t	inSpeed);

private:
	
	struct SDecoder
	{
		uint16_t	address;
		uint8_t		direction;
		uint8_t		speed;
	};

	SDecoder	decoderArray[eMaxDecoders];

	SDecoder*
	FindDecoder(
		uint16_t	inAddress);

	void
	SendStandardSpeedAndDirectionPacket(
		SDecoder*	inDecoder);
};

extern CDCCPacketClass gDCCPacket;

#endif

