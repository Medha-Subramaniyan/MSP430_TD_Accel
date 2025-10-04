// MSP430FR6989 + BOOSTXL-EDUMKII Accelerometer -> UART CSV (Lab-6 UART settings)
// UART: eUSCI_A1 @ 9600 baud, SMCLK = 1 MHz, oversampling (UCBR=6,UCBRF=8,UCBRS=0x20,UCOS16=1)
// ADC : AX=P8.4(A7), AY=P8.5(A6), AZ=P8.6(A5)  (sequence MEM0..2)

#include <msp430fr6989.h>

#define F_CPU     1000000UL

// LEDs
#define RED_LED   BIT0      // P1.0
#define GREEN_LED BIT7      // P9.7

// ---------------- UART helpers ----------------
static inline void uart_putc(char c) {
    while (!(UCA1IFG & UCTXIFG));
    UCA1TXBUF = c;
}
static void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}
// Use CRLF so each line starts at column 0
static inline void uart_crlf(void) { uart_putc('\r'); uart_putc('\n'); }

// prints unsigned int right-aligned in a 5-char field
static void uart_putu_aligned(unsigned v) {
    char b[6];
    int i = 0;
    int j;  // C89-compatible

    if (v == 0) { uart_puts("    0"); return; }

    while (v && i < 5) { b[i++] = '0' + (v % 10); v /= 10; }

    for (j = 5 - i; j > 0; j--) uart_putc(' ');  // left pad
    while (i--) uart_putc(b[i]);
}

// ---------------- Clock: DCO ~1 MHz ----------------
static void clock_to_1MHz(void) {
    CSCTL0_H = CSKEY >> 8;
    CSCTL1   = DCOFSEL_0;
    CSCTL2   = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3   = DIVA__1 | DIVS__1 | DIVM__1;
    CSCTL0_H = 0;
}

// ---------------- UART A1: 9600 @ 1 MHz ----------------
static void uart_init_9600_lab6(void) {
    P3SEL1 &= ~(BIT4 | BIT5);
    P3SEL0 |=  (BIT4 | BIT5);
    UCA1CTLW0  = UCSWRST;
    UCA1CTLW0 |= UCSSEL_2;
    UCA1BRW    = 6;
    UCA1MCTLW  = (UCBRF_8) | (0x20 << 8) | UCOS16;
    UCA1CTLW0 &= ~UCSWRST;
}

// ---------------- ADC12_B: AX(A7) AY(A6) AZ(A5) ----------------
static void adc_pins_init(void) {
    P8SEL0 |= (BIT4 | BIT5 | BIT6);
    P8SEL1 |= (BIT4 | BIT5 | BIT6);
}
static void adc_init(void) {
    ADC12CTL0  = ADC12SHT0_2 | ADC12ON;
    ADC12CTL1  = ADC12SHP | ADC12CONSEQ_1;
    ADC12CTL2  = ADC12RES_2;
    ADC12MCTL0 = ADC12INCH_7;
    ADC12MCTL1 = ADC12INCH_6;
    ADC12MCTL2 = ADC12INCH_5 | ADC12EOS;
    ADC12CTL0 |= ADC12ENC;
}

// read 3 samples (kept timeout but no prints)
static inline int adc_read3(unsigned *ax, unsigned *ay, unsigned *az) {
    ADC12CTL0 |= ADC12SC;
    unsigned tmo = 20000;
    while (!(ADC12IFGR0 & BIT2)) {
        if (--tmo == 0) return -1;
    }
    *ax = ADC12MEM0; *ay = ADC12MEM1; *az = ADC12MEM2;
    return 0;
}

// ----------------------------------- main -----------------------------------
int main(void) {
    WDTCTL  = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;

    P1DIR |= RED_LED;   P1OUT &= ~RED_LED;
    P9DIR |= GREEN_LED; P9OUT &= ~GREEN_LED;

    clock_to_1MHz();
    uart_init_9600_lab6();
    adc_pins_init();
    adc_init();

    __delay_cycles(F_CPU / 2);  // let terminal attach



    for (;;) {
        P1OUT ^= RED_LED;

        unsigned ax, ay, az;
        if (adc_read3(&ax, &ay, &az) == 0) {
            uart_putu_aligned(ax); uart_putc(','); uart_putc(' ');
            uart_putu_aligned(ay); uart_putc(','); uart_putc(' ');
            uart_putu_aligned(az);
            uart_crlf();  // <-- CRLF so next row starts at col 0
        }
        // no ADC_TIMEOUT print to keep rows aligned

        __delay_cycles(F_CPU / 25);  // ~40 Hz
    }
}
