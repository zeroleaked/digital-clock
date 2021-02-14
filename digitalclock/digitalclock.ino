#include <TM1637Display.h>
#include <ModbusMaster.h>
#include <LiquidCrystal_I2C.h>

// seven segment constants
const int sevseg_clock = 7, sevseg_data = 8;
uint8_t digits[] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f }; // decimal to 7 segment
TM1637Display sevseg(sevseg_clock, sevseg_data);

// lcd constants
LiquidCrystal_I2C lcd(0x27,16,2);

// button constants
const int button0 = 4, button1 = 5, button2 = 6; // button0 setting, button1 increment time, button2 decrement time

// time constants
String namaHari[] = {"Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu", "Minggu"};

// time initial settings
uint8_t setHari = 6, setTanggal = 31, setBulan = 12;
unsigned long int setTahun = 2021;
unsigned long int setHours = 23, setMinutes = 55;

// time variables
unsigned long time = (setMinutes * 60 * 1000) + (setHours * 3600 *1000); // in miliseconds
uint8_t hari = setHari, tanggal = setTanggal, bulan = setBulan;
unsigned long tahun = setTahun;
bool isNewDay = false;

const int max485_de = 3, max485_re_neg = 2; // for modbus

ModbusMaster node;

void preTransmission();
void postTransmission();
void change_time(const uint8_t valuesjam[], const uint8_t valueshari, const uint8_t dd, const uint8_t mm, const unsigned long int yy);

void setup()
{
  Serial.begin(9600);
  setupInterrupt();
  initSevSeg();
  initLCD();
  
  pinMode(button0, INPUT);
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);

  pinMode(max485_re_neg, OUTPUT);
  pinMode(max485_de, OUTPUT);
  digitalWrite(max485_re_neg, 0);
  digitalWrite(max485_de, 0);

  Serial.begin(115200);
  node.begin(1, Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

//  delay(5000);
//  node.writeSingleRegister(0x40004,hari);
//  node.writeSingleRegister(0x40005,tanggal);
//  node.writeSingleRegister(0x40006,bulan);
//  node.writeSingleRegister(0x40007,tahun);
}

byte tcnt2;

void initSevSeg () {
  pinMode(sevseg_clock, OUTPUT); // pin 7 is 7 seg clock
  pinMode(sevseg_data, OUTPUT); // pin 8 is 7 seg data
  sevseg.setBrightness(1);
}


void initLCD(){
  lcd.begin();
  lcd.backlight();
  lcd.home();
  lcd.noDisplay();
  delay(300);
  lcd.display();
  lcd.setCursor(0,0);
  lcd.print("Day:" + String(namaHari[hari]));
  lcd.setCursor(0,1);
  lcd.print("Date:" + String(tanggal) + "/" + String(bulan) + "/" + String(tahun));
}
  
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
//  if (!isNewDay) isNewDay = time / 86400000;
  time = time % 86400000; // miliseconds in a day
}

int sendCount = 0;

bool state = true;

void incrementDay();
void showCalendar();
void button_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds);

void loop()
{
  Serial.println("loop");
  unsigned long t = (unsigned long)(time/1000);
  uint8_t minutes = (byte)((t / 60) % 60);
  uint8_t hours = (byte)((t / 3600) % 24);
  uint8_t seconds = (byte)(t % 60);
  sevseg.showNumberDecEx(minutes, 0, true, 2, 2);
  sevseg.showNumberDecEx(hours, (0x80 >> seconds % 2), true, 2, 0);

  if (hours == 0 && minutes == 0 && seconds == 0 && !isNewDay) {
    incrementDay();
    showCalendar();
    isNewDay = true;
  }
  else if (isNewDay && seconds != 0) isNewDay = false;
  
  if (digitalRead(button0) == HIGH) button_set_time(hours, minutes, seconds);

  if (sendCount % 10 == 0) {
    node.writeSingleRegister(0x40001,hours);
    node.writeSingleRegister(0x40002,minutes);
    node.writeSingleRegister(0x40004,hari);
    node.writeSingleRegister(0x40005,tanggal);
    node.writeSingleRegister(0x40006,bulan);
    node.writeSingleRegister(0x40007,tahun);

        
    uint16_t data[7];
    uint8_t j, result;
    result = node.readHoldingRegisters(10, 1);
    uint8_t change = node.getResponseBuffer(0);

    if (change) {
        result = node.readHoldingRegisters(11, 7);
        
        // do something with data if read is successful
        if (result == node.ku8MBSuccess)
        {
          for (j = 0; j < 7; j++)
          {
            data[j] = node.getResponseBuffer(j);
          }
          
          uint8_t valuesjam[] = {data[0], data[1]};
          change_time(valuesjam, data[3], data[4], data[5], data[6]);
          showCalendar();
          node.writeSingleRegister(0x4000A,0);
        }
    }
  }

  sendCount++;
  
  delay(100);
}

