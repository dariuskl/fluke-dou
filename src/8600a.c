//

#define NUMBER_OF_DIGITS 5 // 4½
#define MAX_READING_SIZE ()

#define INPUT_Z            (0x0001U) // BCD 1 ╮
#define INPUT_Y            (0x0002U) // BCD 2 ├ digit
#define INPUT_X            (0x0004U) // BCD 4 │
#define INPUT_W            (0x0008U) // BCD 8 ╯
#define INPUT_a            (0x0000U) // ╮
#define INPUT_b            (0x0000U) // ├ range code
#define INPUT_c            (0x0000U) // ╯

enum range {
  RANGE_200Ohm = 1,
  RANGE_2k = 2,
  RANGE_20k = 3,
  RANGE_200k = 4,
  RANGE_2000k = 5,
  RANGE_20MOhm = 6
};
