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
#include <string.h>
#include "defs.h"

/* Read a file in Intel HEX8M or HEX32 format and return a pointer to the picmemory
   struct on success, or NULL on error */
struct picmemory *read_inhx(char *infile, const struct picmicro *pic, uint8_t debug)
{
        FILE *fp;
        int linenum;
        char line[256], *ptr;
        size_t linelen;
        int nread;

        uint16_t i=0;
        uint8_t  byte_count;
        uint16_t base_address = 0x0000;
        uint16_t address;
        uint32_t extended_address;
        uint8_t  record_type;
        uint16_t data, tmp;
        uint8_t  checksum_calculated;
        uint8_t  checksum_read;

        struct picmemory *pm;

        fp = fopen(infile, "r");
        if (fp == NULL) {
                fprintf(stderr, "Error: cannot open source file %s.\n", infile);
                return NULL;
        }

        pm = calloc(1, sizeof(*pm));
        if (pm) {
                pm -> data   = calloc(pic -> program_memory_size, sizeof(*pm -> data));
                pm -> filled = calloc(pic -> program_memory_size, sizeof(*pm -> filled));
        }
        if (!pm || !pm -> data || !pm -> filled) {
                fprintf(stderr, "Error: calloc() failed.\n");
                return NULL;
        }

        fprintf(stderr, "Reading hex file...");

        linenum = 0;
        while (1) {
                ptr = fgets(line, 256, fp);

                if (ptr != NULL) {
                        linenum++;
                        linelen = strlen(line);
                        if (debug) {
                                fprintf(stderr, "  line %d (%zd bytes): '", linenum, linelen);
                                for (i = 0; i < linelen; i++) {
                                        if (line[i] == '\n')
                                                fprintf(stderr, "\\n");
                                        else if (line[i] == '\r')
                                                fprintf(stderr, "\\r");
                                        else
                                                fprintf(stderr, "%c", line[i]);
                                }
                        fprintf(stderr, "'\n");
                        }

                        if (line[0] != ':') {
                                fprintf(stderr, "Error: invalid start code.\n");
                                free_picmemory(&pm);
                                return NULL;
                        }

                        nread = sscanf(&line[1], "%2hhx", &byte_count);
                        if (nread != 1) {
                                fprintf(stderr, "Error: cannot read byte count.\n");
                                free_picmemory(&pm);
                                return NULL;
                        }
                        if (debug) fprintf(stderr, "  byte_count  = 0x%02X\n", byte_count);


                        nread = sscanf(&line[3], "%4hx", &address);
                        if (nread != 1) {
                                fprintf(stderr, "Error: cannot read address.\n");
                                free_picmemory(&pm);
                                return NULL;
                        }
                        

                        nread = sscanf(&line[7], "%2hhx", &record_type);
                        if (nread != 1) {
                                fprintf(stderr, "Error: cannot read record type.\n");
                                free_picmemory(&pm);
                                return NULL;
                        }

                        if (debug && record_type != 0x04) fprintf(stderr, "  address     = 0x%04X\n", address);

                        if (debug)
                            fprintf(stderr, "  record_type = 0x%02X (%s)\n", record_type, record_type == 0 ? "data" : (record_type == 1 ? "EOF" : (record_type == 0x04 ? "Extended Linear Address" : "Unknown")));

                        if (record_type != 0 && record_type != 1 && record_type != 0x04) {
                                fprintf(stderr, "Error: unknown record type.\n");
                                free_picmemory(&pm);
                                return NULL;
                        }

                        checksum_calculated  = byte_count;
                        checksum_calculated += (address >> 8) & 0xFF;
                        checksum_calculated += address & 0xFF;
                        checksum_calculated += record_type;

                        //if we have an extended linear address line
                        if(record_type == 0x04){
                            nread = sscanf(&line[9], "%4hx", &base_address);
                            if (debug) fprintf(stderr, "  NEW BASE ADDRESS     = 0x%04X\n", base_address);
                            checksum_calculated += (data >> 8) & 0xFF;
                            checksum_calculated += data & 0xFF;                            
                        }

                        else
                            for (i = 0; i < byte_count/2; i++) {
                                    nread = sscanf(&line[9+4*i], "%4hx", &data);
                                    if (nread != 1) {
                                            fprintf(stderr, "Error: cannot read data.\n");
                                            free_picmemory(&pm);
                                            return NULL;
                                    }
                                    tmp = data;
                                    data = (data >> 8) | (tmp << 8);
                                    if (debug) fprintf(stderr, "  data        = 0x%04X\n", data);
                                    checksum_calculated += (data >> 8) & 0xFF;
                                    checksum_calculated += data & 0xFF;

                                    extended_address = ( (base_address << 16) | address);

                                    pm -> data[extended_address/2 + i]   = data;
                                    pm -> filled[extended_address/2 + i] = 1;
                            }

                        checksum_calculated = (checksum_calculated ^ 0xFF) + 1;

                        nread = sscanf(&line[9+4*i], "%2hhx", &checksum_read);
                        if (nread != 1) {
                                fprintf(stderr, "Error: cannot read checksum.\n");
                                free_picmemory(&pm);
                                return NULL;
                        }
                        if (debug) fprintf(stderr, "  checksum    = 0x%02X\n", checksum_read);

                        if (checksum_calculated != checksum_read) {
                                fprintf(stderr, "Error: checksum does not match.\n");
                                free_picmemory(&pm);
                                return NULL;
                        }

                        if (debug) fprintf(stderr, "\n");

                        if (record_type == 1)
                                break;
                }
                else {
                    fprintf(stderr, "Error: unexpected EOF.\n");
                    free_picmemory(&pm);
                    return NULL;
                }
        }

