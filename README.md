# embedded-lab

An embedded-systems lab for building small, low-level projects in
C and learning microcontroller programming.

## Projects

- [`serial-led-blink/`](./serial-led-blink/): linux `termios` host client and
bare-metal ATmega328P UART firmware. The host sends text commands over
USB serial; the board parses them, replies, and controls its built-in LED.
