# Serial Ports: AVR UART Command Prototype

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
```

The host program also accepts:

```text
quit
```

which exits the host client.

## Layout

```text
serial-ports/
├── led.c       # ATmega328P firmware: UART setup, command parsing, LED control
├── send.c      # Linux host client using termios
├── ports.c     # Serial-port discovery/check helper
├── Makefile
└── build/      # Generated files; ignored by Git
```

## Protocol

The application protocol is line-oriented ASCII text:

```text
Host -> board: command followed by \n
Board -> host: response followed by \r\n
```

Runtime UART configuration on both sides is **9600 8N1**: 9600 baud,
8 data bits, no parity, one stop bit, and no flow control.

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

The Makefile's upload port must identify the board. Prefer a stable
`/dev/serial/by-id/...` path over a changing `/dev/ttyUSB*` path.

## Notes

- Firmware upload uses the Arduino-compatible bootloader at 115200 baud.
- The running host-to-firmware UART protocol uses 9600 baud. These are
  separate communication phases and do not need the same baud rate.
- UART receive/transmit is currently implemented with polling and blocking
  delays. This is intentional for the first working version; later work can
  replace blocking behavior with a timer-driven state machine or interrupts.
