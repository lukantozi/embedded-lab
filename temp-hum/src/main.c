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
#define SCL_FREQ 100000
#define START 0x08
#define MT_SLA_ACK 0x18
#define MT_DATA_ACK 0x28
#define SLAVE_ADDRESS 0x4e // from testing all write values

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
} while (0)

void twi_init(void)
{
    TWSR &= ~3;                          // prescalar value 1
    TWBR = (F_CPU / SCL_FREQ - 16) / 2;
    TWCR = 1 << TWEN;                    // enable twi
}

void transaction_begin(void)
{
    TWCR |= (1 << TWINT) | (1 << TWSTA);              // send START cond
    while (!(TWCR & (1 << TWINT)));                   // wait for TWINT flag set
    if ((TWSR & 0xf8) != START) {
        usart_data_transmit("START didn't transmit\r\n");
        return;                                       // handle error
    }

    TWDR = SLAVE_ADDRESS;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    if ((TWSR & 0xf8) != MT_SLA_ACK) {
        usart_data_transmit("SLA didn't transmit\r\n");
        return;                                       // handle error
    }
}

void pcf8574_write(unsigned char c)
{
    TWDR = c;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    if ((TWSR & 0xf8) != MT_DATA_ACK) {
        usart_data_transmit("DATA didn't transmit\r\n");
        return;
    }
}

void write_4bits(unsigned char bits)
{
    pcf8574_write(bits);
    _delay_us(1);

    pcf8574_write(bits | (1 << 2));
    _delay_us(1);

    pcf8574_write(bits);
    _delay_us(50);
}

void twi_prepare_signals(void)
{
    _delay_ms(50);

    write_4bits(0x38);
    _delay_ms(5);

    write_4bits(0x38);
    _delay_ms(5);

    write_4bits(0x38);
    _delay_ms(5);
}

void lcd_send_bytes(unsigned char bytes, unsigned char rs)
{
    unsigned char control = 0x08 | (rs & 1);

    write_4bits((bytes & 0xf0) | control);
    write_4bits(((bytes << 4) & 0xf0) | control);
}

void twi_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
    while (TWCR & (1 << TWSTO));
}

void lcd_send_string(char *s)
{
    while (*s) {
        lcd_send_bytes(*s++, 1);
    }
}

void lcd_display_clear(void)
{
    lcd_send_bytes(0x01, 0);
    _delay_ms(2);
    lcd_send_bytes(0x80, 0);
}

void message_sensor_ready(void)
{
    transaction_begin();
    lcd_display_clear();
    lcd_send_string("Sensor ready.");
}

#define LCD_DISPLAY_FORMATTED(ival, dval, type)         \
do {                                                    \
    lcd_send_string(type);                              \
    itoa(ival, data, 10);                               \
    lcd_send_string(data);                              \
    lcd_send_bytes('.', 1);                             \
    itoa(dval, data, 10);                               \
    lcd_send_string(data);                              \
    lcd_send_string(type[0] == 'H' ? " %" : " C");      \
} while (0)

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
    twi_init();
    transaction_begin();
    twi_prepare_signals();
    write_4bits(0x28);       // enter 4 bit mode

    lcd_send_bytes(0x28, 0); // configure 4-bit, 2 lines, 5x8 font
    lcd_send_bytes(0x08, 0); // display off at startup
    lcd_send_bytes(0x01, 0); // clear display
    _delay_ms(2);
    lcd_send_bytes(0x06, 0); // auto increment position
    lcd_send_bytes(0x0c, 0); // display on
    lcd_send_string("Booting...");
    twi_stop();
    _delay_ms(1000);

    message_sensor_ready();
    twi_stop();
    _delay_ms(1000);

    unsigned char c;
    unsigned char buf[34];
    unsigned int i;

#if 1
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
            /* usart */
            SEND_FORMATTED_DATA(ival_h, dval_h, "Humidity");
            SEND_FORMATTED_DATA(ival_t, dval_t, "Temperature");
            /* lcd */
            transaction_begin();
            lcd_display_clear();
            // LCD_DISPLAY_FORMATTED(1, 1, "Humi: ");
            LCD_DISPLAY_FORMATTED(ival_h, dval_h, "Humi: ");
            lcd_send_bytes(0xc0, 0);
            LCD_DISPLAY_FORMATTED(ival_t, dval_t, "Temp: ");
            // LCD_DISPLAY_FORMATTED(1, 1, "Temp: ");
            twi_stop();
        } else {
            usart_data_transmit("Summed doesn't match checksum\r\n");
            transaction_begin();
            lcd_display_clear();
            lcd_send_string("Summed doesn't match checksum");
            twi_stop();
        }

        _delay_ms(1500);
    }
#else // add user responsiveness
        while (1) {
            i = 0;
            usart_data_transmit("Send message: ");
            while ((c = usart_char_receive()) != '\r' && i < 33) {
                if (i == 16) {
                    buf[i++] = 0;
                }
                usart_char_transmit(c);
                buf[i++] = c;
            }
            usart_data_transmit("\r\n");
            buf[i] = 0;

            transaction_begin();
            lcd_send_bytes(0x01, 0);
            _delay_ms(2);
            lcd_send_bytes(0x80, 0); 

            lcd_send_string(buf);
            lcd_send_bytes(0xc0, 0); // newline
            if (i > 17)
                lcd_send_string(buf + 17);
            twi_stop();
            memset(buf, 0, sizeof(buf));
        }
#endif
}
