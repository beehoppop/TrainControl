
#include <Wire.h>

#include "TCAssert.h"
#include "TCModule.h"
#include "TCUtilities.h"
#include "TCMPR121.h"

// MPR121 Register Defines
#define MHD_R	0x2B
#define NHD_R	0x2C
#define	NCL_R 	0x2D
#define	FDL_R	0x2E
#define	MHD_F	0x2F
#define	NHD_F	0x30
#define	NCL_F	0x31
#define	FDL_F	0x32
#define	ELE0_T	0x41
#define	ELE0_R	0x42
#define	ELE1_T	0x43
#define	ELE1_R	0x44
#define	ELE2_T	0x45
#define	ELE2_R	0x46
#define	ELE3_T	0x47
#define	ELE3_R	0x48
#define	ELE4_T	0x49
#define	ELE4_R	0x4A
#define	ELE5_T	0x4B
#define	ELE5_R	0x4C
#define	ELE6_T	0x4D
#define	ELE6_R	0x4E
#define	ELE7_T	0x4F
#define	ELE7_R	0x50
#define	ELE8_T	0x51
#define	ELE8_R	0x52
#define	ELE9_T	0x53
#define	ELE9_R	0x54
#define	ELE10_T	0x55
#define	ELE10_R	0x56
#define	ELE11_T	0x57
#define	ELE11_R	0x58
#define	FIL_CFG	0x5D
#define	ELE_CFG	0x5E
#define GPIO_CTRL0	0x73
#define	GPIO_CTRL1	0x74
#define GPIO_DATA	0x75
#define	GPIO_DIR	0x76
#define	GPIO_EN		0x77
#define	GPIO_SET	0x78
#define	GPIO_CLEAR	0x79
#define	GPIO_TOGGLE	0x7A
#define	ATO_CFG0	0x7B
#define	ATO_CFGU	0x7D
#define	ATO_CFGL	0x7E
#define	ATO_CFGT	0x7F

#define TOU_THRESH	0x26
#define	REL_THRESH	0x2A

enum
{
	eMaxInterfaces = 4,

	eIRQPin = 20,

	eInputCount = 12,
};

struct SSensorInterface
{
	int				port;
	ITouchSensor*	touchSensor;
};

class CModule_MPR121 : public CModule
{
public:
	
	CModule_MPR121(
		);

	void
	Setup(
		void);
	
	void
	Update(
		uint32_t	inDeltaTimeUS);

	void
	Initialize(
		void);

	bool	initialized;
};

static int				gSensorInterfaceCount;
static SSensorInterface	gSensorInterfaceList[eMaxInterfaces];
static CModule_MPR121	gModuleMPR121;
static int				gMPRRegister = 0x5A;
static uint8_t			gTouchState[eInputCount];
static uint32_t			gLastTouchTime[eInputCount];
static bool				gMPR121Present;

enum 
{
	eState_WaitingForChangeToTouched,
	eState_WaitingForTouchSettle,
	eState_WaitingForChangeToRelease,
	eState_WaitingForReleaseSettle,

	eUpdateTimeUS = 50000,
	eSettleTimeMS = 200
};

uint8_t
MPR121SetRegister(
	uint8_t	inRegister, 
	uint8_t	inValue)
{
	Wire.beginTransmission(gMPRRegister);
	Wire.write(inRegister);
	Wire.write(inValue);
	return Wire.endTransmission();
}

CModule_MPR121::CModule_MPR121(
	)
	:
	CModule("mpr1", 0, eUpdateTimeUS)
{

}

void
CModule_MPR121::Setup(
	void)
{

}

void
CModule_MPR121::Initialize(
	void)
{
	if(initialized)
	{
		return;
	}

	initialized = true;

	pinMode(eIRQPin, INPUT);
	digitalWrite(eIRQPin, HIGH); //enable pullup resistor

	//DebugMsg(eDbgLevel_Verbose, "MPR121: Staring wire");
	Wire.begin();
	//DebugMsg(eDbgLevel_Verbose, "MPR121: done wire");

	// Enter config mode
	if(MPR121SetRegister(ELE_CFG, 0x00) != 0)
	{
		gMPR121Present = false;
	} 
 
 	gMPR121Present = true;
 
	// Section A - Controls filtering when data is > baseline.
	MPR121SetRegister(MHD_R, 0x01);
	MPR121SetRegister(NHD_R, 0x01);
	MPR121SetRegister(NCL_R, 0x00);
	MPR121SetRegister(FDL_R, 0x00);

	// Section B - Controls filtering when data is < baseline.
	MPR121SetRegister(MHD_F, 0x01);
	MPR121SetRegister(NHD_F, 0x01);
	MPR121SetRegister(NCL_F, 0xFF);
	MPR121SetRegister(FDL_F, 0x02);
  
	// Section C - Sets touch and release thresholds for each electrode
	for(int itr = 0; itr < eInputCount; ++itr)
	{
		MPR121SetRegister(ELE0_T + itr * 2, TOU_THRESH);
		MPR121SetRegister(ELE0_R + itr * 2, REL_THRESH);
	}
  
	// Section D
	// Set the Filter Configuration
	// Set ESI2
	MPR121SetRegister(FIL_CFG, 0x04);
  
	// Section E
	// Electrode Configuration
	// Set ELE_CFG to 0x00 to return to standby mode
	MPR121SetRegister( ELE_CFG, 0x0C);  // Enables all 12 Electrodes
  
  
	// Section F
	// Enable Auto Config and auto Reconfig
	/*set_register(0x5A, ATO_CFG0, 0x0B);
	set_register(0x5A, ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   set_register(0x5A, ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
	set_register(0x5A, ATO_CFGT, 0xB5);*/  // Target = 0.9*USL = 0xB5 @3.3V
	
	// Exist config mode
	MPR121SetRegister(ELE_CFG, 0x0C);

}

