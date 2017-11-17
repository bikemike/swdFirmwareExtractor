#
# Copyright (C) 2017 Obermaier Johannes
#
# This Source Code Form is subject to the terms of the MIT License.
# If a copy of the MIT License was not distributed with this file,
# you can obtain one at https://opensource.org/licenses/MIT
#

TARGET=swdFirmwareExtractor
EXECUTABLE=$(TARGET).elf

#compiler flags
CFLAGS = -mthumb -mcpu=cortex-m0 -g3 -O0 -D STM32F030 -Wall -Wextra

#linker flags
LDFLAGS = -T link.ld -nostartfiles

#cross compiler
CC = arm-none-eabi-gcc
CP = arm-none-eabi-objcopy

all: $(TARGET)

$(TARGET): $(EXECUTABLE)
	$(CP) -O binary $^ $@


$(EXECUTABLE): main.o clk.o swd.o target.o uart.o st/startup_stm32f0.o
	$(CC) $(LDFLAGS) $(CFLAGS) main.o clk.o swd.o target.o uart.o st/startup_stm32f0.o -o swdFirmwareExtractor.elf

main.o: main.c main.h
	$(CC) $(CFLAGS) -c main.c -o main.o

clk.o: clk.c clk.h
	$(CC) $(CFLAGS) -c clk.c -o clk.o

swd.o: swd.c swd.h
	$(CC) $(CFLAGS) -c swd.c -o swd.o

target.o: target.c target.h
	$(CC) $(CFLAGS) -c target.c -o target.o

uart.o: uart.c uart.h
	$(CC) $(CFLAGS) -c uart.c -o uart.o

st/startup_stm32f0.o: st/startup_stm32f0.S
	$(CC) $(CFLAGS) -c st/startup_stm32f0.S -o st/startup_stm32f0.o

clean:
	rm -f main.o clk.o swd.o target.o uart.o st/startup_stm32f0.o


erase:
	openocd -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f0x.cfg -c init -c "reset halt" -c "stm32f0x unlock 0" -c "reset run" -c shutdown

flash:
	openocd -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f0x.cfg -c init -c "reset halt" -c "flash write_image erase $(TARGET) 0x08000000" -c "verify_image $(TARGET) 0x08000000" -c "reset run" -c shutdown

flashbwoop:
	openocd -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f0x.cfg -c init -c "reset halt" -c "flash write_image erase cli/bwhoop.bin 0x08000000" -c "verify_image cli/bwhoop.bin 0x08000000" -c "reset run" -c shutdown

