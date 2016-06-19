#!/bin/bash
arm-linux-gnueabihf-gcc -std=gnu99 -o adc adc.c -lrt
target_PID="$(pgrep -f motor)"
./adc 10 $target_PID
