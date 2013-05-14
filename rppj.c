/*
 * Raspberry Pi PIC18 J-Series Programmer using GPIO connector
 * GITHUB URL
 * Copyright 2013 Francesco Valla
 *
 * Based on rpp - Copyright Giorgio Vazzana - http://holdenc.altervista.org/rpp/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include "defs.h"

const struct picmicro pic18f44j10  = {0x1D20,  "PIC18F44J10", 0x2000, {0xE1,0x04,0xC7,0x0F,0x00,0x01,0x00,0x00} };
const struct picmicro pic18f45j10  = {0x1C20,  "PIC18F45J10", 0x4000, {0xE1,0x04,0xC7,0x0F,0x00,0x01,0x00,0x00} };

const struct picmicro pic18f24j11  = {0x4D80,  "PIC18F24J11", 0x2000, {0xE1,0xFC,0xDF,0xFF,0xFF,0xF9,0xFF,0xF1} };
const struct picmicro pic18f25j11  = {0x4DA0,  "PIC18F25J11", 0x4000, {0xE1,0xFC,0xDF,0xFF,0xFF,0xF9,0xFF,0xF1} };
const struct picmicro pic18f26j11  = {0x4DC0,  "PIC18F26J11", 0x8000, {0xE1,0xFC,0xDF,0xFF,0xFF,0xF9,0xFF,0xF1} };

const struct picmicro pic18f44j11  = {0x4DE0,  "PIC18F44J11", 0x2000, {0xE1,0xFC,0xDF,0xFF,0xFF,0xF9,0xFF,0xF1} };
const struct picmicro pic18f45j11  = {0x4E00,  "PIC18F45J11", 0x4000, {0xE1,0xFC,0xDF,0xFF,0xFF,0xF9,0xFF,0xF1} };
const struct picmicro pic18f46j11  = {0x4E20,  "PIC18F46J11", 0x8000, {0xE1,0xFC,0xDF,0xFF,0xFF,0xF9,0xFF,0xF1} };

const struct picmicro pic18f24j50  = {0x4C00,  "PIC18F24J50", 0x2000, {0xEF,0xFF,0xDF,0xFF,0xFF,0xF9,0xFF,0xF1} };
const struct picmicro pic18f25j50  = {0x4C20,  "PIC18F25J50", 0x4000, {0xEF,0xFF,0xDF,0xFF,0xFF,0xF9,0xFF,0xF1} };
const struct picmicro pic18f26j50  = {0x4C40,  "PIC18F26J50", 0x8000, {0xEF,0xFF,0xDF,0xFF,0xFF,0xF9,0xFF,0xF1} };

const struct picmicro pic18f44j50  = {0x4C60,  "PIC18F44J50", 0x2000, {0xEF,0xFF,0xDF,0xFF,0xFF,0xF9,0xFF,0xF1} };
const struct picmicro pic18f45j50  = {0x4C80,  "PIC18F45J50", 0x4000, {0xEF,0xFF,0xDF,0xFF,0xFF,0xF9,0xFF,0xF1} };
const struct picmicro pic18f46j50  = {0x4CA0,  "PIC18F46J50", 0x8000, {0xEF,0xFF,0xDF,0xFF,0xFF,0xF9,0xFF,0xF1} };

const struct picmicro pic18f26j13  = {0x5920,  "PIC18F26J13", 0x8000, {0xFF,0xFC,0xFF,0xFF,0xFF,0xFF,0xBF,0xF3} };
const struct picmicro pic18f46j13  = {0x59A0,  "PIC18F46J13", 0x8000, {0xFF,0xFC,0xFF,0xFF,0xFF,0xFF,0xBF,0xF3} };

const struct picmicro pic18f26j53  = {0x5820,  "PIC18F26J53", 0x8000, {0xFF,0xFF,0xFF,0xFF,0xFF,0xFB,0xBF,0xFB} };
const struct picmicro pic18f46j53  = {0x58A0,  "PIC18F46J53", 0x8000, {0xFF,0xFF,0xFF,0xFF,0xFF,0xFB,0xBF,0xFB} };

const struct picmicro *piclist[] = {&pic18f44j10, &pic18f45j10, &pic18f24j11, &pic18f25j11, &pic18f26j11, &pic18f44j11, &pic18f45j11, &pic18f46j11, &pic18f24j50, &pic18f25j50, &pic18f26j50, &pic18f44j50, &pic18f45j50, &pic18f46j50, &pic18f26j13, &pic18f46j13, &pic18f26j53, &pic18f46j53, NULL};

int                mem_fd;
void              *gpio_map;
volatile uint32_t *gpio;

uint8_t debug;

int pic_clk = DEFAULT_PIC_CLK, pic_data = DEFAULT_PIC_DATA, pic_mclr = DEFAULT_PIC_MCLR;

/* Hardware delay function by Gordons Projects - WiringPi*/
void delayMicrosecondsHard (unsigned int howLong)
{
   struct timeval tNow, tLong, tEnd ;

   gettimeofday (&tNow, NULL) ;
   tLong.tv_sec  = howLong / 1000000 ;
   tLong.tv_usec = howLong % 1000000 ;
   timeradd (&tNow, &tLong, &tEnd) ;

   while (timercmp (&tNow, &tEnd, <))
     gettimeofday (&tNow, NULL) ;
}


