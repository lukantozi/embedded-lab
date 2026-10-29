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

#define USART_REPORT_DATA(ival, dval, type)       \
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

void twi_init(void)
{
    TWSR &= ~3;                          // prescalar value 1
    TWBR = (F_CPU / SCL_FREQ - 16) / 2;
    TWCR = 1 << TWEN;                    // enable twi
}

void twi_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
    while (TWCR & (1 << TWSTO));
}

#define REPORT_ERROR(blame, from, n, t, r)                     \
do {                                                           \
    usart_data_transmit("[" blame " failed] " from #n" \r\n"); \
    if (t == 1)                                                \
        twi_stop();                                            \
    if (r == 0)                                                \
        return 0;                                              \
} while (0)


int transaction_begin(void)
{
    TWCR |= (1 << TWINT) | (1 << TWSTA);              // send START cond
    while (!(TWCR & (1 << TWINT)));                   // wait for TWINT flag set
    if ((TWSR & 0xf8) != START)
        REPORT_ERROR("START set", "transaction_begin", 1, 0, 0);

    TWDR = SLAVE_ADDRESS;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    if ((TWSR & 0xf8) != MT_SLA_ACK)
        REPORT_ERROR("SLA set", "transaction_begin", 1, 0, 0);

    return 1;
}

int pcf8574_write(unsigned char c)
{
    TWDR = c;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    if ((TWSR & 0xf8) != MT_DATA_ACK)
        REPORT_ERROR("DATA ACK", "pcf8574_write", 1, 0, 0);
    return 1;
}

int write_4bits(unsigned char bits)
{
    if (pcf8574_write(bits) == 0)
        REPORT_ERROR("pcf8574_write", "write_4bits", 1, 0, 0);
    _delay_us(1);

    if (pcf8574_write(bits | (1 << 2)) == 0)
        REPORT_ERROR("pcf8574_write", "write_4bits", 2, 0, 0);
    _delay_us(1);

    if (pcf8574_write(bits) == 0)
        REPORT_ERROR("pcf8574_write", "write_4bits", 3, 0, 0);
    _delay_us(50);

    return 1;
}

int lcd_prepare(void)
{
    _delay_ms(20);           // at least 15ms less than 40ms for 4.5V

    if (write_4bits(0x38) == 0)
        REPORT_ERROR("write_4bits", "lcd_prepare", 1, 0, 0);
    _delay_ms(5);            // at least 4.1 ms

    if (write_4bits(0x38) == 0)
        REPORT_ERROR("write_4bits", "lcd_prepare", 2, 0, 0);
    _delay_us(150);         // at least 100us

    if (write_4bits(0x38) == 0)
        REPORT_ERROR("write_4bits", "lcd_prepare", 3, 0, 0);
    _delay_us(150);

    if (write_4bits(0x28) == 0)
        REPORT_ERROR("write_4bits", "lcd_prepare", 4, 0, 0);

    return 1;
}

int lcd_send_bytes(unsigned char bytes, unsigned char rs)
{
    unsigned char control = 0x08 | (rs & 1);

    if (write_4bits((bytes & 0xf0) | control) == 0)
        REPORT_ERROR("write_4bits", "lcd_send_bytes", 1, 0, 0);

    if (write_4bits(((bytes << 4) & 0xf0) | control) == 0)
        REPORT_ERROR("write_4bits", "lcd_send_bytes", 2, 0, 0);

    return 1;
}

int lcd_send_string(const char *s)
{
    while (*s) {
        if (lcd_send_bytes(*s++, 1) == 0)
            REPORT_ERROR("lcd_send_bytes", "lcd_send_string", 1, 0, 0);
    }
    return 1;
}

int lcd_display_clear(void)
{
    if (lcd_send_bytes(0x01, 0) == 0)
        REPORT_ERROR("lcd_send_bytes", "lcd_display_clear", 1, 0, 0);
    _delay_ms(2);

    if (lcd_send_bytes(0x80, 0) == 0) 
        REPORT_ERROR("lcd_send_bytes", "lcd_display_clear", 2, 0, 0);

    return 1;
}

int message_sensor_ready(void)
{
    if (transaction_begin() == 0) {
        REPORT_ERROR("transaction_begin", "message_sensor_ready", 1, 1, 0);
    }

    if (lcd_display_clear() == 0) {
        REPORT_ERROR("lcd_display_clear", "message_sensor_ready", 1, 1, 0);
    }

    if (lcd_send_string("Sensor ready.") == 0) {
        REPORT_ERROR("lcd_send_string", "message_sensor_ready", 1, 1, 0);
    }

    twi_stop();
    _delay_ms(1000);
    return 1;
}
void usart_report_data(uint8_t ival_h, uint8_t dval_h, uint8_t ival_t, uint8_t dval_t)
{
    char data[5];
    USART_REPORT_DATA(ival_h, dval_h, "Humidity");
    USART_REPORT_DATA(ival_t, dval_t, "Temperature");
}

int lcd_display_formatted(uint8_t ival, uint8_t dval, const char *label)
{
    char data[5];

    if (lcd_send_string(label) == 0)
        REPORT_ERROR("lcd_send_string", "lcd_display_formatted", 1, 0, 0);

    itoa(ival, data, 10);
    if (lcd_send_string(data) == 0)
        REPORT_ERROR("lcd_send_string", "lcd_display_formatted", 2, 0, 0);

    if (lcd_send_bytes('.', 1) == 0)
        REPORT_ERROR("lcd_send_bytes", "lcd_display_formatted", 1, 0, 0);

    itoa(dval, data, 10);
    if (lcd_send_string(data) == 0)
        REPORT_ERROR("lcd_send_string", "lcd_display_formatted", 3, 0, 0);

    if (lcd_send_string(label[0] == 'H' ? " %" : " C") == 0)
        REPORT_ERROR("lcd_send_string", "lcd_display_formatted", 4, 0, 0);

    return 1;
}

int lcd_show_data(uint8_t ival_h, uint8_t dval_h, uint8_t ival_t, uint8_t dval_t)
{
    if (transaction_begin() == 0)
        REPORT_ERROR("transaction_begin", "lcd_show_data", 1, 1, 0);

    if (lcd_display_clear() == 0)
        REPORT_ERROR("lcd_display_clear", "lcd_show_data", 1, 1, 0);

    if (lcd_display_formatted(ival_h, dval_h, "Humi: ") == 0)
        REPORT_ERROR("lcd_display_formatted", "lcd_show_data", 1, 1, 0);

    if (lcd_send_bytes(0xc0, 0) == 0)
        REPORT_ERROR("lcd_send_bytes", "lcd_show_data", 1, 1, 0);

    if (lcd_display_formatted(ival_t, dval_t, "Temp: ") == 0)
        REPORT_ERROR("lcd_display_formatted", "lcd_show_data", 2, 1, 0);

    twi_stop();
    return 1;
}

int lcd_init(void)
{
    if (transaction_begin() == 0)
        REPORT_ERROR("transaction_begin", "lcd_init", 1, 1, 0);

    if (lcd_prepare() == 0)
        REPORT_ERROR("lcd_prepare", "lcd_init", 1, 1, 0);

    if (lcd_send_bytes(0x28, 0) == 0)
        REPORT_ERROR("lcd_send_bytes", "lcd_init", 1, 1, 0);

    if (lcd_send_bytes(0x08, 0) == 0) // display off at startup
        REPORT_ERROR("lcd_send_bytes", "lcd_init", 2, 1, 0);

    if (lcd_send_bytes(0x01, 0) == 0) // clear display
        REPORT_ERROR("lcd_send_bytes", "lcd_init", 2, 1, 0);
    _delay_ms(2);

    if (lcd_send_bytes(0x06, 0) == 0) // auto increment position
        REPORT_ERROR("lcd_send_bytes", "lcd_init", 3, 1, 0);

    if (lcd_send_bytes(0x0c, 0) == 0) // display on
        REPORT_ERROR("lcd_send_bytes", "lcd_init", 4, 1, 0);

    if (lcd_send_string("Booting...") == 0)
        REPORT_ERROR("lcd_send_string", "lcd_init", 1, 1, 0);

    twi_stop();
    _delay_ms(1000);
    return 1;
}

int lcd_show_error(char *error)
{
    if (transaction_begin() == 0)
        REPORT_ERROR("transaction_begin", "lcd_show_error", 1, 1, 0);

    if (lcd_display_clear() == 0)
        REPORT_ERROR("lcd_display_clear", "lcd_show_error", 1, 1, 0);

    if (lcd_send_string(error) == 0)
        REPORT_ERROR("lcd_send_string", "lcd_show_error", 1, 1, 0);

    twi_stop();
    return 1;
}

int main(void)
{
    uint8_t ival_h;
    uint8_t dval_h;
    uint8_t ival_t;
    uint8_t dval_t;
    uint8_t checksum;

    usart_init();
    usart_data_transmit("[Done] usart_init\r\n");
    twi_init();
    usart_data_transmit("[Done] twi_init\r\n");
    if (lcd_init() == 0)
        REPORT_ERROR("lcd_init", "main", 1, 0, 1);
    else
        usart_data_transmit("[Done] lcd_init\r\n");

    if (message_sensor_ready() == 0)
        REPORT_ERROR("message_sensor_ready", "main", 1, 0, 1);

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
            usart_report_data(ival_h, dval_h, ival_t, dval_t);
            if (lcd_show_data(ival_h, dval_h, ival_t, dval_t) == 0)
                REPORT_ERROR("lcd_show_data", "main", 1, 0, 1);
        } else {
            usart_data_transmit("Summed doesn't match checksum\r\n");
            if (lcd_show_error("Checksum error") == 0)
                REPORT_ERROR("lcd_show_error", "main", 1, 0, 1);
        }

        _delay_ms(1500);
    }
#else // TODO: add user responsiveness
        unsigned char c;
        unsigned char buf[34];
        unsigned int i;

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
