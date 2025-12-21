## Butterfly Labs BitForce Monarch Series - AVR32 Firmware
AVR32 Firmware for Butterfly Labs Bitforce Monarch double-SHA processors

* Author: Nasser Ghoseiri
* Date: `17-Dec-2025`

---

### Introduction

BitForce Monarch was the last of BitForce series (doulbe-SHA hashing accelerators) released by Butterfly Labs based on Global Foundry's 28nm node back in 2013 to 2014. The system was composed of two high-speed double-SHA ASIC processor, one AVR32 microprocessor and one Xilinx (now AMD) Spartan-6 FPGA.

The FPGA fulfilled the following roles:
* Connect the AVR32 chip to USB bus (via FTDI)
* Support PCI express connection to the host system
* Provide SPI co-processors managing individual hashing engines in ASICs
* Allow the AVR32 to driver other components on the board (such as ISL6307 VID, temperature sensors, etc.)

The AVR32 (which this firmware is about) was essentially managing the system via FPGA. Although the same functionality could have been achieved by the FPGA without the AVR32 intervention, a number factors favored the presence of AVR32:

* Ones less element exposed to FPGA bitstream design.
* In order to accelerate the development phase, the AVR32 had a number of advantageous:

    1. It had better IDE support (the AVR studio)
    2. More comprehensive coding framework available
    3. The MCU would have been powered up before the FPGA and could control its lifecycle.
    4. It depended on fewer voltage rails, increasing the odds of prototype success should one rail fail to pass the smoke test.
    5. It offered integrated Analog-to-Digital converter.

* A portion of the BitForce Single series could have been ported and reused. 

Although the intention was to eventually have the MCU fully integrated into the FPGA using MicroBlaze soft-core processor, the product sadly never reached that stage.

### Startup And Mode Of Operation

The program begins in 'Main_BitforceSC.c' and undertakes the following actions:

- Initialization of the MCU
- Initialization of the USB channel
- Initialization of the LEDs
- Initialization of the Job Pipe System (this is internal to the firmware)
- Initialization of Analog-to-digital converters
- Initialization of system timers
- Initialization of the fan subsystem (the AVR32 controls their velocity)

The firmware not based on any RTOS for performance and reliability reasons, and employs a cooperative kernel to run the system. Once the initialization is complete, the system is ready to start the main kernel, and all is left is to flush ASIC engines from any leftover result they may contain (prior to a reboot for instance?).

Once the ASIC engines are reset, the main kernel loop takes over. The objective is to repeatedly check the USB (or PCI-Express bus) for incoming commands to process and perform house-keeping operation (undertaken by 'Microkernel_Spin' function). These commands provide numerous functionality (such as jobs to process, read back of results, etc.). The full list of commands are provided as following:

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

A command a processes as soon as it is received. As indicated earlier, the kernel loop also performs device related operations (reading results, disabling unresponsive hashing engines), updating fan performance based on velocity, etc. (all part of **Microkernel_Spin** call found in 'HighLevel_Operations.c').

!!!note To ensure non-interrupted operation of this double-SHA system, the internal watchdog of the micro-controller is active, and in the event of a crash (which no documented case exists to this day), will safely reboot the system to bring it back to its functional sate.

### The 'A2D_Module'

The module is in charge of reading voltages and temperature via AVR32's internal A2D. Three voltage rails are read:

1) The 1.0V: This is to primarily power the FPGA 
2) The 3.3V: This power rail supplies the majority of the components on the board
3) The Main: This is the main input.

!!!note The main input is expected to be around 12 volts. The design does support inputs as low as 7 volts and as high as 18 volts, although lower voltages will increase loses in the main VRMs (each supply about 150Amps @ 0.6V to ASICs) and may decrease efficiency as increased current may heat up internal power planes, and higher voltages may increase the risk of input capacitor failure.

### The 'ASIC_Engine'

This module is in charge of operating the 28nm double-SHA ASICs. This includes chip diagnostics and testing, job issuance, result read-back, fine-tuning their operating frequency and presence check (due to manufacturing defects, some engines were non-operational).

Although in the original design the intention was to have the AVR32 directly communicate with the ASICs (and FPGA act as a pass-through switch), certain reliability issus in the ASICs (unreliable 'write-to-all' operation) forced a major design change, under which the FPGA become the intermediate agent sitting between the ASICs and the AVR32. 

Given how dense these chips were (5B+ transistors, ~1000 double-SHA engines per chip), the AVR32's single SPI bus was not performant enough to service all of these double-SHA hashing engines. This lead to the implementation of custom micro-processors inside the FPGA per SPI bus (each SPI bus serviced a good number of double-SHA engines on the chip, and each ASIC contains multiple SPI busses) to manage their operations, essentially off-loading certain functionalities from the AVR32 to FPGA itself.

### The 'AVR32_OptimizedTemplates'

This module contains heavily optimized (assembly level) AVR32 code to rapidly communicate with the CPLD in charge of managing X-Link. Since X-LInk was not shipped with these product, this module serves no purpose.

### The 'FAN_Subsystem'


