void free_picmemory(struct picmemory **ppm)
{
	free((*ppm) -> data);
	free((*ppm) -> filled);
	free(*ppm);
}

void enter_program_mode(void)
{
	int i;

	GPIO_IN(pic_mclr);
	GPIO_OUT(pic_mclr);

	GPIO_CLR(pic_mclr);			/* remove VDD from MCLR pin */
	delayMicrosecondsHard(DELAY_P13);	/* wait P13 */
	GPIO_SET(pic_mclr);			/* apply VDD to MCLR pin */
	delayMicrosecondsHard(10);		/* wait (no minimum time requirement) */
	GPIO_CLR(pic_mclr);			/* remove VDD from MCLR pin */
	delayMicrosecondsHard(DELAY_P19);	/* wait P19 */

	GPIO_CLR(pic_clk);
	/* Shift in the "enter program mode" key sequence (MSB first) */
	for (i = 31; i > -1; i--) {
		if ( (ENTER_PROGRAM_KEY >> i) & 0x01 )
			GPIO_SET(pic_data);
		else
			GPIO_CLR(pic_data);
		delayMicrosecondsHard(DELAY_P2B);	/* Setup time */
		GPIO_SET(pic_clk);
		delayMicrosecondsHard(DELAY_P2A);	/* Hold time */
		GPIO_CLR(pic_clk);
		
	}
	GPIO_CLR(pic_data);
	delayMicrosecondsHard(DELAY_P20);	/* Wait P20 */
	GPIO_SET(pic_mclr);			/* apply VDD to MCLR pin */
	delayMicrosecondsHard(DELAY_P12);	/* Wait (at least) P12 */
}

void exit_program_mode(void)
{

	GPIO_CLR(pic_clk);			/* stop clock on PGC */
	GPIO_CLR(pic_data);			/* clear data pin PGD */
	delayMicrosecondsHard(DELAY_P16);	/* wait P16 */
	GPIO_CLR(pic_mclr);			/* remove VDD from MCLR pin */
	delayMicrosecondsHard(DELAY_P17);	/* wait (at least) P17 */
	GPIO_IN(pic_mclr);                      /* MCLR as input, puts the output driver in Hi-Z */
}


/* Send a 4-bit command to the PIC (LSB first) */
void pic_send_cmd(uint8_t cmd)
{
	int i;

	for (i = 0; i < 4; i++) {
		GPIO_SET(pic_clk);
		if ( (cmd >> i) & 0x01 )
			GPIO_SET(pic_data);
		else
			GPIO_CLR(pic_data);
		delayMicrosecondsHard(DELAY_P2B);	/* Setup time */
		GPIO_CLR(pic_clk);
		delayMicrosecondsHard(DELAY_P2A);	/* Hold time */
	}
	GPIO_CLR(pic_data);
	delayMicrosecondsHard(DELAY_P5);
}