void
CModule_MPR121::Update(
	uint32_t	inDeltaTimeUS)
{
	if(!gMPR121Present)
	{
		return;
	}

	//read the touch state from the MPR121
	Wire.requestFrom(gMPRRegister, 2); 
    
	uint16_t LSB = Wire.read();
	uint16_t MSB = Wire.read();
	uint16_t touchedBV = ((MSB << 8) | LSB);
		
	for(int itr = 0; itr < eInputCount; ++itr)
	{
		bool	touched = (touchedBV & (1 << itr)) ? true : false;

		switch(gTouchState[itr])
		{
			case eState_WaitingForChangeToTouched:
				if(touched)
				{
					gLastTouchTime[itr] = gCurTimeMS;
					gTouchState[itr] = eState_WaitingForTouchSettle;
				}
				break;

			case eState_WaitingForTouchSettle:
				if(!touched)
				{
					gTouchState[itr] = eState_WaitingForChangeToTouched;
					break;
				}

				if(gCurTimeMS - gLastTouchTime[itr] >= eSettleTimeMS)
				{
					gTouchState[itr] = eState_WaitingForChangeToRelease;
					DebugMsg(eDbgLevel_Verbose, "MPR121: Touched %d\n", itr);
					for(int itr2 = 0; itr2 < gSensorInterfaceCount; ++itr2)
					{
						gSensorInterfaceList[itr2].touchSensor->Touch(itr);
					}
				}
				break;

			case eState_WaitingForChangeToRelease:
				if(!touched)
				{
					//DebugMsg(eDbgLevel_Verbose, "MPR121: Going to release settle %d\n", itr);
					gLastTouchTime[itr] = gCurTimeMS;
					gTouchState[itr] = eState_WaitingForReleaseSettle;
				}
				break;

			case eState_WaitingForReleaseSettle:
				if(touched)
				{
					//DebugMsg(eDbgLevel_Verbose, "MPR121: Going to release waiting %d\n", itr);
					gTouchState[itr] = eState_WaitingForChangeToRelease;
					break;
				}

				if(gCurTimeMS - gLastTouchTime[itr] >= eSettleTimeMS)
				{
					gTouchState[itr] = eState_WaitingForChangeToTouched;
					DebugMsg(eDbgLevel_Verbose, "MPR121: Release %d\n", itr);
					for(int itr2 = 0; itr2 < gSensorInterfaceCount; ++itr2)
					{
						gSensorInterfaceList[itr2].touchSensor->Release(itr);
					}
				}
				break;
		}
	}
}

void
MPRRegisterTouchSensor(
	int				inPort,
	ITouchSensor*	inTouchSensor)
{
	MAssert(gSensorInterfaceCount < eMaxInterfaces);

	gModuleMPR121.Initialize();

	for(int i = 0; i < gSensorInterfaceCount; ++i)
	{
		if(gSensorInterfaceList[i].touchSensor == inTouchSensor)
		{
			// Don't re register
			gSensorInterfaceList[i].port = inPort;
			return;
		}
	}

	SSensorInterface*	newInterface = gSensorInterfaceList + gSensorInterfaceCount++;
	newInterface->port = inPort;
	newInterface->touchSensor = inTouchSensor;
}

void
MPRUnRegisterTouchSensor(
	int				inPort,
	ITouchSensor*	inTouchSensor)
{
	for(int i = 0; i < gSensorInterfaceCount; ++i)
	{
		if(gSensorInterfaceList[i].touchSensor == inTouchSensor)
		{
			memmove(gSensorInterfaceList + i, gSensorInterfaceList + i + 1, (gSensorInterfaceCount - i - 1) * sizeof(SSensorInterface));
			--gSensorInterfaceCount;
			return;
		}
	}
}

void
MPRSetTouchSensitivity(
	int		inPort,
	int		inID,
	uint8_t	inSensitivity)
{
	// Enter config mode
	MPR121SetRegister(ELE_CFG, 0x00); 

	MPR121SetRegister(ELE0_T + inID * 2, inSensitivity);

	// Exist config mode
	MPR121SetRegister(ELE_CFG, 0x0C);
}

void
MPRSetReleaseSensitivity(
	int		inPort,
	int		inID,
	uint8_t	inSensitivity)
{
	// Enter config mode
	MPR121SetRegister(ELE_CFG, 0x00); 

	MPR121SetRegister(ELE0_R + inID * 2, inSensitivity);

	// Exist config mode
	MPR121SetRegister(ELE_CFG, 0x0C);
}
