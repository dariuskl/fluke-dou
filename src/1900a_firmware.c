// MSP430G2452-based firmware for the 1900A DOU.

#include "1900a.c"
#include "msp430/g2452.c"

// Masks for the I/O ports with P1 in the lower eight bits and P2 in the upper
// eight bits.
enum signal {
  OUT_B = 0x0001U, // BCD 2
  AS_1 = 0x0002U,  // LSD strobe
  Tx = 0x0004U,
  RNG_2 = 0x0008U,
  NML = 0x0010U,
  OVFL = 0x0020U,
  AS_3 = 0x0040U, // 4SD strobe
  AS_2 = 0x0080U, // 5SD strobe
  nMUP = 0x0100U,
  OUT_C = 0x0200U, // BCD 4
  OUT_D = 0x0400U, // BCD 8
  AS_6 = 0x0800U,  // MSD strobe
  AS_5 = 0x1000U,  // 2SD strobe
  AS_4 = 0x2000U,  // 3SD strobe
  OUT_A = 0x4000U, // BCD 1
  DS = 0x8000U
};

static unsigned dcba(const unsigned input) {
  return (input & OUT_D ? READING_D : 0U) | (input & OUT_C ? READING_C : 0U) |
         (input & OUT_B ? READING_B : 0U) | (input & OUT_A ? READING_A : 0U);
}

static unsigned wait_for(enum signal signal) {
  P2IE = signal >> 8U;
  go_to_sleep();
  P2IE = 0U;
  return P1IN | P2IN << 8U;
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
  P1IES = 0U;
  P1IE = 0U;
  P1SEL = 0U;
  P1REN = 0U;

  P2DIR = 0U;
  P2IES = nMUP >> 8U; // nMUP falling edge
  P2IE = 0U;
  P2REN = 0U;

  enable_interrupts();

  for (;;) {
    // The watchdog timer will reset the device, if no nMUP signal has been
    // detected after expiration of the longest gate time of 10 seconds.
    // TODO set up watchdog

    unsigned input = wait_for(nMUP);

    if ((input & nMUP) != 0U) {
      continue;
    }

    // Initially, wait for the `AS_6` strobe that indicates the most significant
    // digit (MSD). This ensures that decoding starts with the first complete
    // block of digits (MSD to LSD) while `nMUP` is low.
    input = wait_for(AS_6);

    // The reading fits 32 bits: 6 strobes * 4 bit BCD digit.
    uint32_t reading = dcba(input); // MSD
    int decimal_point_digit = input & DS ? 6 : 0;

    // For each strobe, the corresponding digit is captured and appended to the
    // reading. If the decimal strobe is asserted during a digit strobe, the
    // decimal point is prepended to the digit.
    static unsigned strobes[5] = {AS_1, AS_2, AS_3, AS_4, AS_5};
    for (int i = 5; i >= 0; --i) {
      input = wait_for(strobes[i]);
      reading = (reading << 4U) | dcba(input);
      if (input & DS) {
        reading = (reading << 4U) | DECIMAL_POINT_BCD;
        decimal_point_digit = i;
      }
    }

    // Once the least significant digit has been captured, the overflow status
    // and range signals are evaluated and the reading is complete.
    bool overflow = input & OVFL;
    enum unit unit =
        determine_unit(input & NML, input & RNG_2, decimal_point_digit != 0);

    char text[MAX_READING_SIZE];
    print_reading(text, reading, decimal_point_digit, overflow, unit);

    send_serial(text);

    // TODO Only complete readings are returned. This prevents erroneous
    //  readings, which can occur due to glitches that appear on the bus when
    //  actuating front panel switches.
    // The decoder allows multiple passes (MSD..LSD) and always updates the
    //  reading with the digits from the latest pass.
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
    int character = STOP_BIT | (msg[i] << 1U);
    //         extra start & stop bits --v
    for (int bit = 0; bit < SERIAL_DATA_BITS + 2; ++bit) {
      go_to_sleep();
      P1OUT = (P1OUT & ~Tx) | (character & 1U ? Tx : 0U);
      character >>= 1U;
    }
    go_to_sleep();
    P1OUT = P1OUT | Tx;
  }

  TACTL_STOP();
}

// Called on falling edge on /MUP.
__attribute__((interrupt)) void on_strobe() {
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
