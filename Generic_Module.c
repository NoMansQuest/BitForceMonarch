/*
 * Generic_Module.c
 *
 * Created: 21/11/2012 00:54:04
 *  Author: NASSER GHOSEIRI
 */ 


#include "Generic_Module.h"
#include "std_defs.h"

// Include all headers
#include "AVR32X\AVR32_Module.h"


// Now it depends which MCU we have chosen
// General MCU Functions
void MCU_LowLevelInitialize()
{
	__AVR32_LowLevelInitialize();
}

// A2D Functions
void  MCU_A2D_Initialize()
{
	__AVR32_A2D_Initialize();
}

void  MCU_A2D_SetAccess()
{
	__AVR32_A2D_SetAccess();
}

volatile int MCU_A2D_GetTemp1 ()
{
	return __AVR32_A2D_GetTemp1();
}

volatile int MCU_A2D_GetTemp2 ()
{
	volatile int iRetVal = __AVR32_A2D_GetTemp2();
	return iRetVal;
}

volatile int MCU_A2D_Get3P3V  ()
{
	return __AVR32_A2D_Get3P3V();
}

volatile int MCU_A2D_Get1V    ()
{
	return __AVR32_A2D_Get1V();
}

volatile int MCU_A2D_GetPWR_MAIN()
{
	return __AVR32_A2D_GetPWR_MAIN();
}

// USB Chip Functions
void	MCU_USB_Initialize()
{
	__AVR32_USB_Initialize();
}

void	MCU_USB_SetAccess()
{
	__AVR32_USB_SetAccess();
}

char MCU_USB_WriteData(char* iData, unsigned int iCount)
{
	return __AVR32_USB_WriteData(iData, iCount);
}

char MCU_USB_GetData(char* iData, unsigned int iMaxCount)
{
	return __AVR32_USB_GetData(iData, iMaxCount);
}

char MCU_USB_GetInformation()
{
	return __AVR32_USB_GetInformation();
}

void	MCU_USB_FlushInputData()
{
	__AVR32_USB_FlushInputData();
}

void	MCU_USB_FlushOutputData()
{
	__AVR32_USB_FlushOutputData();
}

// SC Chips
volatile void MCU_SC_Initialize()
{
	__AVR32_SC_Initialize();
}

volatile void MCU_SC_SetAccess()
{
	__AVR32_SC_SetAccess();
}

volatile unsigned int MCU_SC_GetDone(char iChip)
{
	return __AVR32_SC_GetDone(iChip);
}

volatile void __MCU_ASIC_Activate_CS(char iBank) // iBank can be 1 or 2
{
	__AVR32_ASIC_Activate_CS(iBank);
}
 
volatile void __MCU_ASIC_Deactivate_CS(char iBank)  // iBank can be 1 or 2
{
	__AVR32_ASIC_Deactivate_CS(iBank);
}

volatile unsigned int MCU_SC_ReadData(char iChip, char iEngine, unsigned char iAdrs)
{
	return __AVR32_SC_ReadData(iChip, iEngine, iAdrs);
}

inline unsigned int MCU_SC_WriteData(char iChip, char iEngine, unsigned char iAdrs, unsigned int iData)
{
	__AVR32_SC_WriteData(iChip, iEngine, iAdrs, iData);
	return TRUE;
}

// Main LED
void	MCU_MainLED_Initialize()
{
	__AVR32_MainLED_Initialize();
}

void	MCU_MainLED_Set()
{
	__AVR32_MainLED_Set();
}

void	MCU_MainLED_Reset()
{
	__AVR32_MainLED_Reset();
}

// LEDs
void	MCU_LED_Initialize()
{
	__AVR32_LED_Initialize();
}

void	MCU_LED_SetAccess()
{
	__AVR32_LED_SetAccess();
}

void	MCU_LED_Set(char iLed)
{
	__AVR32_LED_Set(iLed);
}

void	MCU_LED_Reset(char iLed)
{
	__AVR32_LED_Reset(iLed);
}

// Timer
void MCU_Timer_Initialize()
{
	__AVR32_Timer_Initialize();
}

void	MCU_Timer_SetInterval(unsigned int iPeriod)
{
	__AVR32_Timer_SetInterval(iPeriod);
}

void	MCU_Timer_Start()
{
	__AVR32_Timer_Start();
}

void	MCU_Timer_Stop()
{
	__AVR32_Timer_Stop();
}

int	 MCU_Timer_GetValue()
{
	__AVR32_Timer_GetValue();
}

// FAN unit
void	MCU_FAN_Initialize()
{
	__AVR32_FAN_Initialize();
}

void	MCU_FAN_SetAccess()
{
	__AVR32_FAN_SetAccess();
}

void	MCU_FAN_SetSpeed(char iSpeed)
{
	__AVR32_FAN_SetSpeed(iSpeed);
}