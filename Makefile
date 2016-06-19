#!/bin/bash

CC = arm-linux-gnueabihf-gcc
#CC = gcc
CFLAGS = -Wall -g
SRCFILES = adc.c
EXE = adc

$(EXE):$(SRCFILES) 
	$(CC) -std=gnu99 -g -o $@ $^ -lrt
	
clean:
	@echo Cleaning...
	rm -f *.o *~ $(EXE)
	
connect:
	ssh root@192.168.7.2

push:
	sudo scp $(EXE) root@192.168.7.2:~/lab4