/* Read 8-bit data from the PIC (LSB first) */
uint16_t pic_read_data(void)
{
	uint8_t i;
	uint16_t data = 0x0000;

	for (i = 0; i < 8; i++) {
		GPIO_SET(pic_clk);
		delayMicrosecondsHard(DELAY_P2B);
		GPIO_CLR(pic_clk);
		delayMicrosecondsHard(DELAY_P2A);
	}
	
	delayMicrosecondsHard(DELAY_P6);	/* wait for the data... */
	
	GPIO_IN(pic_data);

	for (i = 0; i < 8; i++) {
		GPIO_SET(pic_clk);
		delayMicrosecondsHard(DELAY_P14);	/* Wait for data to be valid */
		data |= ( GPIO_LEV(pic_data) & 0x00000001 ) << i;
		delayMicrosecondsHard(DELAY_P2B);
		GPIO_CLR(pic_clk);
		delayMicrosecondsHard(DELAY_P2A);
	}
	
	delayMicrosecondsHard(DELAY_P5A);
	GPIO_IN(pic_data);
	GPIO_OUT(pic_data);
	return data;
}

/* Load 16-bit data to the PIC (LSB first) */
void pic_write_data(uint16_t data)
{
	int i;

	for (i = 0; i < 16; i++) {
		GPIO_SET(pic_clk);
		if ( (data >> i) & 0x0001 )
			GPIO_SET(pic_data);
		else
			GPIO_CLR(pic_data);
		delayMicrosecondsHard(DELAY_P2B);	/* Setup time */
		GPIO_CLR(pic_clk);
		delayMicrosecondsHard(DELAY_P2A);	/* Hold time */
	}
	GPIO_CLR(pic_data);
	delayMicrosecondsHard(DELAY_P5A);
}

/* set Table Pointer */
void pic_goto_mem_location(uint32_t data)
{

	data = data & 0x00FFFFFF;	/* set the MSB byte to zero (it should already be zero)	*/

	pic_send_cmd(COMM_CORE_INSTRUCTION);
	pic_write_data( 0x0E00 | ( (data >> 16) & 0x000000FF) );/* MOVLW Addr[21:16] */
	pic_send_cmd(COMM_CORE_INSTRUCTION);
	pic_write_data(0x6EF8);					/* MOVWF TBLPTRU */
	pic_send_cmd(COMM_CORE_INSTRUCTION);
	pic_write_data( 0x0E00 | ( (data >> 8) & 0x000000FF) );	/* MOVLW Addr[15:8] */
	pic_send_cmd(COMM_CORE_INSTRUCTION);
	pic_write_data(0x6EF7);					/* MOVWF TBLPTRH */
	pic_send_cmd(COMM_CORE_INSTRUCTION);
	pic_write_data( 0x0E00 | (data & 0x000000FF) );		/* MOVLW Addr[7:0] */
	pic_send_cmd(COMM_CORE_INSTRUCTION);
	pic_write_data(0x6EF6);					/* MOVWF TBLPTRL */
}

/* Read PIC device id word, located at 0x3FFFFE:0x3FFFFF */
uint16_t pic_read_device_id(void)
{
	uint16_t id;

	enter_program_mode();	/* Enter Program/Verify Mode */
	
	pic_goto_mem_location(0x3FFFFE);
	
	pic_send_cmd(COMM_TABLE_READ_POST_INC);
	id = pic_read_data();
	pic_send_cmd(COMM_TABLE_READ_POST_INC);
	id = ( pic_read_data() << 8) | (id & 0xFF) ;

	exit_program_mode();	/* Exit Program/Verify Mode */

	return id;
}

/* Bulk erase the chip */
void pic_bulk_erase(void)
{
        enter_program_mode();   /* Enter Program/Verify Mode */

	fprintf(stderr, "Performing a Bulk Erase...");
	pic_goto_mem_location(0x3C0004);
	pic_send_cmd(COMM_TABLE_WRITE);
	pic_write_data(0x0180);
	pic_send_cmd(COMM_CORE_INSTRUCTION);
	pic_write_data(0x0000);                 /* NOP */
	pic_send_cmd(COMM_CORE_INSTRUCTION);
	GPIO_CLR(pic_data);	                /* Hold PGD low until erase completes. */
	delayMicrosecondsHard(DELAY_P11);
	delayMicrosecondsHard(DELAY_P10);
	fprintf(stderr, "DONE!\n");

	exit_program_mode();
}

