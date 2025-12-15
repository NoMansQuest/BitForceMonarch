/*
 * AVR32_OptimizedTemplates.h
 *
 * Created: 19/01/2013 19:20:48
 *  Author: NASSER GHOSEIRI
 */ 

#ifndef AVR32_OPTIMIZEDTEMPLATES_H_
#define AVR32_OPTIMIZEDTEMPLATES_H_


//////////////////////////////////////////////////////////////////
// MACROS
/////////////////////////////////////////////////////////////////

#if defined(__OPERATING_FREQUENCY_32MHz__)
	#define MACRO_GetTickCount(x)  (x = (UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_TC.channel[0].cv)))
	#define MACRO_GetTickCountRet  ((UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_TC.channel[0].cv)))
	//#define MACRO_GetTickCount(x)  (x = (UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_RTC.val * 9)))
	//#define MACRO_GetTickCountRet  ((UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_RTC.val * 9 )))	
#else
	#define MACRO_GetTickCount(x)  (x = (((UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_TC.channel[0].cv))) >> 1) )
	#define MACRO_GetTickCountRet  (((UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_TC.channel[0].cv))) >> 1)
	//#define MACRO_GetTickCount(x)  (x = (((UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_RTC.val))) >> 1) )
	//#define MACRO_GetTickCountRet  (((UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_RTC.val))) >> 1)	
#endif


/////////////////////////////////////////////////////////////////
#endif /* AVR32_OPTIMIZEDTEMPLATES_H_ */