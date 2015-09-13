#ifndef _TCMPR121_h
#define _TCMPR121_h

class ITouchSensor
{
public:

	virtual void
	Touch(
		int	inTouchID) = 0;

	virtual void
	Release(
		int	inTouchID) = 0;

};

void
RegisterTouchSensor(
	int				inPort,
	ITouchSensor*	inTouchSensor);

void
SetTouchSensitivity(
	int		inPort,
	int		inID,
	uint8_t	inSensitivity);	// 0 is lowest sensitivity, 0xFF is highest

void
SetReleaseSensitivity(
	int		inPort,
	int		inID,
	uint8_t	inSensitivity);	// 0 is lowest sensitivity, 0xFF is highest

#endif
