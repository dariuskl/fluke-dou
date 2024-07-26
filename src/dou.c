// Logic that is common to all devices (8000A, 1900A, etc.).

#include "dou.h"

// The baud rate must be sufficient to transmit the whole measurement within
// the nMUP/nT period of approximately 100 ms.
#define SERIAL_BAUD_RATE    19200 // bps
#define SERIAL_DATA_BITS    7
#define STOP_BIT            (1U << ((SERIAL_DATA_BITS) + 1))

// Extracts the digit with the given index from a BCD sequence.
// Digits are indexed MSD = N, 2SD = N-1, 3SD = N-2, ..., LSD = 0.
#define DIGIT(reading, idx) (((reading) >> ((unsigned)(idx) * 4U)) & 0xfU)

// The BCD value that is interpreted as a decimal point by `bcd2digit()`.
#define DECIMAL_POINT_BCD   (0xbU)

// Converts the given BCD value to its character representation.
// Works not only for digits, but also for some useful special characters.
static char bcd2digit(const unsigned bcd) {
  static const char digits[] = "0123456789a.cdef";
  return digits[bcd & 0xfU];
}

static char *print_str(char *const begin, const char *end, const char *str) {
  char *dst = begin;
  for (; *str != '\0' && dst != end; ++dst, ++str) {
    *dst = *str;
  }
  return dst;
}
