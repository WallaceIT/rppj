Raspberry Pi PIC18 J-Series Programmer using GPIO connector

Copyright 2013 Francesco Valla
Based on rpp - Copyright Giorgio Vazzana - http://holdenc.altervista.org/rpp/

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.


# Overview

rppj is a Raspberry Pi PIC18F J-series programmer using GPIOs that doesn't require additional programming hardware.
Currently it has theorical support for severl PICs, although only PIC18F24J50 has been tested.

rppj is inspired by and based on [rpp](http://holdenc.altervista.org/rpp/) by Holden.

# Building and Installing rppj

To compile rppj, after cloning the repository, just enter in its directory and launch

	make

and then
	
	sudo make install

to install it in /usr/bin.

To change destination prefix use PREFIX=, e.g.

	sudo make install PREFIX=/usr/local


Compilation is possible either on the RPi or in a cross-build environment.

# Usage of rppj

	rppj [options]
       
Supported options:

	-h                print help
	-D                turn debug on
	-g PGC,PGD,MCLR   GPIO selection, default if not present
	-i file           input file
	-o file           output file (ofile.hex)
	-p part           part (optional - If present, all uppercase - )
	-r                read chip
	-w                bulk erase and write chip
	-e                bulk erase chip
	-b                blank check of the chip

For Example, to connect the PIC to GPIOs 11 (PGC), 9 (PGD), 22 (MCLR) and write on a PIC18F24J50 the file fw.hex:

	rppj -w -g 11,9,22 -p PIC18F24J50 -i fw.hex

# Hardware

To use rppj you will need only the "recommended minimum connections" outlined in each PIC datasheet (avoiding the cap on MCLR), as shown in `minimum_connections.png`.

Between PIC and RPi you must have the four basic ICSP lines: PGC (clock), PGD (data), MCLR (Reset), GND.
You can also connect PIC VDD line to Raspberry Pi 3v3 line, but do this carefully: Raspberry Pi 3v3 pins have only 50mA of current capability, so consider your circuit current drawn!

If not specified in the command line, the default GPIOs <-> PIC connections are:

	PGC  <-> GPIO11
	PGD  <-> GPIO9
	MCLR <-> GPIO22

# Supported PICs

(* only theorical support, feel free to test and report!)

- PIC18F44J10*
- PIC18F45J10*
- PIC18F24J11*
- PIC18F25J11*
- PIC18F26J11*
- PIC18F44J11*
- PIC18F45J11*
- PIC18F46J11*
- PIC18F24J50
- PIC18F25J50*
- PIC18F26J50*
- PIC18F44J50*
- PIC18F45J50*
- PIC18F46J50*
- PIC18F26J13*
- PIC18F46J13*
- PIC18F26J53*
- PIC18F46J53*
- PIC18F27J13*
- PIC18F27J53*
- PIC18F47J13*
- PIC18F47J53*

PLEASE NOTE: do NOT TRY to use rppj with other PIC families: it won't work and WILL seriously damage your Raspberry Pi!

# References

- [PIC18F2XJXX/4XJXX Family Programming Specification](http://ww1.microchip.com/downloads/en/DeviceDoc/39687e.pdf)

# Licensing

rppj is released under the GPLv3 license; for full license see the `COPYING` file.

The Microchip name and logo, PIC, In-Circuit Serial Programming, ICSP are registered trademarks of Microchip Technology Incorporated in the U.S.A. and other countries.



