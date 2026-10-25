# temp-hum

Temperature and humidity monitor using ATmega328P and DHT11 module,
with data displayed on an LCD.

## Build, flash, and run:

```sh
make flrun
```

### Wiring notes:
*(my dht11 module is distinct from the one in the datasheet)*

#### DHT11

Looking at the blue sensor face with the three connector
pins pointing down, leftmost module pin is connected to
arduino pin 6, middle one to 5V and the rightmost to gnd.

#### LCD 16X2 with pcf8574 I2C backpack

SCL <--> A5; SDA <--> A4; VCC <--> 5V; GND <--> GND;
