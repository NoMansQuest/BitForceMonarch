# BitForceMonarch
AVR32 Firmware for Butterfly Labs Bitforce Monarch double-SHA processors

## Introduction

BitForce Monarch was the last of BitForce series (doulbe-SHA hashing accelerators) released by Butterfly Labs based on Global Foundry's 28nm node back in 2013 to 2014. The system was composed of two high-speed double-SHA ASIC processor, one AVR32 microprocessor and one Xilinx (now AMD) Spartant-6 FPGA.

The FPGA fulfilled the following roles:
* Connect the AVR32 chip to USB bus (via FTDI)
* Support PCI express connection to the host system
* Provide SPI co-processors managing invidivual hashing engines in ASICs
* Allow the AVR32 to driver other components on the board (such as ISL6307 VID, temperature sensors, etc.)

The AVR32 (which this firmware is about) was essentially managing the system via FPGA. Although the same functionality could have been achieved by the FPGA without the AVR32 intervention, a number factors favored the presence of AVR32:

* Ones less element exposed to FPGA bitstream design.
* In order to accelerate the development phase, the AVR32 had a number of advantagous:

    1. It had better IDE support (the AVR studio)
    2. More comprehensive coding framework available
    3. The MCU would have been powered up before the FPGA and could control its lifecycle.
    4. It depended on fewer voltage rails, increasing the odds of prototype success should one rail fail to pass the smoke test.
    5. It offered integrated Analog-to-Digital converter.

* A portion of the BitForce Single series could have been ported and reused. 

Although the intention was to eventually have the MCU fully integrated into the FPGA using MicroBlaze soft-core processor, the product sadly never reached that stage.

## Mode of operation

The program begins in 'Main_BitforceSC.c' and undertakes the following actions:

- Initialization of the MCU
- Initialization of the USB channel
- Initialization of the LEDs
- Initialization of the Job Pipe System (this is internal to the firmware)
- Initialization of Analog-to-digital converters
- Initialization of system timers
- Initialization of the fan subsystem (the AVR32 controls their velocity)

At this point, the system is ready to start the main kernel, and all is left is to flush ASIC engines from any leftover result they may containt (prior to a reboot for instance?).

Once the ASIC engines are reset, the main kernel loop takes over. The objective is to repetively check the USB (or PCI-Express bus) for incoming commands to process. These commands provide numerous functionality (such as jobs to process, read back of results, etc.). The full list of commands are provided as following:

| Command Name                      | Parameters        | Description                                                                                                                                   |
| :-------------------------------- | :---------------- | :-------------------------------------------------------------------------------------------------------------------------------------------- |
| PROTOCOL_REQ_INFO_REQUEST         | None              | Returned a full multi-line description of the device, along with operating engines, operating conditions, etc.                                |
| PROTOCOL_REQ_BUF_FLUSH_EX         | None              | Flushes the job-pipe buffers, purging all unread results                                                                                      |
| PROTOCOL_REQ_HANDLE_JOB           | 1024-byte payload | Issues a single job to the unit for processing. The request (once acknowledged) must be followed up by a fixed 1024-byte payload.             |
| PROTOCOL_REQ_ID                   | None              | Returns the unique ID of the device.                                                                                                          |
| PROTOCOL_REQ_GET_FIRMWARE_VERSION | None              | Reports back the firmware version of the device (also available via 'PROTOCOL_REQ_INFO_REQUEST').                                             |
| PROTOCOL_REQ_BLINK                | None              | Causes the device to blink for 30 seconds. This is meant to aid an operator to locate the device in large installations.                      |
| PROTOCOL_REQ_TEMPERATURE          | None              | Returns the temperature reported by the two sensors near the VRM phases.                                                                      |
| PROTOCOL_REQ_BUF_PUSH_JOB_PACK    | 1024-byte payload | Issues a maximum of five jobs to the device (contrary to 'PROTOCOL_REQ_HANDLE_JOB' which only allows a single job to be processed).           |
| PROTOCOL_REQ_BUF_PUSH_JOB         | None              | Reserved, not active used.                                                                                                                    |
| PROTOCOL_REQ_BUF_STATUS           | None              | Reports back the result of the most recently processed job by the device.                                                                     |
| PROTOCOL_REQ_BUF_FLUSH            | None              | Flushes the job buffer.                                                                                                                       |
| PROTOCOL_REQ_GET_VOLTAGES         | None              | Returns the voltages read by the A2D converters. This mostly serves diagnostic purposes.                                                      |
| PROTOCOL_REQ_GET_CHAIN_LENGTH     | Deprecated        | Reports the number of devices present in an X-Link chain. [[Deprecated]].                                                                     |
| PROTOCOL_REQ_SET_FREQ_FACTOR      | 1024-byte payload | Sets the frequency-factor of the ASIC chips,altering their frequency to some extent. The first 4 bytes of the payload constitutes the factor. |
| PROTOCOL_REQ_GET_FREQ_FACTOR      | None              | Reports back the current frequency factor in use.                                                                                             |
| PROTOCOL_REQ_SET_XLINK_ADDRESS    | Deprecated        | Sets the X-Link address of this device. Given that X-Link was never materialized, this command is effectively unused.                         |
| PROTOCOL_REQ_XLINK_ALLOW_PASS     | Deprecated        | Allows X-Link messages to travel down the chain. [[Deprecated]]                                                                               |
| PROTOCOL_REQ_XLINK_DENY_PASS      | Deprecated        | Prevents X-Link messages from traveling down the chain. [[Deprecated]]                                                                        |
| PROTOCOL_REQ_PRESENCE_DETECTION   | Deprecated        | X-Link presence detection (i.e. bus-scan). [[Deprecated]]                                                                                     |
| PROTOCOL_REQ_ECHO                 | None              | Responds to ECHO, serves mainly as presence check.                                                                                            |
| PROTOCOL_REQ_TEST_COMMAND         | None              | Comprehensive test performed on the device for development purposes. Unused for normal operation.                                             |
| PROTOCOL_REQ_SAVE_STRING          | 1024-byte payload | Saves a 255-byte long string to the device which could be read back via 'PROTOCOL_REQ_LOAD_STRING'.                                           |
| PROTOCOL_REQ_LOAD_STRING          | None              | Returns the string previously saved on the device via 'PROTOCOL_REQ_SAVE_STRING'.                                                             |
| PROTOCOL_REQ_GET_STATUS           | None              | Reports the current state of the device: BUSY, IDLE, NONCE-FOUND                                                                              |








