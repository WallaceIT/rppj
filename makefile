#
# Makefile for rppj
#

CC = gcc
CFLAGS = -g -Wall -O3 -DNDEBUG -s
TARGET = rppj
PREFIX = /usr
BINDIR = $(PREFIX)/bin

all: $(TARGET)

rppj: rppj.o inhx8m.o
	$(CC) $(CFLAGS) -o $(TARGET) rppj.o inhx.o

inhx8m.o: inhx.c defs.h
	$(CC) $(CFLAGS) -c inhx.c

rppj.o:  rppj.c defs.h 
	$(CC) $(CFLAGS) -c rppj.c

install: $(TARGET)
	install -m 0755 $(TARGET) $(BINDIR)/$(TARGET)

uninstall:
	$(RM) $(BINDIR)/$(TARGET)

clean: 
	$(RM) $(TARGET) *.o
