/*
 * PipeProcessingKernel.c
 *
 * Created: 03/06/2013 16:36:21
 *  Author: NASSER GHOSEIRI
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
#include "HostInteractionProtocols.h"
#include "AVR32X/AVR32_Module.h"
#include "AVR32_OptimizedTemplates.h"
#include "PipeProcessingKernel.h"

#include <string.h>
#include <stdio.h>

// Information about the result we're holding
extern buf_job_result_packet __buf_job_results[PIPE_MAX_BUFFER_DEPTH];
extern char __buf_job_results_count;  // Total of results in our __buf_job_results
static char _prev_job_was_from_pipe = FALSE;

char PipeKernel_WasPreviousJobFromPipe(void)
{
	return _prev_job_was_from_pipe;
}

// Global Variables
static job_packet    jpActiveJobBeingProcessed;
static unsigned int  iTotalEnginesFinishedActiveJob = 0;
static unsigned int  iNonceListForActualJob[8] = {0,0,0,0,0,0,0,0};
static unsigned char iNonceListForActualJob_Count = 0;
		
static job_packet	 jpInQueueJob;
static unsigned int  iTotalEnginesTakenInQueueJob = 0;
static unsigned char bInQueueJobExists = FALSE;

// Our Activity Map
static unsigned int  sEnginesActivityMap[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // 16 chips Max
		
		
// =============================================================
// Buffer Management ===========================================
// =============================================================
#define SINGLE_CHIP_QUEUE__TOOK_JOB_TO_PROCESS	1
#define SINGLE_CHIP_QUEUE__NO_JOB_TO_TAKE		2
#define SINGLE_CHIP_QUEUE__CHIP_BUSY			3
	
void PipeKernel_Spin()
{
	// Run the process for all available chips on board
	char iTotalChips = ASIC_get_chip_count();
	char iTotalTookJob = 0;
	char iTotalNoJobsAvailable = 0;
	char iTotalProcessing = 0;
	char iRetStat = 0;
	char iHover = 0;
	
	for (iHover = 0; iHover < TOTAL_SUBCHIPS_PER_BUS; iHover++)
	{
		if (!CHIP_EXISTS(iHover)) continue;
		iRetStat = Flush_buffer_into_single_chip(iHover);
		iTotalTookJob += (iRetStat == SINGLE_CHIP_QUEUE__TOOK_JOB_TO_PROCESS) ? 1 : 0;
		iTotalNoJobsAvailable += (iRetStat == SINGLE_CHIP_QUEUE__NO_JOB_TO_TAKE) ? 1 : 0;
		iTotalProcessing += (iRetStat == SINGLE_CHIP_QUEUE__CHIP_BUSY) ? 1 : 0;			
	}		
		
	// If anyone (even only a single chip) took a job, then we can consider that previous job was from pipe
	if (iTotalTookJob > 0)
	{
		// A queue job is being processed...
		_prev_job_was_from_pipe = TRUE;	
	}
		
	// We can determine if the _prev_job_was_from_queue is TRUE or FALSE
	if (iTotalNoJobsAvailable == iTotalChips)
	{
		// Meaning no job was available. We're done now...
		_prev_job_was_from_pipe = FALSE;
	}		
}	
	
char Flush_buffer_into_single_chip(char iChip)
{
	// Our flag which tells us where the previous job
	// was a P2P job processed or not :)
	// Take the job from buffer and put it here...
	// (We take element 0, and push all the arrays back...)
	if (ASIC_is_chip_processing(iChip) == TRUE)
	{
		return SINGLE_CHIP_QUEUE__CHIP_BUSY; // We won't do anything, since there isn't anything we can do
	}

	// Now if we're not processing, we MAY need to save the results if the previous job was from the pipe
	// This is conditioned on whether this chip was processing a single-job or not...
	if ((_prev_job_was_from_pipe == TRUE) && (__inprocess_SCQ_chip_processing[iChip] == TRUE)) 
	{
		// Verify the result counter is correct
		if (__buf_job_results_count == PIPE_MAX_BUFFER_DEPTH)
		{
			// Then move all items one-index back (resulting in loss of the job-result at index 0)
			for (char pIndex = 0; pIndex < PIPE_MAX_BUFFER_DEPTH - 1; pIndex += 1) // PIPE_MAX_BUFFER_DEPTH - 1 because we don't touch the last item in queue
			{
				__buf_job_results[pIndex].iProcessingChip = __buf_job_results[pIndex+1].iProcessingChip;
					
				memcpy((void*)__buf_job_results[pIndex].midstate, 	(void*)__buf_job_results[pIndex+1].midstate, 	32);
				memcpy((void*)__buf_job_results[pIndex].block_data, (void*)__buf_job_results[pIndex+1].block_data, 	12);
				memcpy((void*)__buf_job_results[pIndex].nonce_list,	(void*)__buf_job_results[pIndex+1].nonce_list,  8*sizeof(UL32)); // 8 nonces maximum
				__buf_job_results[pIndex].i_nonce_count = __buf_job_results[pIndex+1].i_nonce_count;
			}
		}

		// Read the result and put it here...
		unsigned int  i_found_nonce_list[16];
		volatile int i_found_nonce_count;
		char i_result_index_to_put = 0;
		ASIC_get_job_status(i_found_nonce_list, &i_found_nonce_count, TRUE, iChip);

		// Set the correct index
		i_result_index_to_put = (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH) ? (__buf_job_results_count) : (PIPE_MAX_BUFFER_DEPTH - 1);

		// Copy data...
		memcpy((void*)__buf_job_results[i_result_index_to_put].midstate, 	(void*)__inprocess_SCQ_midstate[iChip],		32);
		memcpy((void*)__buf_job_results[i_result_index_to_put].block_data, 	(void*)__inprocess_SCQ_blockdata[iChip],	12);
		memcpy((void*)__buf_job_results[i_result_index_to_put].nonce_list,	(void*)i_found_nonce_list,		8*sizeof(int));
		__buf_job_results[i_result_index_to_put].iProcessingChip = iChip;
		__buf_job_results[i_result_index_to_put].i_nonce_count = i_found_nonce_count;

		// Increase the result count (if possible)
		if (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH)
		{
				__buf_job_results_count++;
		}				 

		// Update timestamp on the last job result produced
		GLOBAL_LastJobResultProduced = MACRO_GetTickCountRet;
			
		// We return, since there is nothing left to do
	}

	// Ok, now we have recovered any potential results that could've been useful to us
	// Now, Are there any jobs in our pipe system? If so, we need to start processing on that right away and set _prev_job_from_pipe.
	// If not, we just clear _prev_job_from_pipe and exit
	if (JobPipe__pipe_ok_to_pop() == FALSE)
	{
		// Exit the function. Nothing is left to do here...
		__inprocess_SCQ_chip_processing[iChip] = FALSE;
		return SINGLE_CHIP_QUEUE__NO_JOB_TO_TAKE;
	}

	// We have to issue a new job...
	job_packet job_from_pipe;
	if (JobPipe__pipe_pop_job(&job_from_pipe) == PIPE_JOB_BUFFER_EMPTY)
	{
		// This is odd!!! THIS SHOULD NEVER HAPPEN, however we do set things here by default...
		__inprocess_SCQ_chip_processing[iChip] = FALSE;
		return SINGLE_CHIP_QUEUE__NO_JOB_TO_TAKE;
	}

	// Before we issue the job, we must put the correct information
	if (iChip == 7)
	{
		memcpy((void*)__inprocess_SCQ_midstate[iChip], 	(void*)job_from_pipe.midstate, 	 32);
		memcpy((void*)__inprocess_SCQ_blockdata[iChip], (void*)job_from_pipe.block_data, 12);
	}
	else
	{
		memcpy((void*)__inprocess_SCQ_blockdata[iChip], (void*)job_from_pipe.block_data, 12);			
		memcpy((void*)__inprocess_SCQ_midstate[iChip], 	(void*)job_from_pipe.midstate, 	 32);
	}
		
		
	// Send it to processing...
	ASIC_job_issue(&job_from_pipe, 0x0, 0xFFFFFFFF, TRUE, iChip, FALSE);
		
	// This chip is processing now
	__inprocess_SCQ_chip_processing[iChip] = TRUE;
		
	// The engine took a job
	return SINGLE_CHIP_QUEUE__TOOK_JOB_TO_PROCESS;	
}


// ======================================================================================
// ======================================================================================
// ======================================================================================
