#include <stdlib.h>

void main(void) {
    system("ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null");
}
