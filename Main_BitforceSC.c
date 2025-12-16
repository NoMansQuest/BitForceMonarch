/*
 *  Created on: Nov 8, 2011
 *  Author: NASSER GHOSEIRI
 */

/* BUG FIX LOG
 * ------------------------------------------------------------------------------------------------
 * July 3rd 2012 - Fixed the P2P JOB problem (range_end - range_begin, which was inverse)
 * June 2nd 2012 - Realized the system must run a 16MHz to ensure compatibility with flash SPI
 *
 */
#include "std_defs.h"
#include "MCU_Initialization.h"
#include "JobPipe_Module.h"
#include "Generic_Module.h"
#include "ChainProtocol_Module.h"
#include "USBProtocol_Module.h"
#include "A2D_Module.h"
#include "ASIC_Engine.h"
#include <avr32/io.h>
#include "JohnChengASIC.h"

#include <string.h>
#include <stdio.h>

#include "HostInteractionProtocols.h"
#include "HighLevel_Operations.h"
#include "AVR32_OptimizedTemplates.h"
#include "AVR32X/AVR32_Module.h"
#include "FAN_Subsystem.h"

// Information about the result we're holding
extern buf_job_result_packet  __buf_job_results[PIPE_MAX_BUFFER_DEPTH];
extern char 		   __buf_job_results_count;  // Total of results in our __buf_job_results

// -------------------------------------
// ========== META FEATURES ============
// -------------------------------------

void MCU_Main_Loop(void);

// -------------------------------------
// ============ END META ===============
// -------------------------------------


/////////////////////////////////////////////////////////
/// PROTOCOL
/////////////////////////////////////////////////////////
#define FAN_CONTROL_BYTE_VERY_SLOW			(FAN_CTRL3)
#define FAN_CONTROL_BYTE_SLOW				(FAN_CTRL2)
#define FAN_CONTROL_BYTE_MEDIUM				(FAN_CTRL2 | FAN_CTRL3)
#define FAN_CONTROL_BYTE_FAST				(FAN_CTRL0)
#define FAN_CONTROL_BYTE_VERY_FAST			(FAN_CTRL0 | FAN_CTRL1)

#define FAN_CONTROL_BYTE_REMAIN_FULL_SPEED	(FAN_CTRL0 | FAN_CTRL1 | FAN_CTRL2 | FAN_CTRL3)	// Turn all mosfets off...

#define FAN_CTRL0	 0b00001
#define FAN_CTRL1	 0b00010
#define FAN_CTRL2	 0b00100
#define FAN_CTRL3	 0b01000

char __bUsingUSBChannel = 0x0;

/*!
 * brief Main function. Execution starts here.
 */
int main(void)
{
	// Reset RIMA counter
	DEBUG_TotalRIMA_Count = 0;
	
	// Initialize Variables
	COMM_USING_USB1 = FALSE;
	COMM_USING_USB2 = FALSE;
	COMM_USING_PCIE = FALSE;	
		
	// Initialize everything here...
	init_mcu();

	// Continue...
	init_mcu_led();
	init_pipe_job_system();
	init_USB();
	init_FPGA();
		
	// Initialize A2D
	a2d_init();
	a2d_get_temp(0);    // This is to clear the first invalid conversion result...
	a2d_get_temp(1);    // This is to clear the first invalid conversion result...
	a2d_get_voltage(0); // This is to clear the first invalid conversion result...
	a2d_get_voltage(1); // This is to clear the first invalid conversion result...
	a2d_get_voltage(2); // This is to clear the first invalid conversion result...
	
	// Initialize timer
	MCU_Timer_Initialize();
	MCU_Timer_SetInterval(10);
	MCU_Timer_Start();
	
	// Turn on the front LED
	MCU_MainLED_Initialize();
	MCU_MainLED_Set();
	
	// Initialize flash-saving sequence
	__AVR32_Flash_Initialize();
	
	// Initialize FAN subsystem
	FAN_SUBSYS_Initialize();
	
	// Reset the total number of engines detected on startup
	GLOBAL_TotalEnginesDetectedOnStartup = 0;
	
	// Initialize total thermal cycles
	GLOBAL_TOTAL_THERMAL_CYCLES = 0;	
	
	// Last time JobIssue was called. 
	GLOBAL_LastJobIssueToAllEngines = 0;
	
	// Wait for 500ms before doing anything
	volatile unsigned int iHolder = MACRO_GetTickCountRet;
	while (MACRO_GetTickCountRet + 2 - iHolder < 500000) WATCHDOG_RESET;
	
	// Perform an ASIC GET CHIP COUNT
	init_ASIC();
	
	// Detect if we're chain master or not [MODIFY]
	// Wait for a small time
	blink_medium(); 
	WATCHDOG_RESET;
	blink_medium(); 
	WATCHDOG_RESET;
	blink_medium();
	WATCHDOG_RESET; 
	blink_medium();
	WATCHDOG_RESET; 
	blink_medium();
	WATCHDOG_RESET; 
	blink_medium();
	WATCHDOG_RESET; 
		
	/* INITIALIZE PCI EXPRESS HERE */

	// Reset global values
	global_vals[0] = 0;
	global_vals[1] = 0;
	global_vals[2] = 0;
	global_vals[3] = 0;
	global_vals[4] = 0;
	global_vals[5] = 0;

	GLOBAL_BLINK_REQUEST = 0;
	GLOBAL_PULSE_BLINK_REQUEST = 0;
	
	// Clear ASIC Results... This will make sure the diagnostic nonces are cleared
	unsigned int iNonceValues[16];
	unsigned int iNonceCount;
	ASIC_get_job_status(iNonceValues, &iNonceCount, FALSE, 0);
	
	// Did we reset the ASICs internally?
	GLOBAL_INTERNAL_ASIC_RESET_EXECUTED = FALSE;
	
	// Go to our protocol main loop
	MCU_Main_Loop();
	return(0);
}