/* Read PIC memory and write the contents to a .hex file */
void pic_read(const struct picmicro *pic, char *outfile)
{
	struct picmemory *pm;
	uint16_t addr, data = 0x0000;
	uint16_t global_checksum = 0;

	pm = calloc(1, sizeof(*pm));
	if (pm) {
		pm -> data   = calloc(pic -> program_memory_size, sizeof(*pm -> data));
		pm -> filled = calloc(pic -> program_memory_size, sizeof(*pm -> filled));
	}
	if (!pm || !pm -> data || !pm -> filled) {
		fprintf(stderr, "Error: calloc() failed.\n");
		return;
	}

	fprintf(stderr, "Reading chip...\n");

	enter_program_mode();	/* Enter Program/Verify Mode */

	/* Read Memory */

	pic_goto_mem_location(0x000000);

	for (addr = 0; addr < pic -> program_memory_size; addr++) {
		
		pic_send_cmd(COMM_TABLE_READ_POST_INC);
		data = pic_read_data();
		pic_send_cmd(COMM_TABLE_READ_POST_INC);
		data = ( pic_read_data() << 8 ) | (data & 0x00FF);

		if (debug) fprintf(stderr, "  addr = 0x%04X  data = 0x%04X\n", addr*2, data);

		if (data != 0xFFFF) {
			pm -> data[addr]        = data;
			pm -> filled[addr]      = 1;
		}

		if(addr < (pic -> program_memory_size - 4)){
		    global_checksum += (data & 0x00FF);         /* sum low byte */
		    global_checksum += (data >> 8) & 0x00FF;    /* sum high byte */
		}
		else{
		    global_checksum += ((data & 0x00FF) & (pic -> checksum_mask)[2*(pic -> program_memory_size - addr +4)] );
		    global_checksum += ((data >> 8) & (pic -> checksum_mask)[2*(pic -> program_memory_size - addr +4) +1] );
		}


	}

	/* FIXME: fprintf(stderr, "Checksum: 0x%04X.\n", global_checksum); */

	exit_program_mode();	/* Exit Program/Verify Mode */

	write_inhx8m(pm, outfile, pic);
	free_picmemory(&pm);
}

/* Bulk erase the chip, and then write contents of the .hex file to the PIC */
void pic_write(const struct picmicro *pic, char *infile)
{
	int i;
	uint16_t data;
	uint32_t addr = 0x00000000;
	struct picmemory *pm;

	pm = read_inhx8m(infile, pic, debug);
	if (!pm) return;

	pic_bulk_erase();	/* Bulk erase the chip first */

	enter_program_mode();   /* Enter Program/Verify Mode */

	fprintf(stderr, "Writing chip...");

	pic_send_cmd(COMM_CORE_INSTRUCTION);
	pic_write_data(0x84A6);			/* enable writes */

	for (addr = 0; addr < (pic -> program_memory_size); addr += 32){        /* address in WORDS (2 Bytes) */
		
		pic_goto_mem_location(2*addr);
		if (debug) fprintf(stderr, "Go to address 0x%08X \n", addr);

		for(i=0; i<31; i++){		                        /* write the first 62 bytes */
			if (pm -> filled[addr+i]) {
				if (debug) fprintf(stderr, "  Writing 0x%04X to address 0x%06X \n", pm -> data[addr + i], (addr+i)*2 );
				pic_send_cmd(COMM_TABLE_WRITE_POST_INC_2);
				pic_write_data(pm -> data[addr+i]);
			}
			else {
				if (debug) fprintf(stderr, "  Writing 0xFFFF to address 0x%06X \n", (addr+i)*2 );
				pic_send_cmd(COMM_TABLE_WRITE_POST_INC_2);
				pic_write_data(0xFFFF);			/* write 0xFFFF in empty locations */
			};
		}
		
		/* write the last 2 bytes and start programming */
		if (pm -> filled[addr+31]) {
			if (debug) fprintf(stderr, "  Writing 0x%04X to address 0x%06X and then start programming...\n", pm -> data[addr+31], (addr+31)*2);
			pic_send_cmd(COMM_TABLE_WRITE_STARTP);
			pic_write_data(pm -> data[addr+31]);

		}
		else {
			if (debug) fprintf(stderr, "  Writing 0xFFFF to address 0x%06X and then start programming...\n", (addr+31)*2);
			pic_send_cmd(COMM_TABLE_WRITE_STARTP);
			pic_write_data(0xFFFF);			         /* write 0xFFFF in empty locations */
		};

		/* Programming Sequence */
		GPIO_CLR(pic_data);
		for (i = 0; i < 3; i++) {

		                GPIO_SET(pic_clk);
		                delayMicrosecondsHard(DELAY_P2B);       /* Setup time */
		                GPIO_CLR(pic_clk);
		                delayMicrosecondsHard(DELAY_P2A);       /* Hold time */
		        }
		        GPIO_SET(pic_clk);
		        delayMicrosecondsHard(DELAY_P9);        /* Programming time */
		        GPIO_CLR(pic_clk);
		        delayMicrosecondsHard(DELAY_P5);
		        pic_write_data(0x0000);
		/* end of Programming Sequence */
	};

	fprintf(stderr, "DONE!\n");

	/* Verify Code Memory and Configuration Word */
	fprintf(stderr, "Verifying written data...\n");

	pic_goto_mem_location(0x000000);

	for (addr = 0; addr < pic -> program_memory_size; addr++) {
		
		pic_send_cmd(COMM_TABLE_READ_POST_INC);
		data = pic_read_data();
		pic_send_cmd(COMM_TABLE_READ_POST_INC);
		data = ( pic_read_data() << 8 ) | ( data & 0xFF );

		if (debug) fprintf(stderr, "addr = 0x%06X:  pic = 0x%04X, file = 0x%04X\n", addr*2, data, (pm -> filled[addr]) ? (pm -> data[addr]) : 0xFFFF);

		if ( (data != pm -> data[addr]) & ( pm -> filled[addr]) ) {
			fprintf(stderr, "Error at addr = 0x%06X:  pic = 0x%04X, file = 0x%04X.\nExiting...", addr*2, data, pm -> data[addr]);
			break;
		}
	}

	fprintf(stderr, "DONE!\n");
	
	exit_program_mode();	/* Exit Program/Verify Mode */
}