void incrementDay() {
  hari = (hari + 1) % 7;
  tanggal++;

  if (tanggal == 32) {
    incrementMonth();
  } else if (tanggal == 31) {
    if (bulan == 4 || bulan == 6 || bulan == 9 || bulan == 11) {
      incrementMonth();
    }
  } else if (tanggal == 30) {
    if (bulan == 2) {
      incrementMonth();
    }
  } else if (tanggal == 29) {
    if (bulan == 2)
      if (tahun % 4 != 0) {
        incrementMonth();
      }
  }
  
  node.writeSingleRegister(0x40004,hari);
  node.writeSingleRegister(0x40005,tanggal);
  node.writeSingleRegister(0x40006,bulan);
  node.writeSingleRegister(0x40007,tahun);
}

void incrementMonth() {
  tanggal = 1;
  bulan++;
  if (bulan == 13) {
    bulan = 1;
    tahun ++;
  }
}

void showCalendar(){
    lcd.clear();
    lcd.home();
    lcd.setCursor(0,0);
    lcd.print("Day:" + String(namaHari[hari]));
    lcd.setCursor(0,1);
    
    char tanggalString [3];
    sprintf(tanggalString, "%02u", tanggal);
    char bulanString [3];
    sprintf(bulanString, "%02u", bulan);
    
    lcd.print("Date:" + String(tanggalString) + "/" + String(bulanString) + "/" + String(tahun));
    
}

