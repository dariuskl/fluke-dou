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

#define READING_Z            1U
#define READING_Y            2U
#define READING_X            4U
#define READING_W            8U

#define IS_OVERLOAD(reading) (((reading) & READING_W) != 0U)
#define IS_POSITIVE(reading) (((reading) & READING_Y) != 0U)

static char *print_reading(char buf[static MAX_READING_SIZE],
                           const unsigned reading) {
  const unsigned msd = DIGIT(reading, 3);
  buf[0] = IS_OVERLOAD(msd) ? '>' : ' ';
  buf[1] = IS_POSITIVE(msd) ? '+' : '-';
  buf[2] = (msd & READING_Z) ? '1' : ' ';
  // digit 3 & 4 are swapped, because the strobes appear out of order
  buf[3] = bcd2digit(DIGIT(reading, 1));
  buf[4] = bcd2digit(DIGIT(reading, 2));
  buf[5] = bcd2digit(DIGIT(reading, 0));
  buf[6] = '\r';
  buf[7] = '\n';
  buf[8] = '\0';
  return &buf[8];
}
