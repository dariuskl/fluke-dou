#define nullptr ((void *)0)

typedef unsigned char byte;

// The baud rate must be sufficient to transmit the whole measurement within
// the /MUP period of approximately 100 ms.
#define SERIAL_BAUD_RATE 19200 // bps
#define SERIAL_DATA_BITS 7

static char *print_str(char *buf, const int len, const char *str) {
  char *dst = buf;
  for (; *str != '\0' && buf != buf + len; ++buf, ++str) {
    *dst = *str;
  }
  return dst;
}
