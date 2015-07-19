// 
// 
// 

#include "TCAssert.h"
#include "TCState.h"
#include "TCUtilities.h"

CModule_State::CModule_State(
	)
	:
	CModule(MMakeUID('s', 't', 'a', 't'), 0)
{
	memset(stateVar, 0, sizeof(stateVar));
}

uint8_t
CModule_State::GetVal(
	uint8_t	inVar)
{
	MAssert(inVar < eStateVar_Max);
	return stateVar[inVar];
}

void
CModule_State::SetVal(
	uint8_t	inVar,
	uint8_t	inVal)
{
	MAssert(inVar < eStateVar_Max);
	stateVar[inVar] = inVal;
}

CModule_State	gState;
