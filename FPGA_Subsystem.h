/*
 * FPGA_Subsystem.h
 *
 * Created: 11/29/2013 1:07:44 PM
 *  Author: Nasser
 */ 

#ifndef FPGA_SUBSYSTEM_H_
#define FPGA_SUBSYSTEM_H_

#define  init_FPGA FPGA_Initialize

volatile void FPGA_Initialize(void);
volatile char FPGA_Is_Present(void);
volatile void FPGA_Select_Bus(unsigned char iChannel);
volatile void FPGA_Write(unsigned __int16_t iAddress, unsigned int* iBuf, unsigned int iLength);
volatile void FPGA_Read (unsigned __int16_t iAddress, unsigned int* iBuf, unsigned int iLength);

// FPGA Memory Addresses
#define FPGA_MEM_ADDRESS_RESPONSE_RAM
#define FPGA_MEM_ADDRESS_DATA_RAM
#define FPGA_MEM_ADDRESS_TEMPERATURE
#define FPGA_MEM_ADDRESS_COMMAND
#define FPGA_MEM_ADDRESS_STATUS

// PCI Express related functions
#define PCIE_RESP_BUFFER_FULL		3
#define PCIE_RESP_STREAM_TOO_BIG	2
#define PCIE_RESP_ZERO_LENGTH		1
#define PCIE_RESP_OK				0
#define PCIE_RESP_TIMEOUT			4

#define MAXIMUM_PCIE_RESPONSE_LENGTH 4096 

volatile unsigned int FPGA_PCIE_GetCommandRegister(void);
volatile char FPGA_PCIE_IsCommandReady(void);
volatile char FPGA_PCIE_IsCommandDataReady(void);
volatile int  FPGA_PCIE_CommandDataLength(void);
volatile char FPGA_PCIE_GetCommandData(unsigned __int32_t *szData, int *iCommandDataLen);

volatile char FPGA_PCIE_ResponseBufferReady(void);
volatile char FPGA_PCIE_SendResponse(char* szData, int iLen);
volatile char FPGA_PCIE_SendResponseWithWait(char* szData, int iLen, int iWaitTime/*=1000*/);


#endif /* FPGA_SUBSYSTEM_H_ */