/*
 * FinalFlashlightCode.ino
 * Greg Goldman
 * BME 590: Medical Instrumentation Design
 * Fall, 2018
 * 
 * This code operates a flashlight on an ATTiny85 processor at a clock speed of 8 MHz
 * It is capable of 5 modes: low, medium, high, flash at high, and off
 * When off, the processor is put to sleep, reducing its current draw from a few mA
 * to less than 50 uA.
 * 
 * It uses interrupts to record button inputs, and it has rudimentary software debounce 
 * built in.
 * 
 * A great challenge with the debounce was that the timer resets after going to sleep, so
 * it was hard to tell the time since the last press if the device had slept. This was solved
 * by implementing a delay of the bounce wait time before going to sleeep. 
 * 
 * All time variables are in microseconds because micros() works in the ISR, but millis doesn't
 * 
 * In an unsigned long, there are 2^32 bits, so I can count to about 4.295 billion microseconds,
 * or about 4295 seconds, which is about 2.98 days before the timer count can roll over
 * 
 * Related content can be found at https://github.com/GregGoldman/BlinkLight
 */

//Includes
#include <avr/io.h> 
#include <avr/interrupt.h> //https://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html
#include <avr/sleep.h> //https://www.nongnu.org/avr-libc/user-manual/group__avr__sleep.html
//https://bigdanzblog.wordpress.com/2014/08/10/attiny85-wake-from-sleep-on-pin-state-change-code-example/
// how to put attiny to sleep

int OPIN = 4;
int IPIN = 3;
bool Flash = LOW;
volatile bool swtch = HIGH;
unsigned long now = 0; //current time
unsigned long lastFlashTime = 0; // previous time
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
    SwitchStates();
  }
  analogWrite(OPIN, Flash?((4<<(State<<1))-1):0);
  if (State == 4) {
    FlashMode();
  }
  if (State == 0) {
    SleepMode();
  }
}

void SwitchStates(){
    State = (++State)%5;
    Flash = !(State == 0);
    swtch &= LOW;
}

void FlashMode(){
          now = micros();
      if (now - lastFlashTime > 250000) {
        Flash = !Flash;
        lastFlashTime = now;
      }
}

void SleepMode(){
      set_sleep_mode(SLEEP_MODE_PWR_DOWN);
      sleep_enable();
      sleep_cpu();
}

//Pin Change Interrupt Request 0 (don't look at nested interrupts)
ISR(PCINT0_vect) {
  // This will trigger on both rising and falling by default
  // need to read the state of the pin to tell if falling
      swtch |= !digitalRead(IPIN);
}
