/* Blinking Flashlight Code
 version 3
 
 Chance Fleeting (crf37) 12/11/18

 References:
 While this is my own work, this code was developed through a process referencing
 online resources and open discussion with peers. Such peers include Gregory Goldman
 (ghg3) and Jason Fischell (jnf22). Sources include the following:
 [1] http://ww1.microchip.com/downloads/en/devicedoc/atmel-2586-avr-8-bit-microcontroller-attiny25-attiny45-attiny85_datasheet.pdf
 [2] https://www.nongnu.org/avr-libc/user-manual/io_8h_source.html
 [3] https://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html
 [4] https://www.nongnu.org/avr-libc/user-manual/group__avr__sleep.html
 [5] https://playground.arduino.cc/Main/AVR
 [6] https://thewanderingengineer.com/2014/08/11/pin-change-interrupts-on-attiny85/
*/
// Dependancies
#include <avr/io.h>                                         // AVR Interface with the hardware [2]
#include <avr/interrupt.h>                                   // AVR Interrupt library [3]
#include <avr/sleep.h>                                      // AVR Sleep-mode library [4]

// Definitions [4]

int OPIN = 4;
int IPIN = 3;
bool Flash = LOW;
volatile bool swtch = HIGH;
const unsigned long T = 500; // Flash Period (ms)
unsigned long to = 0; //current time
unsigned long t1 = 0; // previous time
int State = 0;
/*
   0: off
   1: low
   2: med
   3: high
   4: 2Hz blink at high intensity
*/

void setup() {
  // Disable interrupt
  cli();
  // Register Initialization [1] p 64. I prefer Hex over Bin.
  PORTB = (~((1 << OPIN) | (1 << IPIN)));                   // Internal Pull-up is disabled for the nonfloating I/O pins (IPIN and OPIN)
  DDRB  = (1 << OPIN);                                      // Set OPIN to output (this is the LED Driver)
  PINB = 0x00;                                              // From Greg's code. Unnecessary but no harm.
  // interrupt Initialization [1] p 49
  GIMSK = 0x20;                                                // Global staging: Turns on pin interrupt capability [1] p200
  PCMSK = (1 << IPIN);                                      // Set IPIN to input-- interrupt (this is debounced button)
  // Shut Down ADC (Greg's Suggestion to save power)
  ADCSRA &= ~_BV(ADEN);                                      //if ADEN is on, turn off.
  // Enable interrupt
  sei();
}

void loop() {
  if (swtch) {
    SwitchStates();                                     //  If triggered, itterate forward in states
  }
  analogWrite(OPIN, Flash?((4<<(State<<1))-1):0);       // if Flash boolean, use pwm == 4^(1+State)-1, else off
  if (State == 4) {
    FlashMode();                                        //Flashmode activated
  }
  if (State == 0) {
    SleepMode();                                        //Sleepmode activated
  }
}

/* SwitchStates():
 *  Activates Flash Mode (activated by swtch)
 *  5-step counter. Defaults Flash and resets swtch.
 */
void SwitchStates(){
    State = (++State)%5;
    Flash = !(State == 0);
    swtch &= LOW;
}

/* FlashMode():
 *  Activates Flash Mode (activated State == 4)
 *  Toggle alternator using internal clock.
 */
void FlashMode(){
      to = micros();
      if (to - t1 > T*500) {
        Flash = !Flash;
        t1 = to;
      }
}

/* SleepMode():
 *  Activates Sleep Mode (activated State == 0)
 *  Saves Power
 */
void SleepMode(){
      set_sleep_mode(SLEEP_MODE_PWR_DOWN);
      sleep_enable();
      sleep_cpu();
}

/* Interrupt:
 *  Switches Boolean, which allows for change in state
 */
ISR(PCINT0_vect) {                                            // Respond on pin change (Rise | Fall)
  //Pin Change Interrupt Request 0 [3] No Nesting interrupts
      swtch |= !digitalRead(IPIN);              // ~High Respond -> Falling Edge; Turn on swtch state
}
