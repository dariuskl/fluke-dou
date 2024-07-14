
// The baud rate must be sufficient to transmit the whole measurement within
// the /MUP period of approximately 100 ms.
#define SERIAL_BAUD_RATE 19200 // bps
#define SERIAL_DATA_BITS 7

enum unit { ms, us, MHz, kHz, NoUnit };
