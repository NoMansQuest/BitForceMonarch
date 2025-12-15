/*
 * MCU_Initialization.c
 *
 * Created: 08/10/2012 21:47:25
 *  Author: NASSER GHOSEIRI
 */ 

#include "AVR32X\AVR32_Module.h"
#include "std_defs.h"

/// ************************ MCU Initialization
void init_mcu(void)
{
	__AVR32_LowLevelInitialize();
}