/* Blank Check */
void pic_blank_check(const struct picmicro *pic)
{
	uint16_t addr, data;
	uint8_t blank = 1;

	enter_program_mode();	/* Enter Program/Verify Mode */

	fprintf(stderr, "Performing Blank Check...\n");

	pic_goto_mem_location(0x000000);

	for (addr = 0; addr < ((pic -> program_memory_size) - 4); addr++) {	/* FIXME: Ignore Flash Configuration Words ?? */
		
		pic_send_cmd(COMM_TABLE_READ_POST_INC);
		data = pic_read_data();
		pic_send_cmd(COMM_TABLE_READ_POST_INC);
		data = (pic_read_data() << 8) | (data & 0xFF) ;

		if (data != 0xFFFF) {
			fprintf(stderr, "Chip not Blank! Address: 0x%d, Read: 0x%x.\n",  addr*2, data);
			blank = 0;			
			break;
		}
	}

	if (blank) fprintf(stderr, "Blank Chip!\n");
	fprintf(stderr, "DONE!\n");
	
	exit_program_mode();	/* Exit Program/Verify Mode */
}

/* print the help */
void usage(void)
{
	const struct picmicro **ppic;

	fprintf(stderr,
"Usage: rppj [options]\n"
"       -h                print help\n"
"       -D                turn debug on\n"
"       -g PGC,PGD,MCLR   GPIO selection, default if not present\n"
"       -i file           input file\n"
"       -o file           output file (ofile.hex)\n"
"       -p part           part (optional - If present, all uppercase - )\n"
"       -r                read chip\n"
"       -w                bulk erase and write chip\n"
"       -e                bulk erase chip\n"
"       -b                blank check of the chip\n"
"\n"
"Supported PICs:\n"
	);
	for (ppic = piclist; *ppic; ppic++)
		fprintf(stderr, "%s, ", (*ppic) -> name);
	fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
	int opt, function = 0;
	char *infile  = NULL;
	char *outfile = "ofile.hex";
	char *part = "";
	char *pins = "";
	uint16_t device_id;
	const struct picmicro *pic, **ppic;

	fprintf(stderr, "Raspberry Pi PIC18F J-series Programmer ver. %s \n", RPPJ_VERSION);
	fprintf(stderr, "%s", pins);

	while ((opt = getopt(argc, argv, "hDi:o:p:rwebg:")) != -1) {
		switch (opt) {
		case 'h':
			usage();
			exit(0);
			break;
		case 'D':
			debug = 1;
			break;
		case 'i':
			infile = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'p':
		        part = optarg;
		        break;
		case 'g':
		        pins = optarg;
		        break;
		case 'r':
			function |= 0x01;
			break;
		case 'w':
			function |= 0x02;
			break;
		case 'e':
			function |= 0x04;
			break;
		case 'b':
			function |= 0x08;
			break;
		default:
			fprintf(stderr, "\n");
			usage();
			exit(1);
		}
	}

	if (function == 0x02 && !infile) {
		fprintf(stderr, "Please specify an input file with -i option.\n");
		exit(1);
	}

	/* Setup gpio pointer for direct register access */
	if(debug) fprintf(stderr, "Setting up i/o...\n");
	setup_io();

	/* Configure GPIOs */
	if(strcmp(pins, ""))    /* if GPIO connections are specified in the options... */
	   sscanf(&pins[0], "%d,%d,%d", &pic_clk, &pic_data, &pic_mclr);
	if(debug){
	   fprintf(stderr, "PGC connected to pin %d.\n", pic_clk);
	   fprintf(stderr, "PGD connected to pin %d.\n", pic_data);
	   fprintf(stderr, "MCLR connected to pin %d.\n", pic_mclr);
	}
	GPIO_IN(pic_clk); /* NOTE: MUST use GPIO_IN before we can use GPIO_OUT */
	GPIO_OUT(pic_clk);

	GPIO_IN(pic_data);
        GPIO_OUT(pic_data);

	GPIO_IN(pic_mclr);
	GPIO_OUT(pic_mclr);

	GPIO_CLR(pic_clk);
	GPIO_CLR(pic_data);
	GPIO_SET(pic_mclr);
	delayMicrosecondsHard(1);      /* sleep for 1us after GPIO configuration */

	/* Read PIC device id word */
	pic = &pic18f24j50;	        /* default PIC */
	if(debug) fprintf(stderr, "Reading device id...\n");
	device_id = pic_read_device_id();
	if(debug) fprintf(stderr, "device_id = 0x%04x\n", (device_id & 0xFFE0) );
	
	for (ppic = piclist; *ppic; ppic++) {
		if ((*ppic) -> device_id == (device_id & 0xFFE0)) {
			pic = *ppic;
			break;
		}
	}

	if (*ppic == NULL) {
		fprintf(stderr, "Error: unknown/unsupported device or programmer not connected.\n");
		exit(1);
	}
	else if( strcmp(part, "") & strcmp(pic -> name, part) ){
	        fprintf(stderr, "Error: wrong device! Expecting %s, detected %s.\n", part, pic -> name);
	        exit(1);
	}
	else fprintf(stderr, "%s detected, revision 0x%02x\n", pic -> name, device_id & 0x001F);

	switch (function) {
	case 0x00:	/* no function selected, exit */
		break;
	case 0x01:
		pic_read(pic, outfile);
		break;
	case 0x02:
		pic_write(pic, infile);
		break;
	case 0x04:
		pic_bulk_erase();
		break;
	case 0x08:
		pic_blank_check(pic);
		break;
	default:
		fprintf(stderr, "\n\n * Please select only one option between -r, -w, -e. * \n\n");
		break;
	};

	close_io();

	return 0;
}

/* Set up a memory regions to access GPIO */
void setup_io(void)
{
        /* open /dev/mem */
        mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
        if (mem_fd == -1) {
                perror("Cannot open /dev/mem");
                exit(1);
        }

        /* mmap GPIO */
        gpio_map = mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE);
        if (gpio_map == MAP_FAILED) {
                perror("mmap() failed");
                exit(1);
        }

        /* Always use volatile pointer! */
        gpio = (volatile uint32_t *) gpio_map;

}

/* Release GPIO memory region */
void close_io(void)
{
        int ret;

        /* munmap GPIO */
        ret = munmap(gpio_map, BLOCK_SIZE);
        if (ret == -1) {
                perror("munmap() failed");
                exit(1);
        }

        /* close /dev/mem */
        ret = close(mem_fd);
        if (ret == -1) {
                perror("Cannot close /dev/mem");
                exit(1);
        }
}
