# embedded-lab

A personal embedded-systems lab for building small, low-level projects in
C and learning microcontroller peripherals through direct register access.

## Projects

- [`serial-ports/`](./serial-ports/) — Linux `termios` host client and
bare-metal ATmega328P UART firmware. The host sends text commands over
USB serial; the board parses them, replies, and controls its built-in LED.

## Approach

Projects are built incrementally with C, vendor datasheets, command-line
toolchains, tests or hardware checks where appropriate, and short documentation
of protocol and build decisions.
