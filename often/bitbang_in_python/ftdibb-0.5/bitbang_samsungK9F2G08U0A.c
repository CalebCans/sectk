#include <stdio.h>
#include <unistd.h>
#include <usb.h>
#include "ftdibb.h"
#ifdef linux
#include <sys/types.h>
#include <stdlib.h>
#endif

// these are the bit definitions for the lines which are connected:
#define PIN0 1
#define PIN1 2
#define PIN2 4
#define PIN3 8
#define PIN4 16
#define PIN5 32
#define PIN6 64
#define PIN7 128

// specify logic on and off for target
#define HIGH = 1
#define LOW = 0
// the logic is reversed from the eletrical state:
#define OFF = 1
#define ON = 0
/*
 2112Mbit in 131072 rows (pages). "The K9F2G08X0A has addresses multiplexed into 8 I/Os. This scheme dramatically reduces pin counts". Command, address and data are all written through I/O's by 
 bringing WE to low while CE is low. 1.8 to 3.3v

 Pins: 8 for io (data and address).  Command enable, address enable, chip select, WE, read enable and WP.  13
 
start:
 CE = HIGH										// Chip Enable, Chip Select?
 OE = HIGH										// Output Enable
 WE = HIGH and remains so for all stages		// Write Enable
 DATA
 BYTE = LOW
 RP = LOW										// Reset Power

In english:
 Reset Power and Write Enable are are off (RP high, WE high) for reading.  
 Chip Select pins asserted (LOW) to select flash page, Output Enable pin 
 is asserted (LOW) and then the addressed data is sent to IO.
 
In order:
1. RP = H (reset and sync timing)
2. Put address
3. CE = L
4. OE = L
5. DATA is read
5.6. BYTEF = H
 
Glossary
RP 
 RESET/ POWER-DOWN: RP#-low resets internal automation and puts the device in power- 
 down mode. RP#-high enables normal operation. Exit from reset sets the device to read array 
 mode. When driven low, RP# inhibits write operations which provides data protection during 
 power transitions.
OE# 
 OUTPUT ENABLE: Activates the device’s outputs through the data buffers during a read cycle. 
 OE# is active low.
WE# 
 Input WRITE ENABLE: Controls writes to the CUI, the Write Buffer, and array blocks. WE# is active 
 low. Addresses and data are latched on the rising edge of WE#.
CE0, CE1, CE2 
 CHIP ENABLES: Activates the device’s control logic, input buffers, decoders, and sense 
 amplifiers. When the device is de-selected (see Table 13 on page33), power reduces to standby 
 levels. 
 All timing specifications are the same for these three signals. Device selection occurs with the 
 first edge of CE0, CE1, or CE2 that enables the device. Device deselection occurs with the first 
 edge of CE0, CE1, or CE2 that disables the device (see Table 13 on page33). 
 A0 Input This address is latched during a x8 program cycle. Not used in x16 mode (i.e., the A0 input buffer 
 is turned off when BYTE# is high). 
A[MAX:1] Input 
 ADDRESS INPUTS: Inputs for addresses during read and program operations. Addresses are 
 internally latched during a program cycle. 
 32-Mbit: A[21:0] 
 64-Mbit: A[22:0] 
 128-Mbit: A[23:0] 
 256-Mbit: A[24:0] 
D[7:0] Input/Output commands during CUI writes. Outputs array, CFI, identifier, or status data in the appropriate read 
 mode. Data is internally latched during write operations. 
D[15:8] Input/Output HIGH-BYTE DATA BUS: Inputs data during x16 buffer writes and programming operations. 
 Outputs array, CFI, or identifier data in the appropriate read mode; not used for Status Register 
 BYTE# Input 
 BYTE ENABLE: BYTE#-low places the device in x8 mode; data is input or output on D[7:0], 
 while D[15:8] is placed in High-Z. Address A0 selects between the high and low byte. BYTE#- 
 high places the device in x16 mode, and turns off the A0 input buffer. Address A1 becomes the 
 lowest-order address bit. 
 reads. Data is internally latched during write operations in x16 mode. D[15-8] float in x8 mode 
BYTE# Input 
 BYTE ENABLE: BYTE#-low places the device in x8 mode; data is input or output on D[7:0], 
 while D[15:8] is placed in High-Z. Address A0 selects between the high and low byte. BYTE#- 
 high places the device in x1lowest-order address bit. 
 
 
 */
int main(int argc, char **argv)
{
	usb_dev_handle *dev_handle;
	unsigned char selpin;
	int i=10;
#ifdef linux
	if(geteuid()!=0){
		fprintf(stderr,"ERROR: this program must run with effective uid=root.\n");
		/* you can do this:
		 * chown root.root bitbang_example
		 * chmod 4755 bitbang_example
		 * ls -al bitbang_example
		 * -rwsr-xr-x    1 root     root        11832 Aug 11 18:53 bitbang_example
                 * now you can run as normal user.
		 */
		exit(1);
	}
	// set also the real uid to be able to run rmmod
	if (setuid(0)==0){
		system("/sbin/rmmod ftdi_sio > /dev/null 2>&1");
	}else{
		fprintf(stderr,"Warning: suid root failed\n");
	}
#endif
	
	dev_handle = ftdibb_open(0x0403, 0x6001,0);

	if (dev_handle==NULL){
		exit(1);
	}
	if (ftdibb_setBaudRate(dev_handle,9600)!=0){
		exit(2);
	}
	// set which lines are output, the rest is input:
	if (ftdibb_setBitMode(dev_handle,PIN1|PIN2|PIN3, 1)!=0){
		exit(3);
	}
	printf("bitbang mode on\n");
	while(i){
		printf("turning PIN1|PIN2|PIN3 on\n");
		selpin=PIN1|PIN2|PIN3;
		ftdibb_writeDatabyte(dev_handle,selpin);
		printf("Input PIN4 is %u\n",ftdibb_getpin(dev_handle,PIN4));
		fflush(stdout);
		fflush(stderr);
		sleep(1);

		printf("turning everything off\n");
		selpin=0;
		ftdibb_writeDatabyte(dev_handle,selpin);
		printf("Input PIN4 is %u\n",ftdibb_getpin(dev_handle,PIN4));
		fflush(stdout);
		fflush(stderr);
		sleep(1);
		i--;
	}


	printf("disabling bitbang mode\n");
	if (ftdibb_setBitMode(dev_handle,0xff, 0)!=0){
		exit(2);
	}
	ftdibb_close(dev_handle);
	return(0);
}