void button_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds) {
  uint8_t valuesjam[] = { hours, minutes}; // user input value
  uint8_t valueshari = hari;
  uint8_t pointer = 0; // 0 for hours, 1 for minutes, 2 for day. 3 for date, 4 for month, 5 for year, 6 for exit
  uint8_t count = 0; // for blinking purposes
  uint8_t dd = tanggal;
  uint8_t mm = bulan;
  unsigned long int yy = tahun;
  
  while(pointer < 6) {
    // blink every 900 ms
    if (count % 3 == 0) {
      if (count % 6 == 0) { // clear
        if (pointer < 2) {
          uint8_t zero[] = { 0, 0x80 }; // for blank 7 segment
          sevseg.setSegments(zero, 2, pointer*2); // clear digit
        }
        else blinkLCD(pointer, valueshari, dd, mm, yy);
      }
      else { // show
        if (pointer < 2) sevseg.showNumberDecEx(valuesjam[pointer], 0x40, true, 2, pointer*2);
        previewCalendar(valueshari, dd, mm, yy);
      }
    }
    
    // increment time
    if (digitalRead(button1) == HIGH) {
      // for hours
      if (pointer == 0) {
        if (valuesjam[0] < 23){
          valuesjam[0]++;
        } else{
          valuesjam[0] = 0;
        }
        // show number
        sevseg.showNumberDecEx(valuesjam[pointer], 0x40, true, 2, pointer*2);
      // for minutes
      } else if (pointer == 1){
        if (valuesjam[1] < 59){
          valuesjam[1]++;
        } else{
          valuesjam[1] = 0;
        }
        sevseg.showNumberDecEx(valuesjam[pointer], 0x40, true, 2, pointer*2);
      // for days
      } else if (pointer == 2){
        if (valueshari < 6){
          valueshari++;
        } else{
          valueshari = 0;
        }
        previewCalendar(valueshari, dd, mm, yy);
      // untuk tanggal
      } else if (pointer == 3){
        if (mm == 1 || mm == 3 || mm == 5 || mm == 7 || mm == 8 || mm == 10 || mm == 12){ // bulan dengan jumlah 31 hari
          if (dd < 31){
            dd++;
          } else {
            dd = 1;
          }
        } else if (mm == 4 || mm == 6 || mm == 9 || mm == 11){ // bulan dengan jumlah 30 hari
          if (dd < 30){
            dd++;
          } else{
            dd = 1;
          }
        } else if (mm == 2){
          if (yy % 4 == 0){
            if (dd < 29){           // Februari di tahun kabisat
              dd++;
            } else{                         // Februari di tahun non kabisat
              dd = 1;
            }
          } else{
            if (dd < 28){
              dd++;
            } else{
              dd = 1;
            }
          }
       }
       previewCalendar(valueshari, dd, mm, yy);
     // untuk bulan
     } else if (pointer == 4){
       if (mm < 12){
          mm++;
       } else {
          mm = 1;
       }
       if ((mm == 4 || mm == 6 || mm == 9 || mm == 11) && (dd > 30)){ // Bulan dengan jumlah 30 hari
          dd = 30;
       } else if (mm == 2){                                                            // Bulan Februari
         dd = 28;
       }
       previewCalendar(valueshari, dd, mm, yy);
     // untuk tahun
     } else {
        yy++;
        if ((yy % 4 != 0) && (mm == 2) && (dd > 28)){
           dd = 28;
        }
        previewCalendar(valueshari, dd, mm, yy);
     }
    }

    // decrement time
    if (digitalRead(button2) == HIGH) {
      // for hours
      if (pointer == 0) {
        if (valuesjam[0] > 0){
          valuesjam[0]--;
        } else{
          valuesjam[0] = 23;
        }
        // show number
        sevseg.showNumberDecEx(valuesjam[pointer], 0x40, true, 2, pointer*2);
      // for minutes
      } else if (pointer == 1){
        if (valuesjam[1] > 0){
          valuesjam[1]--;
        } else{
          valuesjam[1] = 59;
        }
        sevseg.showNumberDecEx(valuesjam[pointer], 0x40, true, 2, pointer*2);
      // for days
      } else if (pointer == 2){
        if (valueshari > 0){
          valueshari--;
        } else{
          valueshari = 6;
        }
        previewCalendar(valueshari, dd, mm, yy);
      // untuk tanggal
      } else if (pointer == 3){
        if (mm == 1 || mm == 3 || mm == 5 || mm == 7 || mm == 8 || mm == 10 || mm == 12){ // bulan dengan jumlah 31 hari
          if (dd > 1){
            dd--;
          } else {
            dd = 31;
          }
        } else if (mm == 4 || mm == 6 || mm == 9 || mm == 11){ // bulan dengan jumlah 30 hari
          if (dd > 1){
            dd--;
          } else{
            dd = 30;
          }
        } else if (mm == 2){
          if (yy % 4 == 0){
            if (dd > 1){           // Februari di tahun kabisat
              dd--;
            } else{                         // Februari di tahun non kabisat
              dd = 29;
            }
          } else{
            if (dd > 1){
              dd--;
            } else{
              dd = 28;
            }
          }
       }
       previewCalendar(valueshari, dd, mm, yy);
     // untuk bulan
     } else if (pointer == 4){
       if (mm > 1){
          mm--;
       } else {
          mm = 12;
       }
       if ((mm == 4 || mm == 6 || mm == 9 || mm == 11) && (dd > 30)){ // Bulan dengan jumlah 30 hari
          dd = dd - 1;
       } else if ((mm == 2) && (yy % 4 == 0) && (dd > 29)){                                                            // Bulan Februari
         dd = 29;
       } else if ((mm == 2) && (dd > 28)){                                                            // Bulan Februari
         dd = 28;
       }
       previewCalendar(valueshari, dd, mm, yy);
     // untuk tahun
     } else {
        yy--;
        if ((yy % 4 != 0) && (mm == 2) && (dd > 28)){
           dd = 28;
        }
        previewCalendar(valueshari, dd, mm, yy);
     }
    }
    
    count ++;
    delay(150); // loop delay

    // setting button clicked
    if (digitalRead(button0) == HIGH) {
        if (pointer < 2){
          sevseg.showNumberDecEx(valuesjam[pointer], 0x40, true, 2, pointer*2); // stop blinking for digit
        } else{
          previewCalendar(valueshari, dd, mm, yy);
        }
        pointer++;
    }
  }
  change_time(valuesjam, valueshari, dd, mm, yy); // save time
}

