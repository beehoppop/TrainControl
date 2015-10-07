#ifndef _TCTEENSYTOUCH_h
#define _TCTEENSYTOUCH_h

#include "TCCommon.h"

void
TeensyRegisterTouchSensor(
	uint32_t		inTouchInputBV,
	ITouchSensor*	inTouchSensor);

void
TeensyUnRegisterTouchSensor(
	ITouchSensor*	inTouchSensor);

void
TeensyAddTouchInputs(
	uint32_t		inTouchInputBV);

void
TeensyRemoveTouchInputs(
	uint32_t		inTouchInputBV);



#endif /* _TCTEENSYTOUCH_h */

