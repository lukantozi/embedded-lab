#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stddef.h>

#define BLINK_DELAY_MS 250
#define BLINK_DELAY_SLOW_MS 750

void UART_init(unsigned int baud) {
    unsigned int ubrr = F_CPU / 16 / baud - 1;
    UBRR0H = (ubrr >> 8);
    UBRR0L = ubrr;
    UCSR0B = (1 << TXEN0) | (1 << RXEN0); // enable TX and RX
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data, 1 stop bit
}

void UART_send_char(char data) {
    while (!(UCSR0A & (1 << UDRE0))); // wait for transmit buffer to be empty
    UDR0 = data;
}

void UART_send_string(const char *str) {
    while (*str) {
        UART_send_char(*str++);
    }
}

char UART_receive_char() {
    while (!(UCSR0A & (1 << RXC0))); // wait for received data
    return UDR0;
}

int main (void)
{
    /* set pin 5 of PORTB for output*/
    UART_init(9600);

    DDRB |= _BV(DDB5);
    int i = 0;
    char buf[100];
    size_t buf_size = 0;
    const char fast[] = "FAST";
    const char slow[] = "SLOW";
    int fast_flag = 1;
    int slow_flag = 1;

    while(1) {

        char byte;
        while ((byte = UART_receive_char()) != '\n') {
            if (buf_size == sizeof(buf) - 1) {
                UART_send_string("ERR command too long\r\n");

                while (UART_receive_char() != '\n') {
                }

                buf_size = 0;
                continue;
            }
            buf[buf_size++] = byte;
        }

        buf[buf_size] = '\0';

        i = 0;
        while (fast[i] != '\0' && (fast[i] == buf[i])) {
            i++;
        }
        fast_flag = (fast[i] == '\0' && buf[i] == '\0');

        i = 0;
        while (slow[i] != '\0' && (slow[i] == buf[i])) {
            i++;
        }
        slow_flag = (slow[i] == '\0' && buf[i] == '\0');

        if (fast_flag) 
            UART_send_string("FAST OK\r\n");
        else if (slow_flag)
            UART_send_string("SLOW OK\r\n");
        else
            UART_send_string("ERR\r\n");

        buf[0] = 0;
        buf_size = 0;

        if (fast_flag) {
            for (i = 0; i < 3; i++) {
                /* set pin 5 high */
                PORTB |= _BV(PORTB5);
                _delay_ms(BLINK_DELAY_MS);

                /* set pin 5 low */
                PORTB &= ~_BV(PORTB5);
                _delay_ms(BLINK_DELAY_MS);
            }
        } else if (slow_flag){
            for (i = 0; i < 3; i++) {
                /* set pin 5 high */
                PORTB |= _BV(PORTB5);
                _delay_ms(BLINK_DELAY_SLOW_MS);

                /* set pin 5 low */
                PORTB &= ~_BV(PORTB5);
                _delay_ms(BLINK_DELAY_SLOW_MS);
            }
        }
    }
}
