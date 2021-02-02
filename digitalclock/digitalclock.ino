#include <TM1637Display.h>

const int sevseg_clock = 7, sevseg_data = 8;
uint8_t digits[] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f }; // decimal to 7 segment

TM1637Display sevseg(sevseg_clock, sevseg_data);

const int button0 = 6, button1 = 5, button2 = 4;

int button0_clicked = 0;

void setup()
{
  Serial.begin(9600);
  setupInterrupt();
  pinMode(sevseg_clock, OUTPUT); // pin 7 is 7 seg clock
  pinMode(sevseg_data, OUTPUT); // pin 8 is 7 seg data
  sevseg.clear();
  sevseg.setBrightness(7);

  pinMode(button0, INPUT);
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  Serial.println("setup done");
}

byte tcnt2;
// initial time
unsigned long int setMinutes = 52;
unsigned long int setHours = 19;
unsigned long time = (setMinutes * 60 * 1000) + (setHours * 3600 *1000); // in miliseconds
  
void setupInterrupt()
{
  // disable interrupts
  TIMSK2 &= ~(1<<TOIE2);
  TIMSK2 &= ~(1<<OCIE2A);
  //prescaler source clock set to internal Atmega clock (asynch mode)
  ASSR &= ~(1<<AS2);
  //this sets the timer to increment the counter until overflow
  TCCR2A &= ~((1<<WGM21) | (1<<WGM20));
  TCCR2B &= ~(1<<WGM22);
  // prescaler set to 1024
  TCCR2B |= (1<<CS22)  | (1<<CS20); 
  TCCR2B &= ~(1<<CS21);
  // start from 131 (125 ticks to overflow)
  tcnt2 = 131;
  TCNT2 = tcnt2;
  // enable overflow interrupt
  TIMSK2 |= (1<<TOIE2);
}

ISR(TIMER2_OVF_vect) {
  TCNT2 = tcnt2; // reset
  time++; // tick
  time = time % 86400000; // miliseconds in a day
}

void loop()
{
  unsigned long t = (unsigned long)(time/1000);
  uint8_t minutes = (byte)((t / 60) % 60);
  uint8_t hours = (byte)((t / 3600) % 24);
  uint8_t seconds = (byte)(t % 60);
  uint8_t dec = (byte) (hours * 100 + minutes);
  sevseg.showNumberDecEx(minutes, 0, true, 2, 2);
  sevseg.showNumberDecEx(hours, (0x80 >> seconds % 2), true, 2, 0);
//  bool setting_button_clicked = buttons_check();
  if (buttons_check()) button_set_time(hours, minutes);
  delay(100);
}

bool buttons_check() {
  if (digitalRead(button0) == HIGH && !button0_clicked) {
      button0_clicked = 1;
      return true;
  } else if (digitalRead(button0) == LOW) {
      button0_clicked = 0;
      return false;
  } else return false;
}

void button_set_time(uint8_t hours, uint8_t minutes) {
  uint8_t values[] = { hours, minutes }; // user input value
  uint8_t pointer = 0; // 0 for hours, 1 for minutes
  uint8_t zero[] = { 0, 0x80 }; // for blank 7 segment
  uint8_t count = 0; // for blinking purposes
  while(1) {
    // blink every 900 ms
    if (count % 3 == 0) { // every 450 ms
      if (count % 6 == 0)
        sevseg.setSegments(zero, 2, pointer*2); // clear digit
      else
        sevseg.showNumberDecEx(values[pointer], 0x40, true, 2, pointer*2); // show digit
    }

    // increment time
    if (digitalRead(button1) == HIGH) {
      // for hours
      if (pointer == 0) {
        if (values[0] < 23) values[0]++;
        else values[0] = 0;
      }
      // for minutes
      else if (pointer == 1) {
        if (values[1] < 59) values[1]++;
        else values[1] = 0;
      }
      // show number
      sevseg.showNumberDecEx(values[pointer], 0x40, true, 2, pointer*2);
    }

    // decrement time
    if (digitalRead(button2) == HIGH) {
      // for hours
      if (pointer == 0) {
        if (values[0] > 0) values[0]--;
        else values[0] = 23;
      }
      // for minutes
      else if (pointer == 1) {
        if (values[1] > 0) values[1]--;
        else values[1] = 59;
      }
      // show number
      sevseg.showNumberDecEx(values[pointer], 0x40, true, 2, pointer*2);
    }
    
    count ++;
    delay(150); // loop delay

    // setting button clicked
    if (digitalRead(button0) == HIGH) {
        sevseg.showNumberDecEx(values[pointer], 0x40, true, 2, pointer*2); // stop blinking
        if (pointer < 1) pointer++; // change pointer
        else {
          change_time(values); // save time
          return;
        }
    }
  };
}

void change_time(const uint8_t values[]) {
  time = ( (unsigned long int) values[1] * 60 * 1000) + ( (unsigned long int) values[0] * 3600 *1000);
}
