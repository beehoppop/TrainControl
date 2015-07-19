// 
// 
// 
#include <EEPROM.h>

#include "TCAssert.h"
#include "TCCommon.h"
#include "TCModule.h"
#include "TCUtilities.h"

enum
{
	eEEPROM_VersionOffset = 0,
	eEEPROM_ListStart = 1,

	eEEPROM_Size = 2048

};

struct SEEPROMEntry
{
	uint32_t	uid;
	uint16_t	offset;
	uint16_t	size;
};

static int			gModuleCount;
static CModule*		gModuleList[eMaxModuleCount];
static SEEPROMEntry	gEEPROMEntryList[eMaxModuleCount];
static bool			gTooManyModules = false;

CModule::CModule(
	uint32_t	inUID,
	uint16_t	inEEPROMSize)
	:
	uid(inUID),
	eepromSize(inEEPROMSize)
{
	if(gModuleCount >= eMaxModuleCount)
	{
		gTooManyModules = true;
		return;
	}

	gModuleList[gModuleCount++] = this;
}

void
CModule::Setup(
	void)
{

}

void
CModule::Update(
	void)
{

}

void
CModule::EEPROMInitialize(
	void)
{
	for(int i = eepromOffset; i < eepromOffset + eepromSize; ++i)
	{
		EEPROM.write(i, 0xFF);
	}
}

SEEPROMEntry*
FindEEPROMEntry(
	uint32_t		inUID)
{
	for(int i = 0; i < eMaxModuleCount; ++i)
	{
		if(gEEPROMEntryList[i].uid == inUID)
			return gEEPROMEntryList + i;
	}

	return NULL;
}

CModule*
FindModule(
	uint32_t	inUID)
{
	for(int i = 0; i < gModuleCount; ++i)
	{
		if(gModuleList[i]->uid == inUID)
			return gModuleList[i];
	}

	return NULL;
}

int
EEPROMCompare(
	void const*	inA,
	void const*	inB)
{
	SEEPROMEntry const*	a = (SEEPROMEntry const*)inA;
	SEEPROMEntry const*	b = (SEEPROMEntry const*)inB;

	if(a->offset < b->offset)
	{
		return -1;
	}

	if(a->offset > b->offset)
	{
		return 1;
	}

	return 0;
}

uint16_t
FindAvailableEEPROMOffset(
	uint16_t		inSize)
{
	uint16_t	offset = eEEPROM_ListStart + eMaxModuleCount * sizeof(SEEPROMEntry);

	for(int i = 0; i < eMaxModuleCount; ++i)
	{
		if(gEEPROMEntryList[i].uid == 0xFFFFFFFF)
		{
			continue;
		}

		if(offset + inSize <= gEEPROMEntryList[i].offset)
		{
			MAssert(offset + inSize <= eEEPROM_Size);
			return offset;
		}

		offset = gEEPROMEntryList[i].offset + gEEPROMEntryList[i].size;
	}

	MAssert(offset + inSize <= eEEPROM_Size);

	return offset;
}

void
CModule::SetupAll(
	void)
{
	Serial.begin(115200);

	if(gTooManyModules)
	{
		while(true)
		{
			Serial.printf("Too Many Modules\n");
		}
	}

	uint8_t	eepromVersion = EEPROM.read(eEEPROM_VersionOffset);

	if(eepromVersion != eEEPROMVersion)
	{
		for(int i = eEEPROM_ListStart; i < eEEPROM_ListStart + (int)sizeof(gEEPROMEntryList); ++i)
		{
			EEPROM.write(i, 0xFF);
		}
		EEPROM.write(eEEPROM_VersionOffset, eEEPROMVersion);
	}

	bool	changes = false;

	LoadDataFromEEPROM(gEEPROMEntryList, eEEPROM_ListStart, sizeof(gEEPROMEntryList));

	for(int i = 0; i < gModuleCount; ++i)
	{
		if(gModuleList[i]->eepromSize == 0)
			continue;

		SEEPROMEntry*	target = FindEEPROMEntry(gModuleList[i]->uid);

		if(target != NULL && target->size != gModuleList[i]->eepromSize)
		{
			DebugMsg(eDbgLevel_Medium, "Module: %s changed size\n", StringizeUInt32(gModuleList[i]->uid));
			target->uid = 0xFFFFFFFF;
			target->offset = 0xFFFF;
			target->size = 0xFFFF;
			target = NULL;
		}

		if(target == NULL)
		{
			target = FindEEPROMEntry(0xFFFFFFFF);
			MAssert(target != NULL);

			target->offset = FindAvailableEEPROMOffset(gModuleList[i]->eepromSize);
			target->uid = gModuleList[i]->uid;
			target->size = gModuleList[i]->eepromSize;

			gModuleList[i]->eepromOffset = target->offset;
			gModuleList[i]->EEPROMInitialize();

			DebugMsg(eDbgLevel_Medium, "Module: %s added\n", StringizeUInt32(target->uid));

			changes = true;
		}

		gModuleList[i]->eepromOffset = target->offset;
		DebugMsg(eDbgLevel_Medium, "Module: %s offset is %d\n", StringizeUInt32(target->uid), target->offset);
	}

	for(int i = 0; i < eMaxModuleCount; ++i)
	{
		if(gEEPROMEntryList[i].uid == 0xFFFFFFFF)
		{
			continue;
		}

		CModule*	target = FindModule(gEEPROMEntryList[i].uid);

		if(target == NULL)
		{
			DebugMsg(eDbgLevel_Medium, "Module: %s removed\n", StringizeUInt32(target->uid));
			gEEPROMEntryList[i].uid = 0xFFFFFFFF;
			gEEPROMEntryList[i].offset = 0xFFFF;
			gEEPROMEntryList[i].size = 0xFFFF;
			changes = true;
		}
	}

	if(changes)
	{
		qsort(gEEPROMEntryList, eMaxModuleCount, sizeof(SEEPROMEntry), EEPROMCompare);
		WriteDataToEEPROM(gEEPROMEntryList, eEEPROM_ListStart, sizeof(gEEPROMEntryList));
	}

	for(int i = 0; i < gModuleCount; ++i)
	{
		Serial.printf("Setup module %s\n", StringizeUInt32(gModuleList[i]->uid));
		gModuleList[i]->Setup();
	}
}

void
CModule::LoopAll(
	void)
{
	for(int i = 0; i < gModuleCount; ++i)
	{
		gModuleList[i]->Update();
	}
}
