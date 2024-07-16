// Decode logic for the Fluke 1900A DOU.

#include "dou.c"

#include <stdbool.h>

#define NUMBER_OF_DIGITS 6
#define MAX_UNIT_LENGTH  4
#define MAX_READING_SIZE                                                       \
  (NUMBER_OF_DIGITS + 1  /* overflow indicator */                              \
   + 1                   /* decimal point */                                   \
   + MAX_UNIT_LENGTH + 2 /* line ending */                                     \
   + 1 /* terminator */)

enum unit { ms, us, MHz, kHz, NoUnit };

static const char unit_texts_[5][MAX_UNIT_LENGTH] = {"ms", "us", "Mhz", "kHz",
                                                     ""};

static char *print_unit(char *buf, const int len, const enum unit value) {
  return print_str(buf, len, unit_texts_[value]);
}

// Keeps a capture of the decoded momentary bus state.
struct input_state {
  int digit_strobe;
  int out;
  bool decimal_strobe;
  bool overflow;
  bool nml;
  bool rng_2;
};

enum data_state {
  OverflowUnit,
  Digit1,
  Digit2,
  Digit3,
  Digit4,
  Digit5,
  Digit6,
  Init
};

struct decoder_state {
  enum data_state data_state;
  char reading[MAX_READING_SIZE];
  int decimal_point_digit;
  bool complete;
};

const char *decoder_reading(const struct decoder_state *state);
void decoder_transit(struct decoder_state *state,
                     const struct input_state input);
int decoder_get_index(const struct decoder_state *state,
                      const int digit_strobe);
void decoder_set_overflow(struct decoder_state *state, const bool overflow);
void decoder_set_digit(struct decoder_state *state, const int digit_strobe,
                       const int out);

__attribute__((unused)) static enum unit
fluke1900a_determine_unit(const bool nml, const bool rng_2,
                          const bool has_decimal_point) {
  if (!has_decimal_point) {
    return NoUnit;
  }
  const unsigned bcd = (rng_2 ? 2U : 0U) | (nml ? 1U : 0U);
  return bcd;
}
