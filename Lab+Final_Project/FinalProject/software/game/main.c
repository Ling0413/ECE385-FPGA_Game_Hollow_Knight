/*---------------------------------------------------------------------------
  --      main.c                                                    	   --
  --      Christine Chen                                                   --
  --      Ref. DE2-115 Demonstrations by Terasic Technologies Inc.         --
  --      Fall 2014                                                        --
  --                                                                       --
  --      For use with ECE 298 Experiment 7                                --
  --      UIUC ECE Department                                              --
  ---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

#include "system.h"
#include "alt_types.h"
#include <unistd.h>  // usleep 
#include "sys/alt_irq.h"
#include "io_handler.h"

#include "cy7c67200.h"
#include "usb.h"
#include "lcp_cmd.h"
#include "lcp_data.h"

#include "boss.h"
#include "character.h"


//----------------------------------------------------------------------------------------//
//
//                                Sub-function defination
//
//----------------------------------------------------------------------------------------//




void read_keycode(alt_u8 data_size, alt_u16 * usb_ctl_val_d, alt_u8 toggle, char * keycodes)
{	
	unsigned int temp = 0;
	int done_flag = 0;

	IO_write(HPI_ADDR,0x0500); //the start address
	//data phase IN-1
	IO_write(HPI_DATA,0x051c); //500

	IO_write(HPI_DATA,0x000f & data_size);//2 data length

	IO_write(HPI_DATA,0x0291);//4 //endpoint 1
	if(toggle%2)
	{
		IO_write(HPI_DATA,0x0001);//6 //data 1
	}
	else
	{
		IO_write(HPI_DATA,0x0041);//6 //data 1
	}
	IO_write(HPI_DATA,0x0013);//8
	IO_write(HPI_DATA,0x0000);//a
	UsbWrite(HUSB_SIE1_pCurrentTDPtr,0x0500); //HUSB_SIE1_pCurrentTDPtr
	
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		if (done_flag == 0) {
			IO_write(HPI_ADDR,0x0500); //the start address
			//data phase IN-1
			IO_write(HPI_DATA,0x051c); //500

			IO_write(HPI_DATA,0x000f & data_size);//2 data length

			IO_write(HPI_DATA,0x0291);//4 //endpoint 1
			if(toggle%2)
			{
				IO_write(HPI_DATA,0x0001);//6 //data 1
			}
			else
			{
				IO_write(HPI_DATA,0x0041);//6 //data 1
			}
			IO_write(HPI_DATA,0x0013);//8
			IO_write(HPI_DATA,0x0000);//
			UsbWrite(HUSB_SIE1_pCurrentTDPtr,0x0500); //HUSB_SIE1_pCurrentTDPtr
			done_flag = 1;
		}
	}//end while

	*usb_ctl_val_d = UsbWaitTDListDone();
	
	// The first two keycodes are stored in 0x051E. Other keycodes are in 
	// subsequent addresses.
	temp = UsbRead(0x051e);
	keycodes[3] = temp & 0x00ff;
	keycodes[2] = (temp >> 8) & 0x00ff;
	temp = UsbRead(0x0520);
	keycodes[1] = temp & 0x00ff;
	keycodes[0] = (temp >> 8) & 0x00ff;
	
	// Re-order the list
	for (int i = 0; i < 4; i++) {
		if ((keycodes[3] == 0) && (keycodes[2] == 0) && (keycodes[1] == 0) && (keycodes[0] == 0)) {
			break;
		}
		else if (keycodes[0] != 0) {
			break;
		}
		else {
			keycodes[0] = keycodes[1];
			keycodes[1] = keycodes[2];
			keycodes[2] = keycodes[3];
			keycodes[3] = 0;
		}
	}
}



void game_main(alt_u8 data_size, alt_u16 usb_ctl_val)
{
	
	/* Parameters for keyboard reading
	 */ 
	alt_u8 toggle = 0;
	alt_u16 * usb_ctl_val_d ;
	*usb_ctl_val_d = usb_ctl_val;
	char keycodes[4] = {0x0, 0x0, 0x0, 0x0};	// Currently pressed 4 keycode, keycode[0] is the latest pressed key

	/* List of 16 spirits
	 */ 
	long long unsigned int spirit[16] = {
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
	};

	/*	State of character and boss
	 *  X[0] - x coordinate in screen
	 * 	X[1] - y coordinate in screen
	 *  X[2] - x velocity
	 *  X[3] - y velocity
	 *  X[4] - orientation (0-left, 1-right)
	 *  X[5] - x center
	 *  X[6] - y center
	 *  X[7] - x offset
	 *  X[8] - y offset
	 *  X[9] - x center offset
	 *  X[10] - y center offset
	 *  X[11] - health point
	 *  -------------------------------------
	 *  For boss:
	 *  X[12] - next skill to be used (1-dash1; 2-dash2; 3-spine), reset after 3 is complete
	 *        - not necessarily to follow this order
	 *  X[13] - spark flag, 1 means a park has just been performed
	 *  X[14] - 
	 *  X[15] - 
	 */
	int state_c[12] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	int state_b[16] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

	/* Event list
	 * X[0] - {0(character)/1(boss), unique identifer}
	 * X[1] - {number of frames, prohibition vector}
	 *      - prohibition vector: counter from left to right, i-th bit (start from 0) correspods to the i-th index prohibition list
	 * X[i>1] - {frame counter, address and off set information}
	 */ 
	long long unsigned int event_c[12][6] = {
		{0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, 
		{0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, 
		{0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
	};

	long long unsigned int event_b[12][6] = {
		{0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, 
		{0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, 
		{0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
	};

	long long unsigned int event_s[12][6] = {
		{0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, 
		{0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, 
		{0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
	};


	/*	Prohibition list 
	 *  ----------------------------------------------------------------------
	 *  1 means modification of corresponding parameter is prohibited or the 
	 *  event is not allowed to interrupt the current event (even if all other 
	 *  conditions are met), 0 means free.
	 *  ----------------------------------------------------------------------
	 *  X[0] - x coordinate in screen
	 * 	X[1] - y coordinate in screen
	 *  X[2] - x velocity
	 *  X[3] - y velocity
	 *  --------------------------------
	 *  X[4] - orientation
	 *  X[5] - Cenx
	 *  X[6] - Ceny
	 *  X[7] - event_stand
	 *  --------------------------------
	 *  X[8] - event_walk
	 *  X[9] - event_jump
	 *  X[10] - event_jump2
	 *  X[11] - event_attack
	 *  --------------------------------
	 *  X[12] - event_fall
	 *  X[13] - event_dash
	 *  X[14] - 
	 *  X[15] - health
	 *  ----------------------------------------------------------------------
	 * 	For boss we have
	 *  X[7] - event_stand
	 *  --------------------------------
	 *  X[8] - 
	 *  X[9] - 
	 *  X[10] -
	 *  X[11] - event_spark
	 *  --------------------------------
	 *  X[12] - event_spine
	 *  X[13] - event_dash
	 *  X[14] - event_explosion
	 *  X[15] - health
	 */
	int prohibit_c[16] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	int prohibit_b[16] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	
	
	/*  Cooling counter
	 *  X[0] - attack
	 *  X[1] - dash
	 *  X[2] - boss stiffness
	 */
	int cooling_c[5] = {0x0, 0x0, 0x0, 0x0, 0x0};

	/*  System flage
	 *  X[0] - in air
	 *  X[1] - dash
	 *  X[2] - spark orientation flag, 0 means to left, 1 means to right
	 *  X[3] - spell
	 *  X[4] - double jump
	 *  X[5] - jump pressed
	 */
	int system_flage[6] = {0x0, 0x0, 0x0, 0x0, 0x0};

	// System clock
	int clock = 0;

	// Data from hardware
	unsigned int * data;
	int is_cs;
	int is_cb;
	int reset;
	int GoldenFinger;
	long long unsigned int temp;

	*data = 0x0;

	// Initialization
	initial(spirit, state_c, state_b, event_c, event_b, event_s, prohibit_c, prohibit_b, cooling_c, system_flage);

	// Main loop
	while (1==1) {
		// Update the clock signal
		clock = (clock + 1) % 2;

		// Read data from hardware
		reset = *data & 0x0001;
		GoldenFinger = *data & 0x0004;
		is_cs = *data & 0x0008;
		is_cb = *data & 0x0010;
		// Check reset condition
		if (reset != 0 || state_c[Health] == 0 || state_b[Health] == 0) {
			usleep(500000);
			initial(spirit, state_c, state_b, event_c, event_b, event_s, prohibit_c, prohibit_b, cooling_c, system_flage);
			clock = 0;
		}
		// Check golden finger
		if (event_c[0][id] != 5) {
			if (GoldenFinger != 0) {
				prohibit_c[p_health] = 1;
			} else {
				prohibit_c[p_health] = 0;
			}
		}
		
		// Check contact condition
		// If contact with spell
		if (is_cs != 0) {	
			if ((event_s[0][id] != 0) && (event_s[0][fcount] != 1) && (event_s[0][fcount] != 2)) {	// Not prelude
				if (prohibit_c[p_health] == 0 && cooling_c[c_hitc] == 0 && cooling_c[c_attack] == 0) {
					state_c[Health] -= 1;
					cooling_c[c_hitc] = 90;
				}
			}
		} 
		// If contact with boss
		if (is_cb != 0) {	
			if (event_c[0][id] == 5 && cooling_c[c_hitb] == 0) {	// If current event is attack
				state_b[Health] -= 1;
				cooling_c[c_hitb] = 9;
			} else {
				if (prohibit_c[p_health] == 0 && cooling_c[c_hitc] == 0 && cooling_c[c_attack] == 0) {
					state_c[Health] -= 1;
					cooling_c[c_hitc] = 90;
				}
			}
		}

		// Scan current keypresses
		toggle++;
		read_keycode(data_size, usb_ctl_val_d, toggle, keycodes);

		// Run character control functions
		update_event(state_c, event_c, prohibit_c, cooling_c, keycodes, clock, system_flage);
		change_event(state_c, event_c, prohibit_c, cooling_c, keycodes, clock, system_flage);
		movement(state_c, event_c, prohibit_c, cooling_c, keycodes, clock, system_flage);

		// Run boss control functions
		update_event_b(state_b, state_c, event_b, prohibit_b, clock, system_flage, cooling_c);
		change_event_b(state_b, state_c, event_b, prohibit_b, clock, system_flage, cooling_c);
		movement_b(state_b, event_b, prohibit_b, clock, system_flage);

		// Spell generation
		if (system_flage[f_spell] == 1) {
			set_event(event_s, event_s_spine);
			system_flage[f_spell] = 0;
		}

		// Set the value of coordinate for each spirit
		set_state(state_c[Cx], state_c[Cy], state_c[Ori], spirit, 0);
		set_state(state_b[Cx], state_b[Cy], state_b[Ori], spirit, 1);

		// Set the address and offset information each spirit
		set_address(event_c[event_c[0][fcount]][1], spirit, 0);
		set_address(event_b[event_b[0][fcount]][1], spirit, 1);

		// Update the event and set the coordinate, address and offset for each spirit
		if (event_s[0][id] != 0) {
			int index = event_s[0][fcount];
			// If counter of current frame is zero
			if (event_s[index][0] == 0) {
				if (index >= event_s[0][fnum]) {
					clean_event(event_s);
				} else {
					// Current frame ended
					event_s[0][fcount] += 1;
				}
			}
			// Subtract the counter of current frame by 1
			event_s[index][0] -= 1;
			// Display the spell
			for (int i = 0; i < 10; i++) {
				set_state(71*i, 165, 0, spirit, i+2);
				set_address(event_s_spine[index][1], spirit, i+2);
			}
		} else {
			for (int i = 0; i < 10; i++) {
				spirit[i+2] = 0x0;
			}
		}

		// Update cooling list
		for (int i = 0; i < N_cooling_c; i++) {
			if (cooling_c[i] > 0) {
				cooling_c[i] -= 1;
			}
		}

		// Update healthy point
		temp = 0x0;
		for (int i = 0; i < state_c[Health]; i++) {
			temp = (temp << 1) | 0x0001;
		}
		spirit[15] = (spirit[15] & 0xffff00ffffffffff) | (temp << 40);

		// Blinking during cooling period
		if ((cooling_c[c_hitc] != 0) && ((cooling_c[c_hitc] / 10) % 2 == 1)) {
			spirit[0] = 0x0;
		}

		// Display hit effect for boss
		spirit[15] = spirit[15] & 0xfffeffffffffffff;
		if (cooling_c[c_hitb] != 0) {
			temp = 0x01;
			spirit[15] = (spirit[15] & 0xfffeffffffffffff) | (temp << 48);
		}

		// Write data to spirit
		write_spirit(spirit, data);
	}
}


//----------------------------------------------------------------------------------------//
//
//                                Main function
//
//----------------------------------------------------------------------------------------//
int main(void)
{
	IO_init();

	/*while(1)
	{
		IO_write(HPI_MAILBOX,COMM_EXEC_INT);
		printf("[ERROR]:routine mailbox data is %x\n",IO_read(HPI_MAILBOX));
		//UsbWrite(0xc008,0x000f);
		//UsbRead(0xc008);
		usleep(10*10000);
	}*/

	alt_u16 intStat;
	alt_u16 usb_ctl_val;
	static alt_u16 ctl_reg = 0;
	static alt_u16 no_device = 0;
	alt_u16 fs_device = 0;
	int keycode = 0;
	alt_u8 toggle = 0;
	alt_u8 data_size;
	alt_u8 hot_plug_count;
	alt_u16 code;

	// Test
	int done_flag = 0;

	printf("USB keyboard setup...\n\n");

	//----------------------------------------SIE1 initial---------------------------------------------------//
	USB_HOT_PLUG:
	UsbSoftReset();

	// STEP 1a:
	UsbWrite (HPI_SIE1_MSG_ADR, 0);
	UsbWrite (HOST1_STAT_REG, 0xFFFF);

	/* Set HUSB_pEOT time */
	UsbWrite(HUSB_pEOT, 600); // adjust the according to your USB device speed

	usb_ctl_val = SOFEOP1_TO_CPU_EN | RESUME1_TO_HPI_EN;// | SOFEOP1_TO_HPI_EN;
	UsbWrite(HPI_IRQ_ROUTING_REG, usb_ctl_val);

	intStat = A_CHG_IRQ_EN | SOF_EOP_IRQ_EN ;
	UsbWrite(HOST1_IRQ_EN_REG, intStat);
	// STEP 1a end

	// STEP 1b begin
	UsbWrite(COMM_R0,0x0000);//reset time
	UsbWrite(COMM_R1,0x0000);  //port number
	UsbWrite(COMM_R2,0x0000);  //r1
	UsbWrite(COMM_R3,0x0000);  //r1
	UsbWrite(COMM_R4,0x0000);  //r1
	UsbWrite(COMM_R5,0x0000);  //r1
	UsbWrite(COMM_R6,0x0000);  //r1
	UsbWrite(COMM_R7,0x0000);  //r1
	UsbWrite(COMM_R8,0x0000);  //r1
	UsbWrite(COMM_R9,0x0000);  //r1
	UsbWrite(COMM_R10,0x0000);  //r1
	UsbWrite(COMM_R11,0x0000);  //r1
	UsbWrite(COMM_R12,0x0000);  //r1
	UsbWrite(COMM_R13,0x0000);  //r1
	UsbWrite(COMM_INT_NUM,HUSB_SIE1_INIT_INT); //HUSB_SIE1_INIT_INT
	IO_write(HPI_MAILBOX,COMM_EXEC_INT);

	while (!(IO_read(HPI_STATUS) & 0xFFFF) )  //read sie1 msg register
	{
	}
	while (IO_read(HPI_MAILBOX) != COMM_ACK)
	{
		printf("[ERROR]:routine mailbox data is %x\n",IO_read(HPI_MAILBOX));
		goto USB_HOT_PLUG;
	}
	// STEP 1b end

	printf("STEP 1 Complete");
	// STEP 2 begin
	UsbWrite(COMM_INT_NUM,HUSB_RESET_INT); //husb reset
	UsbWrite(COMM_R0,0x003c);//reset time
	UsbWrite(COMM_R1,0x0000);  //port number
	UsbWrite(COMM_R2,0x0000);  //r1
	UsbWrite(COMM_R3,0x0000);  //r1
	UsbWrite(COMM_R4,0x0000);  //r1
	UsbWrite(COMM_R5,0x0000);  //r1
	UsbWrite(COMM_R6,0x0000);  //r1
	UsbWrite(COMM_R7,0x0000);  //r1
	UsbWrite(COMM_R8,0x0000);  //r1
	UsbWrite(COMM_R9,0x0000);  //r1
	UsbWrite(COMM_R10,0x0000);  //r1
	UsbWrite(COMM_R11,0x0000);  //r1
	UsbWrite(COMM_R12,0x0000);  //r1
	UsbWrite(COMM_R13,0x0000);  //r1

	IO_write(HPI_MAILBOX,COMM_EXEC_INT);

	while (IO_read(HPI_MAILBOX) != COMM_ACK)
	{
		printf("[ERROR]:routine mailbox data is %x\n",IO_read(HPI_MAILBOX));
		goto USB_HOT_PLUG;
	}
	// STEP 2 end

	ctl_reg = USB1_CTL_REG;
	no_device = (A_DP_STAT | A_DM_STAT);
	fs_device = A_DP_STAT;
	usb_ctl_val = UsbRead(ctl_reg);

	if (!(usb_ctl_val & no_device))
	{
		for(hot_plug_count = 0 ; hot_plug_count < 5 ; hot_plug_count++)
		{
			usleep(5*1000);
			usb_ctl_val = UsbRead(ctl_reg);
			if(usb_ctl_val & no_device) break;
		}
		if(!(usb_ctl_val & no_device))
		{
			printf("\n[INFO]: no device is present in SIE1!\n");
			printf("[INFO]: please insert a USB keyboard in SIE1!\n");
			while (!(usb_ctl_val & no_device))
			{
				usb_ctl_val = UsbRead(ctl_reg);
				if(usb_ctl_val & no_device)
					goto USB_HOT_PLUG;

				usleep(2000);
			}
		}
	}
	else
	{
		/* check for low speed or full speed by reading D+ and D- lines */
		if (usb_ctl_val & fs_device)
		{
			printf("[INFO]: full speed device\n");
		}
		else
		{
			printf("[INFO]: low speed device\n");
		}
	}



	// STEP 3 begin
	//------------------------------------------------------set address -----------------------------------------------------------------
	UsbSetAddress();

	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		UsbSetAddress();
		usleep(10*1000);
	}

	UsbWaitTDListDone();

	IO_write(HPI_ADDR,0x0506); // i
	printf("[ENUM PROCESS]:step 3 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508); // n
	usb_ctl_val = IO_read(HPI_DATA);
	printf("[ENUM PROCESS]:step 3 TD Control Byte is %x\n",usb_ctl_val);
	while (usb_ctl_val != 0x03) // retries occurred
	{
		usb_ctl_val = UsbGetRetryCnt();

		goto USB_HOT_PLUG;
	}

	printf("------------[ENUM PROCESS]:set address done!---------------\n");

	// STEP 4 begin
	//-------------------------------get device descriptor-1 -----------------------------------//
	// TASK: Call the appropriate function for this step.
	UsbGetDeviceDesc1(); 	// Get Device Descriptor -1

	//usleep(10*1000);
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbGetDeviceDesc1();
		usleep(10*1000);
	}

	UsbWaitTDListDone();

	IO_write(HPI_ADDR,0x0506);
	printf("[ENUM PROCESS]:step 4 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	printf("[ENUM PROCESS]:step 4 TD Control Byte is %x\n",usb_ctl_val);
	while (usb_ctl_val != 0x03)
	{
		usb_ctl_val = UsbGetRetryCnt();
	}

	printf("---------------[ENUM PROCESS]:get device descriptor-1 done!-----------------\n");


	//--------------------------------get device descriptor-2---------------------------------------------//
	//get device descriptor
	// TASK: Call the appropriate function for this step.
	UsbGetDeviceDesc2(); 	// Get Device Descriptor -2

	//if no message
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		//resend the get device descriptor
		//get device descriptor
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbGetDeviceDesc2();
		usleep(10*1000);
	}

	UsbWaitTDListDone();

	IO_write(HPI_ADDR,0x0506);
	printf("[ENUM PROCESS]:step 4 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	printf("[ENUM PROCESS]:step 4 TD Control Byte is %x\n",usb_ctl_val);
	while (usb_ctl_val != 0x03)
	{
		usb_ctl_val = UsbGetRetryCnt();
	}

	printf("------------[ENUM PROCESS]:get device descriptor-2 done!--------------\n");


	// STEP 5 begin
	//-----------------------------------get configuration descriptor -1 ----------------------------------//
	// TASK: Call the appropriate function for this step.
	UsbGetConfigDesc1(); 	// Get Configuration Descriptor -1

	//if no message
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		//resend the get device descriptor
		//get device descriptor

		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbGetConfigDesc1();
		usleep(10*1000);
	}

	UsbWaitTDListDone();

	IO_write(HPI_ADDR,0x0506);
	printf("[ENUM PROCESS]:step 5 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	printf("[ENUM PROCESS]:step 5 TD Control Byte is %x\n",usb_ctl_val);
	while (usb_ctl_val != 0x03)
	{
		usb_ctl_val = UsbGetRetryCnt();
	}
	printf("------------[ENUM PROCESS]:get configuration descriptor-1 pass------------\n");

	// STEP 6 begin
	//-----------------------------------get configuration descriptor-2------------------------------------//
	//get device descriptor
	// TASK: Call the appropriate function for this step.
	UsbGetConfigDesc2(); 	// Get Configuration Descriptor -2

	usleep(100*1000);
	//if no message
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbGetConfigDesc2();
		usleep(10*1000);
	}

	UsbWaitTDListDone();

	IO_write(HPI_ADDR,0x0506);
	printf("[ENUM PROCESS]:step 6 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	printf("[ENUM PROCESS]:step 6 TD Control Byte is %x\n",usb_ctl_val);
	while (usb_ctl_val != 0x03)
	{
		usb_ctl_val = UsbGetRetryCnt();
	}


	printf("-----------[ENUM PROCESS]:get configuration descriptor-2 done!------------\n");


	// ---------------------------------get device info---------------------------------------------//

	// TASK: Write the address to read from the memory for byte 7 of the interface descriptor to HPI_ADDR.
	IO_write(HPI_ADDR,0x056c);
	code = IO_read(HPI_DATA);
	code = code & 0x003;
	printf("\ncode = %x\n", code);

	if (code == 0x01)
	{
		printf("\n[INFO]:check TD rec data7 \n[INFO]:Keyboard Detected!!!\n\n");
	}
	else
	{
		printf("\n[INFO]:Keyboard Not Detected!!! \n\n");
	}

	// TASK: Write the address to read from the memory for the endpoint descriptor to HPI_ADDR.

	IO_write(HPI_ADDR,0x0576);
	IO_write(HPI_DATA,0x073F);
	IO_write(HPI_DATA,0x8105);
	IO_write(HPI_DATA,0x0003);
	IO_write(HPI_DATA,0x0008);
	IO_write(HPI_DATA,0xAC0A);
	UsbWrite(HUSB_SIE1_pCurrentTDPtr,0x0576); //HUSB_SIE1_pCurrentTDPtr

	//data_size = (IO_read(HPI_DATA)>>8)&0x0ff;
	//data_size = 0x08;//(IO_read(HPI_DATA))&0x0ff;
	//UsbPrintMem();
	IO_write(HPI_ADDR,0x057c);
	data_size = (IO_read(HPI_DATA))&0x0ff;
	printf("[ENUM PROCESS]:data packet size is %d\n",data_size);
	// STEP 7 begin
	//------------------------------------set configuration -----------------------------------------//
	// TASK: Call the appropriate function for this step.
	UsbSetConfig();		// Set Configuration

	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbSetConfig();		// Set Configuration
		usleep(10*1000);
	}

	UsbWaitTDListDone();

	IO_write(HPI_ADDR,0x0506);
	printf("[ENUM PROCESS]:step 7 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	printf("[ENUM PROCESS]:step 7 TD Control Byte is %x\n",usb_ctl_val);
	while (usb_ctl_val != 0x03)
	{
		usb_ctl_val = UsbGetRetryCnt();
	}

	printf("------------[ENUM PROCESS]:set configuration done!-------------------\n");

	//----------------------------------------------class request out ------------------------------------------//
	// TASK: Call the appropriate function for this step.
	UsbClassRequest();

	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbClassRequest();
		usleep(10*1000);
	}

	UsbWaitTDListDone();

	IO_write(HPI_ADDR,0x0506);
	printf("[ENUM PROCESS]:step 8 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	printf("[ENUM PROCESS]:step 8 TD Control Byte is %x\n",usb_ctl_val);
	while (usb_ctl_val != 0x03)
	{
		usb_ctl_val = UsbGetRetryCnt();
	}


	printf("------------[ENUM PROCESS]:class request out done!-------------------\n");

	// STEP 8 begin
	//----------------------------------get descriptor(class 0x21 = HID) request out --------------------------------//
	// TASK: Call the appropriate function for this step.
	UsbGetHidDesc();

	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbGetHidDesc();
		usleep(10*1000);
	}

	UsbWaitTDListDone();

	IO_write(HPI_ADDR,0x0506);
	printf("[ENUM PROCESS]:step 8 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	printf("[ENUM PROCESS]:step 8 TD Control Byte is %x\n",usb_ctl_val);
	while (usb_ctl_val != 0x03)
	{
		usb_ctl_val = UsbGetRetryCnt();
	}

	printf("------------[ENUM PROCESS]:get descriptor (class 0x21) done!-------------------\n");

	// STEP 9 begin
	//-------------------------------get descriptor (class 0x22 = report)-------------------------------------------//
	// TASK: Call the appropriate function for this step.
	UsbGetReportDesc();
	//if no message
	while (!(IO_read(HPI_STATUS) & HPI_STATUS_SIE1msg_FLAG) )  //read sie1 msg register
	{
		// TASK: Call the appropriate function again if it wasn't processed successfully.
		UsbGetReportDesc();
		usleep(10*1000);
	}

	UsbWaitTDListDone();

	IO_write(HPI_ADDR,0x0506);
	printf("[ENUM PROCESS]: step 9 TD Status Byte is %x\n",IO_read(HPI_DATA));

	IO_write(HPI_ADDR,0x0508);
	usb_ctl_val = IO_read(HPI_DATA);
	printf("[ENUM PROCESS]: step 9 TD Control Byte is %x\n",usb_ctl_val);
	while (usb_ctl_val != 0x03)
	{
		usb_ctl_val = UsbGetRetryCnt();
	}

	printf("---------------[ENUM PROCESS]:get descriptor (class 0x22) done!----------------\n");


	usleep(10000);
	

	// Start main game loop
	game_main(data_size, usb_ctl_val);

	return 0;
}

