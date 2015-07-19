// 
// 
// 
#include <FastLED.h>

#include "TCModule.h"
#include "TCUtilities.h"
#include "TCLED.h"

CLEDClass gLED;

CLEDClass::CLEDClass(
	)
	:
	CModule(MMakeUID('l', 'e', 'd', 's'), 0)
{

}

void
CLEDClass::Setup(
	void)
{

}

void
CLEDClass::Update(
	void)
{

}

void
CLEDClass::SetLEDColor(
	uint8_t		inLEDIndex,
	uint32_t	inColor)
{

}
