MCU		= atmega328p
F_CPU	= 16000000UL
PORT	= /dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
PROGRAM = build/led
HOST 	= send.c
SEND	= build/send

CC		= avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

CFLAGS    = -Os -DF_CPU=$(F_CPU) -mmcu=$(MCU)
LDFLAGS   = -mmcu=$(MCU)
DUDEFLAGS = -c arduino -p m328p -P $(PORT) -b 115200

.PHONY: all send program flash clean check

all: $(SEND) $(PROGRAM).hex

check: ports.c
	mkdir -p build
	gcc ports.c -o build/ports
	./build/ports

$(SEND): $(HOST)
	mkdir -p build
	gcc -o $@ $<

send: $(SEND)
	./$(SEND)

program: $(PROGRAM).hex

$(PROGRAM).o: led.c
	mkdir -p build
	$(CC) $(CFLAGS) -c -o $@ $<

$(PROGRAM): $(PROGRAM).o
	mkdir -p build
	$(CC) $(LDFLAGS) -o $@ $<

$(PROGRAM).hex: $(PROGRAM) 
	mkdir -p build
	$(OBJCOPY) -O ihex -R .eeprom $< $@

flash: $(PROGRAM).hex
	$(AVRDUDE) $(DUDEFLAGS) -U flash:w:$(PROGRAM).hex

clean:
	rm -f build/*
