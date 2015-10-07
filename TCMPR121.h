#ifndef _TCMPR121_h
#define _TCMPR121_h

#include "TCCommon.h"

void
MPRRegisterTouchSensor(
	int				inPort,
	ITouchSensor*	inTouchSensor);

void
MPRUnRegisterTouchSensor(
	int				inPort,
	ITouchSensor*	inTouchSensor);

void
MPRSetTouchSensitivity(
	int		inPort,
	int		inID,
	uint8_t	inSensitivity);	// 0 is lowest sensitivity, 0xFF is highest

void
MPRSetReleaseSensitivity(
	int		inPort,
	int		inID,
	uint8_t	inSensitivity);	// 0 is lowest sensitivity, 0xFF is highest

#endif
