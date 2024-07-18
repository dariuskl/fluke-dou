// The 8000A DOU is comparatively bare-bones, having no range indication
// available and thus neither decimal point nor unit.

// (All durations given in ms.)
//                              ⬐ 0 to 12.8
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
  buf[0] = IS_OVERLOAD(reading >> 12U) ? '>' : ' ';
  buf[1] = IS_POSITIVE(reading >> 12U) ? '+' : '-';
  buf[2] = (reading >> 12U & READING_Z) != 0U ? '1' : ' ';
  buf[3] = '0' + (reading >> 8U);
  buf[4] = '0' + (reading >> 4U);
  buf[5] = '0' + (reading >> 0U);
  buf[6] = '\r';
  buf[7] = '\n';
  buf[8] = '\0';
  return &buf[8];
}
