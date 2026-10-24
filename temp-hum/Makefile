MCU = atmega328p
F_CPU = 16000000UL
PORT = /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0

CC = gcc
CCARV = avr-gcc
OBJCOPY = avr-objcopy
ARVDUDE = avrdude
CFLAGS = -Os -DF_CPU=$(F_CPU) -mmcu=$(MCU)
LDFLAGS = -mmcu=$(MCU)
DUDEFLAGS = -c arduino -p m328p -P $(PORT) -b 115200

SRC = src/main.c
BUILD_DIR = build
TARGET = build/main

.PHONY: all clean

all: $(TARGET).hex

flash: $(TARGET).hex
	$(ARVDUDE) $(DUDEFLAGS) -U flash:w:$<

run: $(TARGET).hex
	picocom -b 9600 /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0

flrun: $(TARGET).hex
	$(ARVDUDE) $(DUDEFLAGS) -U flash:w:$<
	picocom -b 9600 /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0


$(TARGET).o: $(SRC)
	mkdir -p $(BUILD_DIR)
	$(CCARV) $(CFLAGS) -c -o $@ $<

$(TARGET).elf: $(TARGET).o
	mkdir -p $(BUILD_DIR)
	$(CCARV) $(LDFLAGS) -o $@ $<

$(TARGET).hex: $(TARGET).elf
	mkdir -p $(BUILD_DIR)
	$(OBJCOPY) -O ihex -R .eeprom $< $@

clean:
	rm -rf $(BUILD_DIR)/*
