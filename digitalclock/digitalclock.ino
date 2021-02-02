
 
 // set current time by editing the values at line 16 and 17
const int sevseg_clock = 7;
const int sevseg_data = 8;
uint8_t digits[] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f }; // decimal to 7 segment

const int button0 = 3;
int button0_clicked = 0;

void setup()
{
  Serial.begin(9600);
  setupInterrupt();
  pinMode(sevseg_clock, OUTPUT); // pin 7 is 7 seg clock
  pinMode(sevseg_data, OUTPUT); // pin 8 is 7 seg data
  start();
  writeValue(0x8c); // set 7 seg brightness
  stop();
  sevseg_write(0x00, 0x00, 0x00, 0x00); // clear 7 seg

  pinMode(button0, INPUT);
}

byte tcnt2;
// initial time
unsigned long int setMinutes = 06;
unsigned long int setHours = 17;
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
  sevseg_write(digits[hours / 10], digits[hours % 10] | ((seconds & 0x01) << 7), digits[minutes / 10], digits[minutes % 10]);
  if (digitalRead(button0) == HIGH && !button0_clicked) {
      Serial.write("clicked\n");
      button0_clicked = 1;
  } else if (digitalRead(button0) == LOW) {
      button0_clicked = 0;
  }
}

void sevseg_write(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth)
{
  start();
  writeValue(0x40);
  stop();
  start();
  writeValue(0xc0);
  writeValue(first);
  writeValue(second);
  writeValue(third);
  writeValue(fourth);
  stop();
}

void start(void)
{
  digitalWrite(sevseg_clock,HIGH);
  digitalWrite(sevseg_data,HIGH);
  delayMicroseconds(5);
  digitalWrite(sevseg_data,LOW);
  digitalWrite(sevseg_clock,LOW);
  delayMicroseconds(5);
}

void stop(void)
{
  digitalWrite(sevseg_clock,LOW);
  digitalWrite(sevseg_data,LOW);
  delayMicroseconds(5);

  digitalWrite(sevseg_clock,HIGH);
  digitalWrite(sevseg_data,HIGH);
  delayMicroseconds(5);
}

bool writeValue(uint8_t value)
{
  for(uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(sevseg_clock, LOW);
    delayMicroseconds(5);
    digitalWrite(sevseg_data, (value & (1 << i)) >> i);
    delayMicroseconds(5);
    digitalWrite(sevseg_clock, HIGH);
    delayMicroseconds(5);
  }
  digitalWrite(sevseg_clock,LOW);
  delayMicroseconds(5);
  pinMode(sevseg_data,INPUT);
  digitalWrite(sevseg_clock,HIGH);
  delayMicroseconds(5);
  bool ack = digitalRead(sevseg_data) == 0;
  pinMode(sevseg_data,OUTPUT);
  return ack;
}