void change_time(const uint8_t valuesjam[], const uint8_t valueshari, const uint8_t dd, const uint8_t mm, const unsigned long int yy){ // values = { hours, minutes}
  time = ( (unsigned long int) valuesjam[1] * 60 * 1000) + ( (unsigned long int) valuesjam[0] * 3600 *1000);
  hari = valueshari;
  tanggal = dd;
  bulan = mm;
  tahun = yy;
}

void previewCalendar(uint8_t valueshari, uint8_t dd, uint8_t mm, unsigned long yy) {
  lcd.clear();
  lcd.home();
  lcd.setCursor(0,0);
  lcd.print("Day:");
  lcd.setCursor(4,0);
  lcd.print(String(namaHari[valueshari]));
  lcd.setCursor(0,1);

  char ddString [3];
  sprintf(ddString, "%02u", dd);
  char mmString [3];
  sprintf(mmString, "%02u", mm);
  
  lcd.print("Date:" + String(ddString) + "/" + String(mmString) + "/" + String(yy)); // show days
}

void blinkLCD(uint8_t pointer, uint8_t valueshari, uint8_t dd, uint8_t mm, unsigned long yy) {
  // cursors
  // (5,1) tanggal
  // (8,1) bulan
  // (11,1) tahun

  // pasang 0 di satu digit
  char ddString [3];
  sprintf(ddString, "%02u", dd);
  char mmString [3];
  sprintf(mmString, "%02u", mm);
  
  if (pointer == 2){
    lcd.clear();
    lcd.home();
    lcd.setCursor(0,0);
    lcd.print("Day:");
    lcd.setCursor(0,1);
    lcd.print("Date:" + String(ddString) + "/" + String(mmString) + "/" + String(yy)); // clear days
  }
  else if (pointer == 3){
      lcd.clear();
      lcd.home();
      lcd.setCursor(0,0);
      lcd.print("Day:" + String(namaHari[valueshari]));
      lcd.setCursor(0,1);
      lcd.print("Date:");
      lcd.setCursor(7,1);
      lcd.print("/" + String(mmString) + "/" + String(yy)); // clear tanggal
  } else if (pointer == 4){
      lcd.clear();
      lcd.home();
      lcd.setCursor(0,0);
      lcd.print("Day:" + String(namaHari[valueshari]));
      lcd.setCursor(0,1);
      lcd.print("Date:" + String(ddString) + "/");
      lcd.setCursor(10,1);
      lcd.print("/" + String(yy));            // clear bulan
  } else {
      lcd.clear();
      lcd.home();
      lcd.setCursor(0,0);
      lcd.print("Day:" + String(namaHari[valueshari]));
      lcd.setCursor(0,1);
      lcd.print("Date:" + String(ddString) + "/" + String(mmString) + "/");          // clear tahun
    }
}

void preTransmission()
{
  digitalWrite(max485_re_neg, 1);
  digitalWrite(max485_de, 1);
}
void postTransmission()
{
  digitalWrite(max485_re_neg, 0);
  digitalWrite(max485_de, 0);
}
