#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    int fd = open("/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0", O_RDWR | O_NOCTTY);

    if (fd == -1) {
        perror("Failed to open the serial port");
        return 1;
    } else {
        printf("Connection to port opened fd = %d\n", fd);
    }

    tcflush(fd, TCIOFLUSH);

    struct termios serial_port;

    /* get the serial port settings from the termios structure */
    tcgetattr(fd, &serial_port);

    serial_port.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  /* enable non-canonical mode */
    serial_port.c_iflag &= ~(IXON | IXOFF | IXANY);          /* turn off software based flow control (XON/XOFF) */
    serial_port.c_iflag &= ~(ICRNL | INLCR | IGNCR); /* host sends '\n' device receives exactly '\n' */
    serial_port.c_oflag &= ~OPOST;

    serial_port.c_cflag |= CREAD | CLOCAL; /* turn ON the receiver of the serial port (CREAD) */
    serial_port.c_cflag &= ~CRTSCTS;       /* turn OFF hardware based flow control RTS/CTS */

    serial_port.c_cflag &= ~PARENB; // no parity
    serial_port.c_cflag &= ~CSTOPB; // one stop bit
    serial_port.c_cflag &= ~CSIZE;
    serial_port.c_cflag |= CS8;     // 8 bits
    serial_port.c_cc[VMIN] = 1;
    serial_port.c_cc[VTIME] = 0;

    /* update I/O speed */
    cfsetispeed(&serial_port, B9600);
    cfsetospeed(&serial_port, B9600);

    /* update new settings to termios structure */
    tcsetattr(fd, TCSANOW, &serial_port);

    char input[100];
    char resp[100];
    int c;
    size_t input_size;
    ssize_t resp_size;
    while (fgets(input, 100, stdin)) {
        if (strcmp(input, "quit\n") == 0)
            break;

        char *newline = strchr(input, '\n');

        if (newline == NULL) {
            fprintf(stderr, "command too long\n");

            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
            }

            continue;
        }

        input_size = (size_t)(newline - input) + 1;
        write(fd, input, input_size);
        input[0] = 0;

        size_t used = 0;
        while (used < sizeof(resp) - 1) {
            resp_size = read(fd, &resp[used], 1);
            if (resp_size == -1) {
                perror("read");
                break;
            }

            if (resp_size == 0) {
                fprintf(stderr, "no response\n");
                break;

            }
            if (resp[used++] == '\n') {
                break;
            }
        }
        resp[used] = '\0';

        if (used > 0) {
            printf("answer from uno: %s", resp);
        }

        resp[0] = 0;
    }
    close(fd);
}
