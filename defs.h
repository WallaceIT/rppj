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

#define RPPJ_VERSION       "0.2alpha"

/* GPIO registers address */
#define BCM2708_PERI_BASE  0x20000000
#define GPIO_BASE          (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
#define BLOCK_SIZE         (256)

/* GPIO setup macros. Always use GPIO_IN(x) before using GPIO_OUT(x) */
#define GPIO_IN(g)    *(gpio+((g)/10))   &= ~(7<<(((g)%10)*3))
#define GPIO_OUT(g)   *(gpio+((g)/10))   |=  (1<<(((g)%10)*3))

#define GPIO_SET(g)   *(gpio+7)  = 1<<(g)  /* sets   bit which are 1, ignores bit which are 0 */
#define GPIO_CLR(g)   *(gpio+10) = 1<<(g)  /* clears bit which are 1, ignores bit which are 0 */
#define GPIO_LEV(g)   (*(gpio+13) >> (g)) & 0x1	/* reads pin level */

/* default GPIO <-> PIC connections */
#define DEFAULT_PIC_CLK    11	/* PGC - Output */
#define DEFAULT_PIC_DATA   9	/* PGD - I/O */
#define DEFAULT_PIC_MCLR   22	/* MCLR - Output */

/* delays (in microseconds) */
#define DELAY_P1   	1
#define DELAY_P2   	1
#define DELAY_P2A  	1
#define DELAY_P2B  	1
#define DELAY_P3   	1
#define DELAY_P4   	1
#define DELAY_P5   	1
#define DELAY_P5A  	1
#define DELAY_P6   	1
#define DELAY_P9  	3400
#define DELAY_P10  	54000
#define DELAY_P11  	524000
#define DELAY_P12  	400
#define DELAY_P13  	1
#define DELAY_P14  	1
#define DELAY_P16  	1
#define DELAY_P17  	3
#define DELAY_P19	4000
#define DELAY_P20	1

/* commands for programming */
#define COMM_CORE_INSTRUCTION 			0x00
#define COMM_SHIFT_OUT_TABLAT 			0x02
#define COMM_TABLE_READ	     			0x08	
#define COMM_TABLE_READ_POST_INC 		0x09
#define COMM_TABLE_READ_POST_DEC		0x0A
#define COMM_TABLE_READ_PRE_INC 		0x0B
#define COMM_TABLE_WRITE			0x0C
#define COMM_TABLE_WRITE_POST_INC_2		0x0D
#define COMM_TABLE_WRITE_STARTP_POST_INC_2	0x0E
#define COMM_TABLE_WRITE_STARTP			0x0F

#define ENTER_PROGRAM_KEY			0x4D434850

struct picmemory {
	uint16_t *data;		/* 16-bit data */
	uint8_t  *filled;	/* 1 if this cell is used */
};

struct picmicro {
	uint16_t        device_id;
	char            name[16];
	size_t          program_memory_size;   /* size in WORDS (2 Bytes) */
	uint16_t        checksum_mask[8];       /* checksum masks */
};

/* rppj.c functions */
void delayMicrosecondsHard (unsigned int howLong);
void free_picmemory(struct picmemory **ppm);
void enter_program_mode(void);
void exit_program_mode(void);
void pic_send_cmd(uint8_t cmd);
uint16_t pic_read_data(void);
uint16_t pic_read_device_id(void);
void pic_write_data(uint16_t data);
void pic_goto_mem_location(uint32_t data);
void pic_bulk_erase(void);
void pic_read(const struct picmicro *pic, char *outfile);
void pic_write(const struct picmicro *pic, char *infile);
void pic_blank_check(const struct picmicro *pic);
void usage(void);
void setup_io(void);
void close_io(void);

/* inhx.c functions */
struct picmemory *read_inhx(char *infile, const struct picmicro *pic, uint8_t debug);
void write_inhx(struct picmemory *pm, char *outfile, const struct picmicro *pic);

