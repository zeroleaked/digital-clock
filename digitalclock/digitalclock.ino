#include <TM1637Display.h>
#include <LiquidCrystal_I2C.h>

const int sevseg_clock = 7, sevseg_data = 8;
uint8_t digits[] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f }; // decimal to 7 segment

TM1637Display sevseg(sevseg_clock, sevseg_data);
LiquidCrystal_I2C lcd(0x27,16,2);

String setHari[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
uint8_t hari = 6;
uint8_t setTanggal = 28, setBulan = 2;
unsigned long int setTahun = 2021;
const int button0 = 4, button1 = 5, button2 = 6; // button0 setting, button1 increment time, button2 decrement time

void setup()
{
  Serial.begin(9600);
  setupInterrupt();
  pinMode(sevseg_clock, OUTPUT); // pin 7 is 7 seg clock
  pinMode(sevseg_data, OUTPUT); // pin 8 is 7 seg data
  initCalendar(setHari, hari, setTanggal, setBulan, setTahun);
  sevseg.setBrightness(1);
  
  pinMode(button0, INPUT);
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  Serial.println("setup done");
}

byte tcnt2;
// initial time
unsigned long int setMinutes = 59;
unsigned long int setHours = 23;
unsigned long time = (setMinutes * 60 * 1000) + (setHours * 3600 *1000); // in miliseconds
void initCalendar(String setHari[], uint8_t hari, uint8_t setTanggal, uint8_t setBulan, unsigned long int setTahun){
  lcd.begin();
  lcd.backlight();
  lcd.home();
  lcd.noDisplay();
  sevseg.clear();
  delay(300);
  lcd.display();
  lcd.setCursor(0,0);
  lcd.print("Day:" + String(setHari[hari]));
  lcd.setCursor(0,1);
  lcd.print("Date:" + String(setTanggal) + "/" + String(setBulan) + "/" + String(setTahun));
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
  showCalendar(hours, minutes, seconds, setHari, hari, setTanggal, setBulan, setTahun);
  if (digitalRead(button0) == HIGH) button_set_time(hours, minutes, seconds, setHari, hari, setTanggal, setBulan, setTahun);
  delay(100);
}

void showCalendar(uint8_t hours, uint8_t minutes, uint8_t seconds, String setHari[], uint8_t hari, uint8_t setTanggal, uint8_t setBulan, unsigned long int setTahun){
  if ((hours == 0)&&(minutes == 0)&&(seconds == 0)){
    if (setBulan == 1 || setBulan == 3 || setBulan == 5 || setBulan == 7 || setBulan == 8 || setBulan == 10 || setBulan == 12){ // bulan dengan jumlah 31 hari
          if (setTanggal < 31){
            setTanggal++;
          } else {
            setTanggal = 1;
          }
    } else if (setBulan == 4 || setBulan == 6 || setBulan == 9 || setBulan == 11){ // bulan dengan jumlah 30 hari
          if (setTanggal < 30){
            setTanggal++;
          } else{
            setTanggal = 1;
          }
    } else if (setBulan == 2){
          if (setTahun % 4 == 0){
            if (setTanggal < 29){           // Februari di tahun kabisat
              setTanggal++;
            } else{                         // Februari di tahun non kabisat
              setTanggal = 1;
            }
          } else{
            if (setTanggal < 28){
              setTanggal++;
            } else{
              setTanggal = 1;
            }
          }
    }
    
    if (hari < 6){
      hari++;
    } else{
      hari = 0;
    }
     
    if ((setTanggal == 28) && (setBulan == 2)){
      if (setBulan < 12){
          setBulan++;
       } else {
          setBulan = 1;
       }
    } else if((setTanggal == 29) && (setBulan == 2) & (setTahun % 4 == 0)){
      if (setBulan < 12){
          setBulan++;
       } else {
          setBulan = 1;
       }
    } else if((setTanggal == 30) && ((setBulan == 4)||(setBulan == 6)||(setBulan == 9)||(setBulan == 11))){
      if (setBulan < 12){
          setBulan++;
       } else {
          setBulan = 1;
       }
    } else if((setTanggal == 31) && ((setBulan == 1)||(setBulan == 3)||(setBulan == 5)||(setBulan == 7)||(setBulan == 8)||(setBulan == 10)||(setBulan == 12))){
      if (setBulan < 12){
          setBulan++;
       } else {
          setBulan = 1;
       }
    }
    
    if ((setTanggal == 31) && (setBulan == 12)){
      setTahun++;
    }
    
    lcd.clear();
    lcd.home();
    lcd.setCursor(0,0);
    lcd.print("Day:" + String(setHari[hari]));
    lcd.setCursor(0,1);
    lcd.print("Date:" + String(setTanggal) + "/" + String(setBulan) + "/" + String(setTahun));
  }
}

void button_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds, String setHari[], uint8_t hari, uint8_t setTanggal, uint8_t setBulan, unsigned long int setTahun) {
  uint8_t valuesjam[] = { hours, minutes}; // user input value
  uint8_t valueshari = hari;
  uint8_t pointer = 0; // 0 for hours, 1 for minutes
  uint8_t zero[] = { 0, 0x80 }; // for blank 7 segment
  uint8_t count = 0; // for blinking purposes
  uint8_t dd = setTanggal;
  uint8_t mm = setBulan;
  unsigned long int yy = setTahun;
  while(1) {
    // blink every 900 ms
    if (pointer < 2){
      if (count % 3 == 0) { // every 450 ms
        if (count % 6 == 0)
          sevseg.setSegments(zero, 2, pointer*2); // clear digit
        else
          sevseg.showNumberDecEx(valuesjam[pointer], 0x40, true, 2, pointer*2); // show digit
      }
    } else if (pointer == 2){
      if (count % 3 == 0) { // every 450 ms
        if (count % 6 == 0){
          lcd.clear();
          lcd.home();
          lcd.setCursor(0,0);
          lcd.print("Day:");
          lcd.setCursor(0,1);
          lcd.print("Date:" + String(dd) + "/" + String(mm) + "/" + String(yy)); // clear days
        }  else{
          lcd.clear();
          lcd.home();
          lcd.setCursor(0,0);
          lcd.print("Day:");
          lcd.setCursor(4,0);
          lcd.print(String(setHari[valueshari]));
          lcd.setCursor(0,1);
          lcd.print("Date:" + String(dd) + "/" + String(mm) + "/" + String(yy)); // show days
        }
      }
    } else if (pointer == 3){
      if (count % 3 == 0) { // every 450 ms
        if (count % 6 == 0){
          lcd.clear();
          lcd.home();
          lcd.setCursor(0,0);
          lcd.print("Day:" + String(setHari[valueshari]));
          lcd.setCursor(0,1);
          lcd.print("Date:");
          lcd.setCursor(6,1);
          lcd.print("/" + String(mm) + "/" + String(yy)); // clear tanggal
        } else{
          lcd.clear();
          lcd.home();
          lcd.setCursor(0,0);
          lcd.print("Day:" + String(setHari[valueshari]));
          lcd.setCursor(0,1);
          lcd.print("Date:" + String(dd));
          if (dd <10){
            lcd.setCursor(6,1);
            lcd.print("/" + String(mm) + "/" + String(yy));
          } else{
            lcd.setCursor(7,1);
            lcd.print("/" + String(mm) + "/" + String(yy)); // show tanggal
          }
        }
      }
    } else if (pointer == 4){
      if (count % 3 == 0) { // every 450 ms
        if (count % 6 == 0){
          lcd.clear();
          lcd.home();
          lcd.setCursor(0,0);
          lcd.print("Day:" + String(setHari[valueshari]));
          lcd.setCursor(0,1);
          lcd.print("Date:" + String(dd) + "/");
          if (dd < 10){
            lcd.setCursor(8,1);
            lcd.print("/" + String(yy));
          } else{
            lcd.setCursor(9,1);
            lcd.print("/" + String(yy));            // clear bulan
          }
        } else{
          lcd.clear();
          lcd.home();
          lcd.setCursor(0,0);
          lcd.print("Day:" + String(setHari[valueshari]));
          lcd.setCursor(0,1);
          lcd.print("Date:" + String(dd) + "/");
          if (dd < 10){
            lcd.setCursor(7,1);
            lcd.print(String(mm));
            if (mm < 10){
              lcd.setCursor(8,1);
              lcd.print("/" + String(yy));
            } else {
              lcd.setCursor(9,1);
              lcd.print("/" + String(yy));
            }
          } else{
            lcd.setCursor(8,1);
            lcd.print(String(mm));
            if (mm < 10){
              lcd.setCursor(9,1);
              lcd.print("/" + String(yy));
            } else {
              lcd.setCursor(10,1);
              lcd.print("/" + String(yy));        // show bulan
            }
          }
        }
      }
    } else{
      if (count % 3 == 0) { // every 450 ms
        if (count % 6 == 0){
          lcd.clear();
          lcd.home();
          lcd.setCursor(0,0);
          lcd.print("Day:" + String(setHari[valueshari]));
          lcd.setCursor(0,1);
          lcd.print("Date:" + String(dd) + "/" + String(mm) + "/");          // clear tahun
        } else{
          lcd.clear();
          lcd.home();
          lcd.setCursor(0,0);
          lcd.print("Day:" + String(setHari[valueshari]));
          lcd.setCursor(0,1);
          lcd.print("Date:" + String(dd) + "/");
          if ((dd < 10) && (mm < 10)){
            lcd.setCursor(9,1);
            lcd.print(String(yy));
          } else if (((dd < 10) && (mm > 9)) || ((dd > 9) && (mm < 10))){
            lcd.setCursor(10,1);
            lcd.print(String(yy));
          } else if ((dd > 9) && (mm > 9)){
            lcd.setCursor(11,1);
            lcd.print(String(yy));                                  // show tahun
          }
        }
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
        if (hari < 6){
          hari++;
        } else{
          hari = 0;
        }
        valueshari = hari;
        lcd.clear();
        lcd.home();
        lcd.setCursor(0,0);
        lcd.print("Day:");
        lcd.setCursor(4,0);
        lcd.print(String(setHari[valueshari]));
        lcd.setCursor(0,1);
        lcd.print("Date:" + String(dd) + "/" + String(mm) + "/" + String(yy));
      // untuk tanggal
      } else if (pointer == 3){
        if (setBulan == 1 || setBulan == 3 || setBulan == 5 || setBulan == 7 || setBulan == 8 || setBulan == 10 || setBulan == 12){ // bulan dengan jumlah 31 hari
          if (setTanggal < 31){
            setTanggal++;
          } else {
            setTanggal = 1;
          }
        } else if (setBulan == 4 || setBulan == 6 || setBulan == 9 || setBulan == 11){ // bulan dengan jumlah 30 hari
          if (setTanggal < 30){
            setTanggal++;
          } else{
            setTanggal = 1;
          }
        } else if (setBulan == 2){
          if (setTahun % 4 == 0){
            if (setTanggal < 29){           // Februari di tahun kabisat
              setTanggal++;
            } else{                         // Februari di tahun non kabisat
              setTanggal = 1;
            }
          } else{
            if (setTanggal < 28){
              setTanggal++;
            } else{
              setTanggal = 1;
            }
          }
       }
       dd = setTanggal;
       lcd.clear();
       lcd.home();
       lcd.setCursor(0,0);
       lcd.print("Day:" + String(setHari[valueshari]));
       lcd.setCursor(0,1);
       lcd.print("Date:");
       lcd.setCursor(5,1);
       lcd.print(String(dd));
       if (dd <10){
        lcd.setCursor(6,1);
        lcd.print("/" + String(mm) + "/" + String(yy));
       } else{
        lcd.setCursor(7,1);
        lcd.print("/" + String(mm) + "/" + String(yy));
       }
     // untuk bulan
     } else if (pointer == 4){
       if (setBulan < 12){
          setBulan++;
       } else {
          setBulan = 1;
       }
       mm = setBulan;
       if ((mm == 4 || mm == 6 || mm == 9 || mm == 11) && (dd > 30)){ // Bulan dengan jumlah 30 hari
          dd = dd - 1;
          lcd.clear();
          lcd.home();
          lcd.setCursor(0,0);
          lcd.print("Day:" + String(setHari[valueshari]));
          lcd.setCursor(0,1);
          lcd.print("Date:" + String(dd) + "/" + String(mm) + "/" + String(yy));
       } else if (mm == 2){                                                            // Bulan Februari
         dd = dd - 3;
         lcd.clear();
         lcd.home();
         lcd.setCursor(0,0);
         lcd.print("Day:" + String(setHari[valueshari]));
         lcd.setCursor(0,1);
         lcd.print("Date:" + String(dd) + "/" + String(mm) + "/" + String(yy)); 
       }
       lcd.clear();
       lcd.home();
       lcd.setCursor(0,0);
       lcd.print("Day:" + String(setHari[valueshari]));
       lcd.setCursor(0,1);
       lcd.print("Date:" + String(dd) + "/");
       if (dd < 10){
        lcd.setCursor(7,1);
        lcd.print(String(mm));
       } else{
        lcd.setCursor(8,1);
        lcd.print(String(mm));
       }
       if (mm < 10){
        lcd.setCursor(8,1);
        lcd.print("/" + String(yy));
       } else{
        lcd.setCursor(9,1);
        lcd.print("/" + String(yy));
       }
     // untuk tahun
     } else {
        setTahun++;
        yy = setTahun;
        if ((yy % 4 != 0) && (mm == 2) && (dd == 29)){
           dd = dd - 1;
           lcd.clear();
           lcd.home();
           lcd.setCursor(0,0);
           lcd.print("Day:" + String(setHari[valueshari]));
           lcd.setCursor(0,1);
           lcd.print("Date:" + String(dd) + "/" + String(mm) + "/" + String(yy));
        }
        lcd.clear();
        lcd.home();
        lcd.setCursor(0,0);
        lcd.print("Day:" + String(setHari[valueshari]));
        lcd.setCursor(0,1);
        lcd.print("Date:" + String(dd) + "/" + String(mm) + "/");
        if ((dd < 10) && (mm < 10)){
         lcd.setCursor(9,1);
         lcd.print(String(yy));
        } else if (((dd < 10) && (mm > 9)) || ((dd > 9) && (mm < 10))){
         lcd.setCursor(10,1);
         lcd.print(String(yy));
        } else if ((dd > 9) && (mm > 9)){
         lcd.setCursor(11,1);
         lcd.print(String(yy));
       }
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
        if (hari > 0){
          hari--;
        } else{
          hari = 6;
        }
        valueshari = hari;
        lcd.clear();
        lcd.home();
        lcd.setCursor(0,0);
        lcd.print("Day:");
        lcd.setCursor(4,0);
        lcd.print(String(setHari[valueshari]));
        lcd.setCursor(0,1);
        lcd.print("Date:" + String(dd) + "/" + String(mm) + "/" + String(yy));
      // untuk tanggal
      } else if (pointer == 3){
        if (setBulan == 1 || setBulan == 3 || setBulan == 5 || setBulan == 7 || setBulan == 8 || setBulan == 10 || setBulan == 12){ // bulan dengan jumlah 31 hari
          if (setTanggal > 1){
            setTanggal--;
          } else {
            setTanggal = 31;
          }
        } else if (setBulan == 4 || setBulan == 6 || setBulan == 9 || setBulan == 11){ // bulan dengan jumlah 30 hari
          if (setTanggal > 1){
            setTanggal--;
          } else{
            setTanggal = 30;
          }
        } else if (setBulan == 2){
          if (setTahun % 4 == 0){
            if (setTanggal > 1){           // Februari di tahun kabisat
              setTanggal--;
            } else{                         // Februari di tahun non kabisat
              setTanggal = 29;
            }
          } else{
            if (setTanggal > 1){
              setTanggal--;
            } else{
              setTanggal = 28;
            }
          }
       }
       dd = setTanggal;
       lcd.clear();
       lcd.home();
       lcd.setCursor(0,0);
       lcd.print("Day:" + String(setHari[valueshari]));
       lcd.setCursor(0,1);
       lcd.print("Date:");
       lcd.setCursor(5,1);
       lcd.print(String(dd));
       if (dd <10){
        lcd.setCursor(6,1);
        lcd.print("/" + String(mm) + "/" + String(yy));
       } else{
        lcd.setCursor(7,1);
        lcd.print("/" + String(mm) + "/" + String(yy));
       }
     // untuk bulan
     } else if (pointer == 4){
       if (setBulan > 1){
          setBulan--;
       } else {
          setBulan = 12;
       }
       mm = setBulan;
       if ((mm == 4 || mm == 6 || mm == 9 || mm == 11) && (dd > 30)){ // Bulan dengan jumlah 30 hari
          dd = dd - 1;
          lcd.clear();
          lcd.home();
          lcd.setCursor(0,0);
          lcd.print("Day:" + String(setHari[valueshari]));
          lcd.setCursor(0,1);
          lcd.print("Date:" + String(dd) + "/" + String(mm) + "/" + String(yy));
       } else if (mm == 2){                                                            // Bulan Februari
         dd = dd - 3;
         lcd.clear();
         lcd.home();
         lcd.setCursor(0,0);
         lcd.print("Day:" + String(setHari[valueshari]));
         lcd.setCursor(0,1);
         lcd.print("Date:" + String(dd) + "/" + String(mm) + "/" + String(yy)); 
       }
       lcd.clear();
       lcd.home();
       lcd.setCursor(0,0);
       lcd.print("Day:" + String(setHari[valueshari]));
       lcd.setCursor(0,1);
       lcd.print("Date:" + String(dd) + "/");
       if (dd < 10){
        lcd.setCursor(7,1);
        lcd.print(String(mm));
       } else{
        lcd.setCursor(8,1);
        lcd.print(String(mm));
       }
       if (mm < 10){
        lcd.setCursor(8,1);
        lcd.print("/" + String(yy));
       } else{
        lcd.setCursor(9,1);
        lcd.print("/" + String(yy));
       }
     // untuk tahun
     } else {
        setTahun--;
        yy = setTahun;
        if ((yy % 4 != 0) && (mm == 2) && (dd == 29)){
           dd = dd - 1;
           lcd.clear();
           lcd.home();
           lcd.setCursor(0,0);
           lcd.print("Day:" + String(setHari[valueshari]));
           lcd.setCursor(0,1);
           lcd.print("Date:" + String(dd) + "/" + String(mm) + "/" + String(yy));
        }
        lcd.clear();
        lcd.home();
        lcd.setCursor(0,0);
        lcd.print("Day:" + String(setHari[valueshari]));
        lcd.setCursor(0,1);
        lcd.print("Date:" + String(dd) + "/" + String(mm) + "/");
        if ((dd < 10) && (mm < 10)){
         lcd.setCursor(9,1);
         lcd.print(String(yy));
        } else if (((dd < 10) && (mm > 9)) || ((dd > 9) && (mm < 10))){
         lcd.setCursor(10,1);
         lcd.print(String(yy));
        } else if ((dd > 9) && (mm > 9)){
         lcd.setCursor(11,1);
         lcd.print(String(yy));
       }
     }
    }
    
    count ++;
    delay(150); // loop delay

    // setting button clicked
    if (digitalRead(button0) == HIGH) {
        if (pointer < 2){
          sevseg.showNumberDecEx(valuesjam[pointer], 0x40, true, 2, pointer*2); // stop blinking for digit
        } else{
          lcd.clear();
          lcd.home();
          lcd.setCursor(0,0);
          lcd.print("Day:" + String(setHari[valueshari]));
          lcd.setCursor(0,1);
          lcd.print("Date:" + String(dd) + "/" + String(mm) + "/" + String(yy)); // stop blinking for day
        }
        if (pointer < 5) pointer++; // change pointer
        else {
          change_time(valuesjam, setHari, valueshari, dd, mm, yy); // save time
          return;
        }
    }
  };
}

void change_time(const uint8_t valuesjam[], String setHari[], const uint8_t valueshari, const uint8_t dd, const uint8_t mm, const unsigned long int yy){ // values = { hours, minutes}
  time = ( (unsigned long int) valuesjam[1] * 60 * 1000) + ( (unsigned long int) valuesjam[0] * 3600 *1000);
  setHari[hari] = setHari[valueshari];
  setTanggal = dd;
  setBulan = mm;
  setTahun = yy;
}
