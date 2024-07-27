// Tests whether the calculation of checksums is correct.

#include "8000a.c"

#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_decode(void) {
  struct decoder_state state = {0U, 0};

  state = decode(state, INPUT_T); // T is high -> meter is updating
  TEST_ASSERT_EQUAL_INT(0, state.next_digit);
  TEST_ASSERT_EQUAL_UINT(0U, state.reading);

  state = decode(state, 0U); // T is low -> display is updating
  TEST_ASSERT_EQUAL_INT(1, state.next_digit);
  TEST_ASSERT_EQUAL_UINT(0U, state.reading);

  state = decode(state, INPUT_S); // clock, but no strobe
  TEST_ASSERT_EQUAL_INT(1, state.next_digit);
  TEST_ASSERT_EQUAL_UINT(0U, state.reading);

  state = decode(state, INPUT_S | INPUT_S4); // clock with wrong strobe
  TEST_ASSERT_EQUAL_INT(1, state.next_digit);
  TEST_ASSERT_EQUAL_UINT(0U, state.reading);

  state = decode(state, INPUT_S | INPUT_S1 // clock with S1 strobe
                            | INPUT_W | INPUT_X | INPUT_Y | INPUT_Z);
  TEST_ASSERT_EQUAL_INT(2, state.next_digit);
  TEST_ASSERT_EQUAL_UINT(0xfU, state.reading);

  // a glitch on the clock leads to a second interrupt, so once again ...
  state = decode(state, INPUT_S | INPUT_S1 // clock with S1 strobe
                            | INPUT_W | INPUT_X | INPUT_Y | INPUT_Z);
  TEST_ASSERT_EQUAL_INT(2, state.next_digit);
  TEST_ASSERT_EQUAL_UINT(0xfU, state.reading);

  state = decode(state, INPUT_S // clock with no strobe
                            | INPUT_X | INPUT_Y | INPUT_Z);
  TEST_ASSERT_EQUAL_INT(3, state.next_digit);
  TEST_ASSERT_EQUAL_UINT(0xf7U, state.reading);

  state = decode(state, INPUT_S // clock with no strobe
                            | INPUT_Y | INPUT_Z);
  TEST_ASSERT_EQUAL_INT(4, state.next_digit);
  TEST_ASSERT_EQUAL_UINT(0xf73U, state.reading);

  state = decode(state, INPUT_S | INPUT_S4 // clock with S4 strobe
                            | INPUT_Z);
  TEST_ASSERT_EQUAL_INT(5, state.next_digit);
  TEST_ASSERT_EQUAL_UINT(0xf731U, state.reading);
}

void test_print_reading(void) {
  char buffer[MAX_READING_SIZE];
  print_reading(buffer, 0x0000U);
  TEST_ASSERT_EQUAL_STRING(" - 000\r\n", buffer);

  print_reading(buffer, 0x2000U);
  TEST_ASSERT_EQUAL_STRING(" + 000\r\n", buffer);

  print_reading(buffer, 0x6000U);
  TEST_ASSERT_EQUAL_STRING(" + 000\r\n", buffer);

  print_reading(buffer, 0x7000U);
  TEST_ASSERT_EQUAL_STRING(" +1000\r\n", buffer);

  print_reading(buffer, 0x6001U);
  TEST_ASSERT_EQUAL_STRING(" + 001\r\n", buffer);

  print_reading(buffer, 0x6010U);
  TEST_ASSERT_EQUAL_STRING(" + 100\r\n", buffer);

  print_reading(buffer, 0x6100U);
  TEST_ASSERT_EQUAL_STRING(" + 010\r\n", buffer);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_decode);
  RUN_TEST(test_print_reading);
  return UNITY_END();
}
