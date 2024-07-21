// MSP430G2452-based firmware for the 1900A DOU.

#include "1900a.c"
#include "msp430/g2452.c"

// Masks for the I/O ports.
enum port1 {     // pin  | function
  OUT_B = 0x01U, // P1.0 | BCD 2
  AS_1 = 0x02U,  // P1.1 | LSD strobe
  Tx = 0x04U,    // P1.2 | serial data out
  RNG_2 = 0x08U, // P1.3 | range 2
  NML = 0x10U,   // P1.4 |
  OVFL = 0x20U,  // P1.5 | overflow indication
  AS_3 = 0x40U,  // P1.6 | 4SD strobe
  AS_2 = 0x80U,  // P1.7 | 5SD strobe
};
enum port2 {     // pin  | function
  nMUP = 0x01U,  // P2.0 | memory update
  OUT_C = 0x02U, // P2.1 | BCD 4
  OUT_D = 0x04U, // P2.2 | BCD 8
  AS_6 = 0x08U,  // P2.3 | MSD strobe
  AS_5 = 0x10U,  // P2.4 | 2SD strobe
  AS_4 = 0x20U,  // P2.5 | 3SD strobe
  OUT_A = 0x40U, // P2.6 | BCD 1
  DS = 0x80U     // P2.7 | decimal point strobe
};

static unsigned capture_input(void) {
  const byte port1 = P1IN;
  const byte port2 = P2IN;
  return (port2 & OUT_A ? INPUT_A : 0U) | (port1 & OUT_B ? INPUT_B : 0U) |
         (port2 & OUT_C ? INPUT_C : 0U) | (port2 & OUT_D ? INPUT_D : 0U) |
         (port2 & AS_6 ? INPUT_AS6 : 0U) | (port2 & AS_5 ? INPUT_AS5 : 0U) |
         (port2 & AS_4 ? INPUT_AS4 : 0U) | (port1 & AS_3 ? INPUT_AS3 : 0U) |
         (port1 & AS_2 ? INPUT_AS2 : 0U) | (port1 & AS_1 ? INPUT_AS1 : 0U) |
         (port1 & RNG_2 ? INPUT_RNG2 : 0U) | (port1 & NML ? INPUT_NML : 0U) |
         (port1 & OVFL ? INPUT_OVFL : 0U) | (port1 & nMUP ? INPUT_nMUP : 0U) |
         (port1 & DS ? INPUT_DS : 0U);
}

static void send_serial(const char msg[static MAX_READING_SIZE]);

int main(void) {
  WDTCTL = WDT_UNLOCK | WDT_HOLD;

  // Clear P2SEL reasonably early, because excess current will flow from
  // the oscillator driver output at P2.7.
  P2SEL = 0U;

  // The higher the frequency, the better we can process the signals from the
  // device. However, the low cost chips such as the G2231 don't have factory
  // calibration for 16 MHz. So if one of those is to be used, the constants
  // have to be determined by hand. The `msp430/info_util.c` can be used to
  // store the constants to the info memory.
  BCSCTL1 = CAL_BC1_16MHz;
  DCOCTL = CAL_DCO_16MHz;
#define SMCLK_FREQUENCY (16000000UL)
  BCSCTL3 = 0x24U; // ACLK = VLOCLK

  TACTL = TACTL_SMCLK; // use SMCLK for best resolution of serial baudrate

  P1OUT = Tx;
  P1DIR = Tx;
  P1IES = PxIES_RISING_EDGE(AS_3 | AS_2 | AS_1);
  P1IFG = 0U;
  P1SEL = 0U;

  P2DIR = 0U;
  P2IES = PxIES_FALLING_EDGE(nMUP) | PxIES_RISING_EDGE(AS_6 | AS_5 | AS_4);
  P2IFG = 0U;

  enable_interrupts();

  for (;;) {
    P1IE = AS_3 | AS_2 | AS_1;
    P2IE = AS_6 | AS_5 | AS_4 | nMUP;
    struct decoder_state state = {0U, 0, 0};
    for (; state.next_digit <= NUMBER_OF_DIGITS;
         state = decode(state, capture_input())) {
      // The watchdog timer will reset the device, if no nMUP signal has been
      // detected after expiration of the longest gate time of 10 seconds.
      // TODO set up watchdog
      go_to_sleep();
    }
    P1IE = 0U;
    P2IE = 0U;

    // Once the least significant digit has been captured, the overflow status
    // and range signals are evaluated and the reading is complete.
    const byte port1 = P1IN;
    bool overflow = port1 & OVFL;
    enum unit unit = determine_unit(port1 & NML, port1 & RNG_2,
                                    state.decimal_point_digit != 0);

    char text[MAX_READING_SIZE];
    print_reading(text, state.reading, state.decimal_point_digit, overflow,
                  unit);

    send_serial(text);

    // TODO Only complete readings are returned. This prevents erroneous
    //  readings, which can occur due to glitches that appear on the bus when
    //  actuating front panel switches.
    // The decoder allows multiple passes (MSD..LSD) and always updates the
    // reading with the digits from the latest pass.
  }
}

static void send_serial(const char msg[static MAX_READING_SIZE]) {
  // start timer for serial data clock
  TACCR0 = (SMCLK_FREQUENCY / SERIAL_BAUD_RATE) - 1;
  TACTL_START(TACTL_UP);

  // configure serial output
  P1SEL |= Tx; // USI on P1.6
  USICCTL = USI_TACCR0;
  USICTL = USI_PE6 | USI_LSB | USI_MASTER | USI_OE;
  // spurious interrupt can be triggered, if USIIE is set in reset
  while (USICTL & USI_RESET) {
  }
  USICTL |= USI_IE;

  for (int i = 0; i < MAX_READING_SIZE && msg[i] != '\0'; ++i) {
    //       make space for the start bit --vv
    unsigned character = STOP_BIT | ((unsigned)msg[i] << 1U);
    //         extra start & stop bits --v
    for (int bit = 0; bit < SERIAL_DATA_BITS + 2; ++bit) {
      go_to_sleep();
      P1OUT = (byte)((unsigned)(P1OUT & ~Tx) | (character & 1U ? Tx : 0U));
      character >>= 1U;
    }
    go_to_sleep();
    P1OUT = P1OUT | Tx;
  }

  TACTL_STOP();
}

__attribute__((interrupt)) void on_strobe() {
  P1IFG = 0U;
  P2IFG = 0U;
  stay_awake();
}

__attribute__((interrupt)) void on_timer() {
  stay_awake();
}

__attribute__((used, section(".vectors"))) static const struct vtable vt = {
    .reset = on_reset,
    .port1 = on_strobe,
    .port2 = on_strobe,
    .timer0_a3 = on_timer};
