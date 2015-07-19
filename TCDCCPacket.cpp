// 
// 
// 

#include "TCDCCPacket.h"
#include "TCDCCProtocol.h"

CDCCPacketClass gDCCPacket;

CDCCPacketClass::CDCCPacketClass(
	)
{
	memset(decoderArray, 0xFF, sizeof(decoderArray));
}

void
CDCCPacketClass::EmergencyAllStop(
	void)
{

}

void
CDCCPacketClass::ResetAllDecoders(
	void)
{

}

void
CDCCPacketClass::EmergencyStop(
	uint8_t	inAddress)
{

}
	
void
CDCCPacketClass::StandardDirection(
	uint8_t	inAddress,
	uint8_t	inDirection)
{
	SDecoder*	decoder = FindDecoder(inAddress);

	decoder->direction = inDirection;
	SendStandardSpeedAndDirectionPacket(decoder);
}
	
void
CDCCPacketClass::StandardSpeed(
	uint8_t	inAddress,
	uint8_t	inSpeed)
{
	SDecoder*	decoder = FindDecoder(inAddress);

	decoder->speed = inSpeed;
	SendStandardSpeedAndDirectionPacket(decoder);
}

CDCCPacketClass::SDecoder*
CDCCPacketClass::FindDecoder(
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

	return NULL;
}

void
CDCCPacketClass::SendStandardSpeedAndDirectionPacket(
	SDecoder*	inDecoder)
{
	uint8_t	data[3];

	uint8_t	dir = inDecoder->direction & 1;
	uint8_t	speed = inDecoder->speed & 0x1F;

	data[0] = inDecoder->address;
	data[1] = (dir << 5) | ((speed & 1) << 4) | (speed >> 1);
	data[2] = data[0] ^ data[1];

	gDCCProtocol.AddPacket(3, data);
}
