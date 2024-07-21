// Decode logic for the Fluke 1900A DOU.

#include "dou.c"

#define NUMBER_OF_DIGITS 6
#define MAX_UNIT_LENGTH  4
#define MAX_READING_SIZE                                                       \
  (NUMBER_OF_DIGITS + 1  /* overflow indicator */                              \
   + 1                   /* decimal point */                                   \
   + MAX_UNIT_LENGTH + 2 /* line ending */                                     \
   + 1 /* terminator */)

#define INPUT_A                 (0x0001U)
#define INPUT_B                 (0x0002U)
#define INPUT_C                 (0x0004U)
#define INPUT_D                 (0x0008U)
#define INPUT_AS6               (0x0010U)
#define INPUT_AS5               (0x0020U)
#define INPUT_AS4               (0x0040U)
#define INPUT_AS3               (0x0080U)
#define INPUT_AS2               (0x0100U)
#define INPUT_AS1               (0x0200U)
#define INPUT_RNG2              (0x0400U)
#define INPUT_NML               (0x0800U)
#define INPUT_OVFL              (0x1000U)
#define INPUT_nMUP              (0x2000U)
#define INPUT_DS                (0x8000U)

#define DCBA(input)             ((input) & (INPUT_A | INPUT_B | INPUT_C | INPUT_D))
#define IS_DECIMAL_POINT(input) ((input) & INPUT_DS)

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

struct decoder_state {
  // The reading fits 32 bits: 6 strobes * 4 bit BCD digit + 4 bit decimal
  // point.
  u32 reading;
  int next_digit;
  int decimal_point_digit;
};

struct decoder_state decode(const struct decoder_state state,
                            const unsigned input) {
  switch (state.next_digit) {
  default:
    if ((input & INPUT_nMUP) == 0U) {
      return (struct decoder_state){0U, 1, 0};
    }
    return state;
  case 1:
    if ((input & INPUT_nMUP) != 0U) {
      return (struct decoder_state){0U, 0, 0};
    }
    if (input & INPUT_AS6) {
      return (struct decoder_state){DCBA(input), 2,
                                    IS_DECIMAL_POINT(input) ? 6 : 0};
    }
    return state;
  // Initially, wait for the `AS_6` strobe that indicates the most significant
  // digit (MSD). This ensures that decoding starts with the first complete
  // block of digits (MSD to LSD) while `nMUP` is low.
  //  For each strobe, the corresponding digit is captured and appended to the
  // reading. If the decimal strobe is asserted during a digit strobe, the
  // decimal point is appended to the reading.
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
    if ((input & INPUT_nMUP) != 0U) {
      return (struct decoder_state){0U, 0, 0};
    }
    if (input & (INPUT_AS6 << (state.next_digit - 1))) {
      // TODO put decimal point into the reading
      //  reading = (reading << 4U) | DECIMAL_POINT_BCD;
      return (struct decoder_state){
          (state.reading << 4U) | DCBA(input), state.next_digit + 1,
          IS_DECIMAL_POINT(input) ? state.next_digit : 0};
    }
    return state;
  }
}

static char *print_reading(char buf[static MAX_READING_SIZE], const u32 reading,
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
