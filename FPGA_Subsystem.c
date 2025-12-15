/*
 * FPGA_Subsystem.c
 *
 * Created: 11/29/2013 1:08:00 PM
 *  Author: Nasser
 */ 

#include "FPGA_Subsystem.h"
#include "std_defs.h"
#include "Generic_Module.h"
#include "AVR32_OptimizedTemplates.h"
#include "AVR32X/AVR32_Module.h"
#include "avr32/io.h"


volatile void FPGA_Initialize()
{
	// To be implemented for PCI-Express intended device.
}

volatile char FPGA_Is_Present()
{
	// To be implemented for PCI-Express intended device.
}

volatile void FPGA_Select_Bus(unsigned char iChannel)
{
	// To be implemented for PCI-Express intended device.
}

volatile void FPGA_Write(unsigned __int16 iAddress, unsigned __int32* iBuf, unsigned int iLength)
{
	// To be implemented for PCI-Express intended device.
}

volatile void FPGA_Read(unsigned  __int16 iAddress, unsigned __int32* iBuf, unsigned int iLength)
{
	// To be implemented for PCI-Express intended device.
}


// PCI Express related functions
volatile char FPGA_PCIE_ResponseBufferReady(void)
{
	
	// ALL OK, it means we have buffer
	return TRUE;
}

volatile char FPGA_PCIE_SendResponse(char* szData, int iLen)
{
	// If buffer is too big, we return an error
	if (iLen > MAXIMUM_PCIE_RESPONSE_LENGTH) return PCIE_RESP_STREAM_TOO_BIG;
	if (iLen == 0) return PCIE_RESP_ZERO_LENGTH;
	
	// Copy the buffer
	FPGA_Write(FPGA_MEM_ADDRESS_RESPONSE_RAM, (unsigned __int32*)szData, iLen);	
	return PCIE_RESP_OK;
}

volatile char FPGA_PCIE_SendResponseWithWait(char* szData, int iLen, int iWaitTime)
{
	// If buffer is too big, we return an error
	if (iLen > MAXIMUM_PCIE_RESPONSE_LENGTH) return PCIE_RESP_STREAM_TOO_BIG;
	if (iLen == 0) return PCIE_RESP_ZERO_LENGTH;
	
	// Wait until 	
	volatile unsigned int iActualTime = MACRO_GetTickCountRet;
	
	while ((MACRO_GetTickCountRet-iActualTime) < iWaitTime)
	{
		WATCHDOG_RESET;
		if (FPGA_PCIE_ResponseBufferReady() == FALSE) continue;
		break;
	}
	
	// Did we timeout?
	if (FPGA_PCIE_ResponseBufferReady() == FALSE) return PCIE_RESP_TIMEOUT;
	
	// We have the buffer available...
	FPGA_Write(FPGA_MEM_ADDRESS_RESPONSE_RAM, (unsigned int*)szData, iLen);
	return PCIE_RESP_OK;		
}
