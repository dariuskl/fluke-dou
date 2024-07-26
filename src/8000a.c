// The 8000A DOU is comparatively bare-bones, having no range indication
// available and thus neither decimal point nor unit.

// (All durations given in ms.)
//                              ⬐ 0 to 12.8    vvv this info from SM, but WTF?!
//        ├──←  100 →──┼← 50 →┼←→┼────────────← 0.8 to 12.8 →────────────┤
//        ┍━━━━━━━━━━━━┑      ┍━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// nT ━━━━┙            ┕━━━━━━┙  ┊                                       ┊
//                               ┍━━━━━━┑                                ┊
// S1 ━━━━━━━━━━━━━━━━━━━━━━━━━━━┙      ┕━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//                                                             ┍━━━━━━┑  ┊
// S4 ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┙      ┕━━━━━━━━
//    ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┑  ┍━━━━━━┑  ┍━━━━━━┑  ┍━━━━━━┑  ┍━━━━━
//  S                                   ┕━━┙      ┕━━┙      ┕━━┙      ┕━━┙
//
// For some reason, the strobes do not come in order, but instead
// S1 -> S3 -> S2 -> S4. Also, there is no guarantee, which strobe comes first
// after a rising edge of nT.
// In pathological situations when the display flashes to indicate overload,
// there might not be any strobes while nT is high.

#include "dou.c"

#define NUMBER_OF_DIGITS 4 // 3½
#define MAX_READING_SIZE                                                       \
  (NUMBER_OF_DIGITS + 1 /* overload indicator */ +                             \
   1 /* polarity indicator */ + 2 /* line ending */ + 1 /* terminator */)

#define INPUT_Z            (0x0001U) // BCD 1
#define INPUT_Y            (0x0002U) // BCD 2
#define INPUT_X            (0x0004U) // BCD 4
#define INPUT_W            (0x0008U) // BCD 8
#define INPUT_T            (0x0010U)
#define INPUT_S            (0x0020U)
#define INPUT_S1           (0x0040U)
#define INPUT_S4           (0x0080U)

#define ZYXW(input)        ((input) & (INPUT_Z | INPUT_Y | INPUT_X | INPUT_W))
#define IS_OVERLOAD(input) (((input) & INPUT_W) != 0U)
#define IS_POSITIVE(input) (((input) & INPUT_Y) != 0U)

struct decoder_state {
  // The reading fits into 16 bits: 4 strobes * 4 bit BCD digit.
  unsigned reading;
  int next_digit;
};

static struct decoder_state decode(const struct decoder_state state,
                                   const unsigned input) {
  switch (state.next_digit) {
  default:
    if ((input & INPUT_T) == 0U) {
      return (struct decoder_state){0U, 1};
    }
    return state;
  case 1:
    if ((input & INPUT_T) != 0U) {
      return (struct decoder_state){0U, 0};
    }
    if ((input & INPUT_S) != 0U && (input & INPUT_S1) != 0U) {
      return (struct decoder_state){ZYXW(input), 2};
    }
    return state;
  case 2:
  case 3:
    if ((input & INPUT_T) != 0U) {
      return (struct decoder_state){0U, 0};
    }
    if ((input & INPUT_S) != 0U) {
      if ((input & INPUT_S4) != 0U) {
        return (struct decoder_state){0U, 1};
      }
      return (struct decoder_state){(state.reading << 4U) | ZYXW(input),
                                    state.next_digit + 1};
    }
    return state;
  case 4:
    if ((input & INPUT_T) != 0U) {
      return (struct decoder_state){0U, 0};
    }
    if ((input & INPUT_S) != 0U && (input & INPUT_S4) != 0U) {
      return (struct decoder_state){(state.reading << 4U) | ZYXW(input),
                                    state.next_digit + 1};
    }
    return state;
  }
}

static char *print_reading(char buf[static MAX_READING_SIZE],
                           const unsigned reading) {
  const unsigned msd = DIGIT(reading, 3);
  buf[0] = IS_OVERLOAD(msd) ? '>' : ' ';
  buf[1] = IS_POSITIVE(msd) ? '+' : '-';
  buf[2] = (msd & INPUT_Z) ? '1' : ' ';
  // digit 3 & 4 are swapped, because the strobes appear out of order
  buf[3] = bcd2digit(DIGIT(reading, 1));
  buf[4] = bcd2digit(DIGIT(reading, 2));
  buf[5] = bcd2digit(DIGIT(reading, 0));
  buf[6] = '\r';
  buf[7] = '\n';
  buf[8] = '\0';
  return &buf[8];
}
