#ifndef F_CPU
    #define F_CPU 16000000U
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

#define BAUD 9600
#define UBRR (F_CPU / 16 / BAUD - 1)
#define DHT11_PIN 6

void usart_init(void)
{
    unsigned int ubrr = UBRR;
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);   // enable t/r
    UCSR0C = (1 << UCSZ00) | (1 << UCSZ01); // set 8N1
}             

void usart_char_transmit(unsigned char c)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

unsigned char usart_char_receive(void)
{
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

void usart_data_transmit(char *msg)
{
    while (*msg) {
        usart_char_transmit(*msg++);
    }
}

void dht11_request(void)
{
    DDRD |= (1 << DHT11_PIN);   // config as output
    PORTD &= ~(1 << DHT11_PIN); // low
    _delay_ms(20);              // at least 18
    PORTD |= (1 << DHT11_PIN);  // high
}

void dht11_response(void)
{
    DDRD &= ~(1 << DHT11_PIN);              // config as input
    while (PIND & (1 << DHT11_PIN));        // wait for high->low
    while ((PIND & (1 << DHT11_PIN)) == 0); // wait for low->high
    while (PIND & (1 << DHT11_PIN));        // wait for high->low
}

uint8_t dht11_data_receive(void)
{
    uint8_t byte = 0;
	for (int i = 0; i < 8; i++) {
		while ((PIND & (1 << DHT11_PIN)) == 0);  // wait for rising
		_delay_us(30);
		if (PIND & (1 << DHT11_PIN))             // still high after 30us
            byte = (byte << 1) | (0x01);	     // '1' data indication
        else
            byte = (byte << 1);                  // '0' data indication
		while (PIND & (1 << DHT11_PIN));         // wait for high signal to end
	}
	return byte;
}

#define SEND_FORMATTED_DATA(ival, dval, type)     \
do {                                              \
    usart_data_transmit(type);                    \
    usart_data_transmit(": ");                    \
    itoa(ival, data, 10);                         \
    usart_data_transmit(data);                    \
    usart_char_transmit('.');                     \
    itoa(dval, data, 10);                         \
    usart_data_transmit(data);                    \
    usart_data_transmit("\r\n");                  \
} while (0)                                       \

int main(int argc, char *argv[])
{
    uint8_t ival_h;
    uint8_t dval_h;
    uint8_t ival_t;
    uint8_t dval_t;
    uint8_t checksum;
    char data[5];

    usart_init();
    usart_data_transmit("Sensor monitor ready\r\n");
    _delay_ms(1000); // pass the unstable status

    while (1) {
        dht11_request();
        dht11_response();

        ival_h = dht11_data_receive();
        dval_h = dht11_data_receive();
        ival_t = dht11_data_receive();
        dval_t = dht11_data_receive();
        checksum = dht11_data_receive();

        uint8_t summed = (uint8_t)(ival_h + dval_h + ival_t + dval_t);
        if (summed == checksum) {
            SEND_FORMATTED_DATA(ival_h, dval_h, "Humidity");
            SEND_FORMATTED_DATA(ival_t, dval_t, "Temperature");
        } else {
            usart_data_transmit("Summed doesn't match checksum\r\n");
        }

        _delay_ms(1500);
    }
}
