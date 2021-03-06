// Blinking Flashlight Code
// version 1
//
// Chance Fleeting (crf37) 11/20/18
//
// References:
// While this is my own work, this code was developed through a process referencing
// online resources and open discussion with peers. Such peers include Gregory Goldman
// (ghg3) and Jason Fischell (jnf22). Sources include the following:
// [1] http://ww1.microchip.com/downloads/en/devicedoc/atmel-2586-avr-8-bit-microcontroller-attiny25-attiny45-attiny85_datasheet.pdf
// [2] https://www.nongnu.org/avr-libc/user-manual/io_8h_source.html
// [3] https://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html
// [4] https://www.nongnu.org/avr-libc/user-manual/group__avr__sleep.html
// [5] https://playground.arduino.cc/Main/AVR
// [6] https://thewanderingengineer.com/2014/08/11/pin-change-interrupts-on-attiny85/
// Dependancies
#include <avr/io.h>                                         // AVR Interface with the hardware [2]
#include <avr/interrupt.h>                                   // AVR Interrupt library [3]
#include <avr/sleep.h>                                      // AVR Sleep-mode library [4]

// Definitions [4]

#define IPIN PB3                                            // PB3 Interrupt/Input Pin <- Button
#define OPIN PB4                                            // PB4 Output Pin -> LED

// Interrupt Switch
volatile bool swtch = false;
//  Constants
const int T = 500;                                         // s Flash period
//  Variables
int state = 0;
int pwm = 0;
unsigned long t1 = 0;
unsigned long t2 = 0;
bool flash_en = false;
bool flash_alt = false;

void setup() {
  // Disable interrupt
  cli();
  // Register Initialization [1] p 64. I prefer Hex over Bin.
  PORTB = (~((1 << OPIN) | (1 << IPIN)));                   // Internal Pull-up is disabled for the nonfloating I/O pins (IPIN and OPIN)
  DDRB  = (1 << OPIN);                                      // Set OPIN to output (this is the LED Driver)
  // interrupt Initialization [1] p 49
  GIMSK = 0x20;                                                // Global staging: Turns on pin interrupt capability [1] p200
  PCMSK = (1 << IPIN);                                      // Set IPIN to input-- interrupt (this is debounced button)
  // Shut Down ADC (Greg's Suggestion to save power)
  ADCSRA &= ~_BV(ADEN);                                      //if ADEN is on, turn off.
  // Enable interrupt
  sei();
}

void loop() {
  // State Selection: 0: off; 1: low; 2: mid; 3: high; 4: flash
    if (swtch) {
        switch (state) {
      case 0:                                                  // Reset to off
        pwm = 0;
        flash_en = false;
        flash_alt = true;
        GoToSleep(true);
        break;
      case 4:                                                  // End case: flash
        flash_en = true;
        break;
      default:                                                 // Default: Get brighter
        if (state == 1) {
          GoToSleep(false);
        }
        pwm += 85;
        break;
    }
    state = (state++) % 5;                                   // Increment to next state
  }
  if (flash_en) {                                            // if flash is enabled, toggle flash alternator 2Hz 50% duty
      // Monitor and Toggle States
    t1 = millis();
    if ((t1 - t2) > (T / 2)) {
      flash_alt = !flash_alt;
      t2 = t1;
    }
  }
  analogWrite(OPIN, (flash_alt ? 0 : pwm));                //flash at pwm if flash_alt is off; if else off
}

void GoToSleep(bool Now) {
  // Sleep Mode control. Less power consumption in Sleep mode.
  if (Now) { // Go to sleep
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_cpu();
  }
  else { // Wake up
    sleep_disable();
  }
}

ISR(PCINT0_vect) {                                            // Respond on pin change (Rise | Fall)
  //Pin Change Interrupt Request 0 [3] No Nesting interrupts

  if (!digitalRead(IPIN)) {                                    // ~High Respond -> Falling Edge
    swtch |= true;                                            // Turn on swtch state
  }
}