//////////////////////////////////
//// PROTOCOL functions
//////////////////////////////////

// OK, a bit of explanation here:
// The only operation that can be in progress is Job-Handling.
// We can do one thing however, make sure that the job is not active...
// We must read the ASIC periodically until we have some response.
void MCU_Main_Loop()
{
	// Commands received from PC is 3 bytes in length
	// and will look like 'Z' + [COMMAND] + 'X'.
	//
	// Once we receive a packet like this, we must respond with 'OK'
	// and call the corresponding function.
	// The function will take it from there...
	// (PC Will send the rest of the data after that...)

	// First things first, we must clear the FTDI chip
	// Read all that you can..
	volatile int i = 10000;
	unsigned int intercepted_command_length = 0;
	
	while (USB_inbound_USB_data() && i-- > 1) USB_read_byte();

	// OK, now the memory on FTDI is empty,
	// wait for standard packet size
	char sz_cmd[2048];
	volatile unsigned int umx;
	volatile unsigned int  i_count = 0;
	
	char bTimeoutDetectedOnXLINK = 0;
	char bDeviceNotRespondedOnXLINK = 0;			
	
	for (umx = 0; umx < 1024; umx++) sz_cmd[umx] = 0;
	
	// Clear logg buffer if needed
	#if defined(__SHOW_DECOMMISSIONED_ENGINES_LOG)
		strcpy(szDecommLog,"");	
	#endif
	
	/* Turn the LED on */
	MCU_MainLED_Set();
	
	while (1)
	{
		// HighLevel Functions Spin
		Microkernel_Spin();

		// Are we using USB?
		if (__bUsingUSBChannel == TRUE)
		{
			// We listen to USB
			i = 3;
			while (!USB_inbound_USB_data() && i-- > 1);
			
			// Check, if 'i' equals zero, we discard the actual command buffer
			if (i <= 1)
			{
				// Clear buffer, something is wrong...
				i_count = 0;
				sz_cmd[0] = 0;
				sz_cmd[1] = 0;
				sz_cmd[2] = 0;
			}

			// We've reduced timeout counter to 5000, so we can run this function periodically
			// Flush the job (should they exist)
			// This must be called on a timer running 
			// Management_flush_p2p_buffer_into_engines();

			// Check if there is data, or we just had
			// an overflow?
			if (!USB_inbound_USB_data()) continue;
			
			// Was EndOfStream detected?
			intercepted_command_length = 0;
			
			volatile unsigned int bEOSDetected = FALSE;
			volatile unsigned int iExpectedPacketLength = 0;
			volatile unsigned int bInterceptingChainForwardReq = FALSE;
			volatile unsigned char bSingleStageJobIssueCommand = FALSE;
			volatile unsigned char bSingleStageMultiJobIssueCommand = FALSE;
			volatile unsigned char __expected_singlestage_multijob_length = 0xFF;

			// Read all the data that has arrived
			while (USB_inbound_USB_data() && i_count < 2048)
			{
				// Read byte
				sz_cmd[i_count] = USB_read_byte();
				
				// Are we a single-cycle job issue?
				bSingleStageJobIssueCommand = ((sz_cmd[0] == 'S') && (sz_cmd[1] == 48)) ? TRUE : FALSE;
				bSingleStageMultiJobIssueCommand = ((sz_cmd[0] == 'W') && (sz_cmd[1] == 'X')) ? TRUE : FALSE;
				
				// Are we expecting 
				bInterceptingChainForwardReq = (sz_cmd[0] == 64) ? TRUE : FALSE;
				
				if (bInterceptingChainForwardReq) 
				{
					 intercepted_command_length = (i_count + 1) - 3; // First three characters are @XY (X = Packet Size, Y = Forware Number)
					 if (i_count > 1) iExpectedPacketLength = sz_cmd[1] & 0x0FF;
				}	
				
				// Increase Count				 
				i_count++;
				
				// Determine single-stage multi-job command length
				if (i_count >= 3)
				{
					__expected_singlestage_multijob_length = sz_cmd[2] + 1 + 2; // +1 because this length does not include the stream-length byte itself
																				// +2 because it doesn't include the initial 'WX' either
				}
				
				// Check if 3-byte packet is done
				if ((i_count == 3) && (bInterceptingChainForwardReq == FALSE) && 
					(bSingleStageJobIssueCommand == FALSE) && 
					(bSingleStageMultiJobIssueCommand == FALSE))
				{
					bEOSDetected = TRUE;
					break;
				}					 
				
				if (bInterceptingChainForwardReq && (intercepted_command_length == iExpectedPacketLength))
				{
					bEOSDetected = TRUE;
					break;
				}
				
				if ((bSingleStageJobIssueCommand == TRUE) && (i_count == 48))
				{
					bEOSDetected = TRUE;
					break;
				}
				
				if ((bSingleStageMultiJobIssueCommand == TRUE) && (i_count == __expected_singlestage_multijob_length))
				{
					bEOSDetected = TRUE;
					// Set the correct intercepted_Command_length
					intercepted_command_length = i_count - 2; // Minus 2 because you need to include Field
					break;
				}				
				
				// Check if we've overlapped
				if (i_count > 256)							
				{
					// Clear buffer, something is wrong...
					i_count = 0;
					__expected_singlestage_multijob_length = 0xFF;
					sz_cmd[0] = 0;
					sz_cmd[1] = 0;
					sz_cmd[2] = 0;
					continue;
				}
				
			}	
			
			// Check for length and signature
			// If we've received less than three characters, continue waiting
			if (i_count < 3)
			{
				if (bEOSDetected == TRUE)
				{
					// Clear buffer, something is wrong...
					i_count = 0;
					__expected_singlestage_multijob_length = 0xFF;
					sz_cmd[0] = 0;
					sz_cmd[1] = 0;
					sz_cmd[2] = 0;
					continue;
				}
				else 
				{
					continue; // We'll continue...			
				}					
			}			
		}
		else 
		{
			/* We listen to PCI Express */
			// TODO: To be implemented for PCIe device
		}		
		
		// Are we a single-stage job-issue command?
		char bSingleStageJobIssueCommand = ((sz_cmd[0] == 'S') && (sz_cmd[1] == 48)) ? TRUE : FALSE; 
		char bSingleStageMultiJobIssueCommand = ((sz_cmd[0] == 'W') && (sz_cmd[1] == 'X')) ? TRUE : FALSE;
		
		// Check number of bytes received so far.
		// If they are 3, we may have a command here (4 for the AMUX Read)...
		
		// We're using USB Channel
		if (__bUsingUSBChannel == TRUE)
		{
			// Reset the count anyway
			i_count = 0;

			// Check for packet integrity
			if (((sz_cmd[0] != 'Z' || sz_cmd[2] != 'X') && 
				(sz_cmd[0] != '@')) && 
				(bSingleStageJobIssueCommand == FALSE) && 
				(bSingleStageMultiJobIssueCommand == FALSE)) // @XX means forward to XX (X must be between '0' and '9')
			{
				continue;
			}
			else
			{
				// We have a command. Check for validity
				if ((sz_cmd[0] != '@') &&
					(bSingleStageJobIssueCommand == FALSE) && 
					(bSingleStageMultiJobIssueCommand == FALSE) && 
					(sz_cmd[1] != PROTOCOL_REQ_INFO_REQUEST) &&
					(sz_cmd[1] != PROTOCOL_REQ_BUF_FLUSH_EX) &&
					(sz_cmd[1] != PROTOCOL_REQ_HANDLE_JOB) &&
					(sz_cmd[1] != PROTOCOL_REQ_ID) &&
					(sz_cmd[1] != PROTOCOL_REQ_GET_FIRMWARE_VERSION) &&
					(sz_cmd[1] != PROTOCOL_REQ_BLINK) &&
					(sz_cmd[1] != PROTOCOL_REQ_TEMPERATURE) &&
					(sz_cmd[1] != PROTOCOL_REQ_BUF_PUSH_JOB_PACK) &&
					(sz_cmd[1] != PROTOCOL_REQ_BUF_PUSH_JOB) &&
					(sz_cmd[1] != PROTOCOL_REQ_BUF_STATUS) &&
					(sz_cmd[1] != PROTOCOL_REQ_BUF_FLUSH) &&
					(sz_cmd[1] != PROTOCOL_REQ_GET_VOLTAGES) &&
					(sz_cmd[1] != PROTOCOL_REQ_GET_CHAIN_LENGTH) &&
					(sz_cmd[1] != PROTOCOL_REQ_SET_FREQ_FACTOR) &&
					(sz_cmd[1] != PROTOCOL_REQ_GET_FREQ_FACTOR) &&
					(sz_cmd[1] != PROTOCOL_REQ_SET_XLINK_ADDRESS)	&&
					(sz_cmd[1] != PROTOCOL_REQ_XLINK_ALLOW_PASS) &&
					(sz_cmd[1] != PROTOCOL_REQ_XLINK_DENY_PASS) &&
					(sz_cmd[1] != PROTOCOL_REQ_PRESENCE_DETECTION) &&
					(sz_cmd[1] != PROTOCOL_REQ_ECHO) &&
					(sz_cmd[1] != PROTOCOL_REQ_TEST_COMMAND) &&
					(sz_cmd[1] != PROTOCOL_REQ_SAVE_STRING) &&
					(sz_cmd[1] != PROTOCOL_REQ_LOAD_STRING) &&
					(sz_cmd[1] != PROTOCOL_REQ_GET_STATUS))
				{					
					if (XLINK_ARE_WE_MASTER)
					{
						USB_send_string("ERR:UNKNOWN COMMAND\n");
					}						
					else
					{
						XLINK_SLAVE_respond_transact("ERR:UNKNOWN COMMAND\n", 
													 sizeof("ERR:UNKNOWN COMMAND\n"), 
													 __XLINK_TRANSACTION_TIMEOUT__, 
													 &bDeviceNotRespondedOnXLINK, 
													 FALSE);
					}
																		 
					// Continue the loop	
					continue;
				}
				
				// We have a valid command, go call its procedure...
				if (bSingleStageJobIssueCommand)
				{
					Protocol_handle_job_single_stage(sz_cmd);
				}
				else if (bSingleStageMultiJobIssueCommand)
				{
					Protocol_MultiJob_single_stage((const char*)(sz_cmd + 2), 
													(intercepted_command_length - 2)); // +2 because the first two characters are 'W','X'
				}
				else
				{
					// Job-Issuing commands
					if (sz_cmd[1] == PROTOCOL_REQ_BUF_PUSH_JOB_PACK)    Protocol_PIPE_BUF_PUSH_PACK();
					if (sz_cmd[1] == PROTOCOL_REQ_HANDLE_JOB)			Protocol_handle_job();
					
					
					// The rest of the commands
					if (sz_cmd[1] == PROTOCOL_REQ_BUF_FLUSH)			Protocol_PIPE_BUF_FLUSH();
					if (sz_cmd[1] == PROTOCOL_REQ_BUF_FLUSH_EX)			Protocol_PIPE_BUF_FLUSH_EX();
					if (sz_cmd[1] == PROTOCOL_REQ_BUF_STATUS)			Protocol_PIPE_BUF_STATUS();
					if (sz_cmd[1] == PROTOCOL_REQ_INFO_REQUEST)			Protocol_info_request();
					if (sz_cmd[1] == PROTOCOL_REQ_ID)					Protocol_id();
					if (sz_cmd[1] == PROTOCOL_REQ_BLINK)				Protocol_Blink();
					if (sz_cmd[1] == PROTOCOL_REQ_TEMPERATURE)			Protocol_temperature();
					if (sz_cmd[1] == PROTOCOL_REQ_GET_STATUS)			Protocol_get_status();
					if (sz_cmd[1] == PROTOCOL_REQ_GET_VOLTAGES)			Protocol_get_voltages();
					if (sz_cmd[1] == PROTOCOL_REQ_GET_FIRMWARE_VERSION)	Protocol_get_firmware_version();
					if (sz_cmd[1] == PROTOCOL_REQ_SET_FREQ_FACTOR)		Protocol_set_freq_factor();
					if (sz_cmd[1] == PROTOCOL_REQ_ECHO)					Protocol_Echo();
					if (sz_cmd[1] == PROTOCOL_REQ_TEST_COMMAND)			Protocol_Test_Command();
					if (sz_cmd[1] == PROTOCOL_REQ_PRESENCE_DETECTION)   Protocol_xlink_presence_detection();
				}
				
				// Once we reach here, our procedure has run and we're back to standby...
			}
		}
		else // We are using PCI Express
		{
			// Take appropriate action on PCI Express Side
			// TODO: To be implemented for PCIe device
		}		
	}
}

