// Decode logic for the Fluke 1900A DOU.

#include "dou.c"

#include <stdbool.h>
#include <stdint.h>

#define NUMBER_OF_DIGITS 6
#define MAX_UNIT_LENGTH  4
#define MAX_READING_SIZE                                                       \
  (NUMBER_OF_DIGITS + 1  /* overflow indicator */                              \
   + 1                   /* decimal point */                                   \
   + MAX_UNIT_LENGTH + 2 /* line ending */                                     \
   + 1 /* terminator */)

#define READING_A 1U
#define READING_B 2U
#define READING_C 4U
#define READING_D 8U

enum unit { ms, us, MHz, kHz, NoUnit };

static const char unit_texts_[5][MAX_UNIT_LENGTH] = {"ms", "us", "Mhz", "kHz",
                                                     ""};

static char *print_unit(char *const begin, const char *end,
                        const enum unit value) {
  return print_str(begin, end, unit_texts_[value]);
}

static enum unit determine_unit(const bool nml, const bool rng_2,
                                const bool has_decimal_point) {
  if (!has_decimal_point) {
    return NoUnit;
  }
  const unsigned bcd = (rng_2 ? 2U : 0U) | (nml ? 1U : 0U);
  return bcd;
}

static char *print_reading(char buf[static MAX_READING_SIZE],
                           const uint32_t reading,
                           const int decimal_point_digit, const bool overflow,
                           const enum unit unit) {
  buf[NUMBER_OF_DIGITS + 1 - decimal_point_digit] = '.';
  buf[0] = overflow ? '>' : ' ';

  const int num_chars = NUMBER_OF_DIGITS + (decimal_point_digit != 0);
  int i = 0;
  for (; i < num_chars; ++i) {
    buf[1 + i] = DIGIT(reading, num_chars - i);
  }
  return print_str(print_unit(&buf[i], &buf[MAX_READING_SIZE], unit),
                   &buf[MAX_READING_SIZE], "\r\n");
}