        fclose(fp);

        fprintf(stderr, "DONE!\n");

        return pm;
}

/* Write the filled cells in struct picmemory to a Intel HEX8M or HEX32 file */
void write_inhx(struct picmemory *pm, char *outfile, const struct picmicro *pic)
{
        FILE *fp;
        uint32_t base, j, k, start, stop;
        uint8_t  byte_count;
        uint32_t address;
        uint32_t base_address = 0x0000;
        uint8_t  record_type;
        uint16_t data, tmp;
        uint8_t  checksum;

        fp = fopen(outfile, "w");
        if (fp == NULL) {
                fprintf(stderr, "Error: cannot open destination file %s.\n", outfile);
                return;
        }

        fprintf(stderr, "Writing hex file...\n");

        /* Write the program memory bytes */

        for (base = 0; base < (pic -> program_memory_size) - 4; ){

            for (j = 0 ; j < (pic -> program_memory_size - base -4); j++){
                  if (pm -> filled[base+j]) break;
            }
            start = j;

            for ( ; j < (pic -> program_memory_size - base - 4); j++){
                 if (!pm -> filled[base+j] || (j-start == 8) ) break;
            }
            stop = j;

            byte_count  = (stop - start)*2;

            if (byte_count > 0) {
                address = (base + start)*2;
                record_type = 0x00;

                if(pic->program_memory_size >= 0x10000 && (address >> 16) != base_address){  //extended linear address
                        base_address = (address >> 16);
                        fprintf(fp, ":02000004%04X", base_address);
                        checksum = 0x06 + ((base_address>>8) & 0xFF) + (base_address & 0xFF);
                        checksum = (checksum ^ 0xFF) + 1;
                        fprintf(fp, "%02X\n", checksum);
                }
                    

                fprintf(fp, ":%02X%04X%02X", byte_count, address, record_type);

                checksum  = byte_count;
                checksum += (address >> 8) & 0xFF;
                checksum += address & 0xFF;
                checksum += record_type;

                for (k = start; k < stop; k++) {
                    data = pm -> data[base+k];
                    tmp = data;
                    data = (data >> 8) | (tmp << 8);
                    fprintf(fp, "%04X", data);
                    checksum += (data >> 8) & 0xFF;
                    checksum += data & 0xFF;

                }

                checksum = (checksum ^ 0xFF) + 1;
                fprintf(fp, "%02X\n", checksum);

            }
            base += stop;
        }

        /* Write the configuration words (last 4 bytes) */

        base = (pic -> program_memory_size) - 4;
        for (j = 0 ; j < 4; j++){
            if (pm -> filled[base+j]) break;
        }
        start = j;

        for ( ; j < 4; j++){
            if (!pm -> filled[base+j]) break;
        }
        stop = j;

        byte_count  = (stop - start)*2;

        if (byte_count > 0) {
            address = (base + start)*2;
            record_type = 0x00;
            fprintf(fp, ":%02X%04X%02X", byte_count, address, record_type);

            checksum  = byte_count;
            checksum += (address >> 8) & 0xFF;
            checksum += address & 0xFF;
            checksum += record_type;

            for (k = start; k < stop; k++) {
                  data = pm -> data[base+k];
                  tmp = data;
                  data = (data >> 8) | (tmp << 8);
                  fprintf(fp, "%04X", data);
                  checksum += (data >> 8) & 0xFF;
                  checksum += data & 0xFF;
            }

            checksum = (checksum ^ 0xFF) + 1;
            fprintf(fp, "%02X\n", checksum);
        }

        fprintf(fp, ":00000001FF\n");
        fclose(fp);

}
