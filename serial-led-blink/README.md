# Serial led blink

A small bare-metal AVR C project that sends newline-terminated commands
from a Linux host program to an ATmega328P-based Arduino-compatible
board over USB serial.

The firmware parses a command, replies over UART, and blinks the board
LED at the requested speed.

## Current commands

```text
FAST\n  -> FAST OK\r\n, then blink PB5 / built-in LED quickly three times
SLOW\n  -> SLOW OK\r\n, then blink PB5 / built-in LED slowly three times
other   -> ERR\r\n
quit
```

## Build and run

Requirements on Arch Linux:

```sh
sudo pacman -S avr-gcc avr-libc avrdude
```

Build the AVR firmware and host client:

```sh
make all
```

Flash the AVR firmware to the connected board:

```sh
make flash
```

Run the host client:

```sh
make send
```
