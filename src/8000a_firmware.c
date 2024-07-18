// MSP430G2452-based firmware for the 8000A DOU.

#include "8000a.c"
#include "msp430/g2231.c"

// Masks for the I/O ports with P1 in the lower eight bits and P2 in the upper
// eight bits.
enum signal {
  nZERO = 0x0001U,
  Y = 0x0001U, // BCD 2
  X = 0x0002U, // BCD 4
  W = 0x0004U, // BCD 8
  Z = 0x0008U, // BCD 1
  nT = 0x0010U,
  S1 = 0x0020U,
  Tx = 0x0040U, // P1.6, SDO
  S = 0x0080U,
  S4 = 0x4000U, // P2.6
};

static unsigned wxyz(const unsigned input) {
  return (input & W ? READING_W : 0U) | (input & X ? READING_X : 0U) |
         (input & Y ? READING_Y : 0U) | (input & Z ? READING_Z : 0U);
}

static unsigned wait_for(enum signal signal) {
  P2IE = signal >> 8U;
  go_to_sleep();
  P2IE = 0U;
  return P1IN | P2IN << 8U;
}

static void send_serial(const char msg[static MAX_READING_SIZE]) {
  // start timer for serial data clock
  TACCR0 = ((SMCLK_FREQUENCY / SERIAL_BAUD_RATE) / 2) - 1;
  TACCTL0 = TACCTL0_OUTMODE_TOGGLE;
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
#define STOP_BIT (1U << ((SERIAL_DATA_BITS) + 1))
    USISR = STOP_BIT | (msg[i] << 1U); // make space for the start bit
    // extra start & stop bits as we're in SPI mode --v
    USICNT = USI_16BIT | (SERIAL_DATA_BITS + 2);
    go_to_sleep();
  }

  TACTL_STOP();
}

int main(void) {
  WDTCTL = WDT_UNLOCK | WDT_HOLD;

  // Clear P2SEL reasonably early, because excess current will flow from
  // the oscillator driver output at P2.7.
  P2SEL = 0U;

  BCSCTL1 = 0x8f;  // TODO CAL_BC1_16MHz;
  DCOCTL = 0x8a;   // TODO CAL_DCO_16MHz;
  BCSCTL3 = 0x24U; // ACLK = VLOCLK

#define SMCLK_FREQUENCY (16000000UL)

  TACTL = TACTL_SMCLK; // ues SMCLK for best resolution of serial baudrate

  P1OUT = Tx; // make sure to pull Tx high asap
  P1DIR = Tx;
  P1IES = 0U;
  P1IE = 0U;
  P1SEL = 0U; // USI will be configured to P1.6 later
  P1REN = 0U;

  P2DIR = 0U;
  P2IES = 0U; // rising edges
  P2IE = 0U;
  P2REN = 0U;

  enable_interrupts();

  for (;;) {
    // The watchdog timer will reset the device, if no measurement has been
    // detected for a while.
    // ACLK = VLOCLK = max. 20 kHz, according to datasheet
    // 6 measurements per seconds, typically, according to service manual
    // 20 kHz / 8192 = ~2,4 Hz
    // gives enough headroom to transmit the reading at 19200 baud
    WDTCTL = WDT_UNLOCK | WDT_CLEAR | WDT_ACLK | WDT_8192;

    unsigned input = wait_for(nT);

    if ((input & nT) == 0U) {
      continue;
    }

    input = wait_for(S1);

    unsigned reading = wxyz(input);

    do {
      input = wait_for(S);
      reading = reading << 4U | wxyz(input);
    } while ((input & S4) == 0U);

    char text[MAX_READING_SIZE];
    print_reading(text, reading);

    send_serial(text);
  }
}

__attribute__((interrupt)) void on_port2(void) {
  P2IFG = 0U;
  stay_awake();
}

__attribute__((interrupt)) void on_usi(void) {
  USICTL &= ~USI_IFG;
  stay_awake();
}

__attribute__((used, section(".vectors"))) static const struct vtable vt = {
    .reset = on_reset, .port2 = on_port2, .usi = on_usi};
