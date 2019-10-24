#include <EEPROM.h>
#include <Wire.h>
#include "RTClib.h"

#include <LCD5110_Graph.h>
#include "sha1.h"

#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

// Define time step required for OTP changes
#define timeStep 30

// Define bar thickness for display
#define BARTH 2

// Define idle time to sleep in seconds
#define SLEEP_IDLE 60

RTC_DS1307 RTC;

/* Days of the Week */
char daysOfTheWeek[7][12] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

/* Display Pins */
//      SCK  - Pin 8    #14
//      MOSI - Pin 9    #15
//      DC   - Pin 10   #16
//      RST  - Pin 11   #17
//      CS   - Pin 12   #18
LCD5110 LCD(8,9,10,11,12);

/* Trigger Sleep Mode Button */
#define TRIG_SLEEP  2
/* Show Time/OTP Button */
#define TRIG_SHOW   7
/* Trigger Tamper Detection */
#define TRIG_TAMPER 13

uint8_t Key[20];

/* Font Sizes */
extern uint8_t TinyFont[];
extern uint8_t SmallFont[];
extern uint8_t MediumNumbers[];
extern uint8_t BigNumbers[];

/* Time Variable */
long intern;
long prev_intern = 0;
int last_idle = 0;
int idle_count = 0;
/* OTP Variable */
long oldOtp = 0;

/* Date/Time Variable */
DateTime now;
String LCD_dow, LCD_date, LCD_hour, LCD_min, LCD_gmt_hour, LCD_gmt_minute;
int t_dow, t_year, t_month, t_day, t_hour, t_minute;
int gmt = 0;  // in minutes

/* Push button status */
int curr, prev;
int curr_show, prev_show;
bool sleep, mode, detect;

void setup() {
  // Initiate Nokia 5110 Display module
  LCD.InitLCD();
/*
  for (byte i = 0; i < 20; i++) {
    pinMode(i, INPUT_PULLUP);
  }
*/
  wdt_disable();
  power_adc_disable();
  power_spi_disable();
  power_timer1_disable();
  power_timer2_disable();
  power_usart0_disable();
  // Sleep Mode Button
  pinMode(TRIG_SLEEP, INPUT_PULLUP);
  // Show OTP/Time Button
  pinMode(TRIG_SHOW, INPUT_PULLUP);
  // Tamper detection button
  pinMode(TRIG_TAMPER, INPUT_PULLUP);
  
  //Serial.begin(9600);
/*
  if (! RTC.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
  }
*/
  /* Uncomment this line after setting up the RTC */
  //setupTime();
  
  prev = digitalRead(TRIG_SLEEP);
  curr = prev;
  prev_show = digitalRead(TRIG_SHOW);
  curr_show = prev_show;
  sleep = false;
  mode = false;
  detect = false;
  LCD_gmt_hour = "00";
  LCD_gmt_minute = "00";

  EEPROM.get(0,Key);

  intTamperSetup();
  initState();
  // activate I2C and clock
  Wire.begin();
  RTC.begin();
}

void loop() {
  now = RTC.now();
  String TOTP = "";
  intern = now.unixtime() - (gmt * 60);
  long time = intern / timeStep;
  int timeElapsed = intern % timeStep;

  if (sleep) {
    LCD.enableSleep();
    goToSleep();
  }
  if (!sleep) {
    if (curr_show != prev_show) {
      idle_count = 0;
      if (curr_show == HIGH) {
        mode = !mode;
        LCD.clrScr();
      }
      prev_show = curr_show;
    }
    LCD.disableSleep();
  }

  LCD_dow = daysOfTheWeek[now.dayOfTheWeek()];
  LCD_date = String(now.day()) + '/' + String(now.month()) + '/' +
             String(now.year());
  if (now.hour() >= 0 && now.hour() < 10)
    LCD_hour = "0";
  else
    LCD_hour = "";
  LCD_hour = LCD_hour + String(now.hour());
  if (now.minute() >= 0 && now.minute() < 10)
    LCD_min = "0";
  else
    LCD_min = "";
  LCD_min = LCD_min + String(now.minute());
  t_dow = now.dayOfTheWeek();
  t_year = now.year();
  t_month = now.month();
  t_day = now.day();
  t_hour = now.hour();
  t_minute = now.minute();
  
  if (prev_intern != intern) {
    if (!sleep) {
      idle_count++;
      if (idle_count == SLEEP_IDLE) {
        sleep = true;
        idle_count = 0;
      }
    }
    prev_intern = intern;
  }

  // Enable sleep mode if button is pressed
  // Disable sleep mode if button is pressed again
  curr = digitalRead(TRIG_SLEEP);
  curr_show = digitalRead(TRIG_SHOW);
  if (curr != prev) {
    idle_count = 0;
    if (curr == HIGH) {
      sleep = !sleep;
    }
    prev = curr;
  }

  // Adjusting the time
  if (curr_show == LOW && curr == LOW) {
    // Wait until all buttons are released
    while (curr_show != HIGH || curr != HIGH) {
      curr = digitalRead(TRIG_SLEEP);
      curr_show = digitalRead(TRIG_SHOW);
    }
    // Adjusting the year
    // Wait until all buttons are pressed
    while (curr_show != LOW || curr != LOW) {
      LCD.clrScr();
      LCD.setFont(SmallFont);
      LCD.print("YEAR:", CENTER, 0);
      LCD.setFont(BigNumbers);
      LCD.print(String(t_year), CENTER, 10);
      LCD.update();
      curr = digitalRead(TRIG_SLEEP);
      curr_show = digitalRead(TRIG_SHOW);
      if (curr == LOW && curr_show == HIGH) {
        t_year = t_year + 1;
        if (t_year > 9999)
          t_year = 0;
      }
      else if (curr == HIGH && curr_show == LOW) {
        t_year = t_year - 1;
        if (t_year < 0)
          t_year = 9999;
      }
      delay(200);
    }
    // Wait until all buttons are released
    while (curr_show != HIGH || curr != HIGH) {
      curr = digitalRead(TRIG_SLEEP);
      curr_show = digitalRead(TRIG_SHOW);
    }
    // Adjusting the month
    // Wait until all buttons are pressed
    while (curr_show != LOW || curr != LOW) {
      LCD.clrScr();
      LCD.setFont(SmallFont);
      LCD.print("MONTH:", CENTER, 0);
      LCD.setFont(BigNumbers);
      LCD.print(String(t_month), CENTER, 10);
      LCD.update();
      curr = digitalRead(TRIG_SLEEP);
      curr_show = digitalRead(TRIG_SHOW);
      if (curr == LOW && curr_show == HIGH) {
        t_month = t_month + 1;
        if (t_month > 12)
          t_month = 1;
      }
      else if (curr == HIGH && curr_show == LOW) {
        t_month = t_month - 1;
        if (t_month < 1)
          t_month = 12;
      }
      delay(200);
    }
    // Wait until all buttons are released
    while (curr_show != HIGH || curr != HIGH) {
      curr = digitalRead(TRIG_SLEEP);
      curr_show = digitalRead(TRIG_SHOW);
    }
    // Adjusting the day
    // Wait until all buttons are pressed
    while (curr_show != LOW || curr != LOW) {
      LCD.clrScr();
      LCD.setFont(SmallFont);
      LCD.print("DAY:", CENTER, 0);
      LCD.setFont(BigNumbers);
      LCD.print(String(t_day), CENTER, 10);
      LCD.update();
      curr = digitalRead(TRIG_SLEEP);
      curr_show = digitalRead(TRIG_SHOW);
      if (curr == LOW && curr_show == HIGH) {
        t_day = t_day + 1;
        if (t_month == 1 || t_month == 3 || t_month == 5 || t_month == 7
            || t_month == 8 || t_month == 10 || t_month == 12) {
          if (t_day > 31)
            t_day = 1;
        }
        else if (t_month == 4 || t_month == 6 || t_month ==  9
                 || t_month == 11) {
          if (t_day > 30)
            t_day = 1;
        }
        else {
          if (t_year % 4 == 0) {
            if (t_day > 29)
              t_day = 1;
          }
          else {
            if (t_day > 28)
              t_day = 1;
          }
        }
      }
      else if (curr == HIGH && curr_show == LOW) {
        t_day = t_day - 1;
        if (t_month == 1 || t_month == 3 || t_month == 5 || t_month == 7
            || t_month == 8 || t_month == 10 || t_month == 12) {
          if (t_day < 1)
            t_day = 31;
        }
        else if (t_month == 4 || t_month == 6 || t_month ==  9
                 || t_month == 11) {
          if (t_day < 1)
            t_day = 30;
        }
        else {
          if (t_year % 4 == 0) {
            if (t_day < 1)
              t_day = 29;
          }
          else {
            if (t_day < 1)
              t_day = 28;
          }
        }
      }
      delay(200);
    }
    // Wait until all buttons are released
    while (curr_show != HIGH || curr != HIGH) {
      curr = digitalRead(TRIG_SLEEP);
      curr_show = digitalRead(TRIG_SHOW);
    }
    // Adjusting the hour and minute
    // Wait until all buttons are pressed
    while (curr_show != LOW || curr != LOW) {
      LCD.clrScr();
      LCD.setFont(BigNumbers);
      LCD.print(LCD_hour,7,14);
      LCD.print(LCD_min,49,14);
      LCD.print(".",36,10);
      LCD.print(".",36,0);
      LCD.setFont(SmallFont);
      LCD.print("HOUR:", CENTER, 0);
      LCD.update();
      curr = digitalRead(TRIG_SLEEP);
      curr_show = digitalRead(TRIG_SHOW);
      if (curr == LOW && curr_show == HIGH) {
        t_hour = t_hour + 1;
        if (t_hour > 23)
          t_hour = 0;
      }
      else if (curr == HIGH && curr_show == LOW) {
        t_hour = t_hour - 1;
        if (t_hour < 0)
          t_hour = 23;
      }
      if (t_hour >= 0 && t_hour < 10)
        LCD_hour = "0";
      else
        LCD_hour = "";
      LCD_hour = LCD_hour + String(t_hour);
      delay(200);
    }
    // Wait until all buttons are released
    while (curr_show != HIGH || curr != HIGH) {
      curr = digitalRead(TRIG_SLEEP);
      curr_show = digitalRead(TRIG_SHOW);
    }
    // Wait until all buttons are pressed
    while (curr_show != LOW || curr != LOW) {
      LCD.clrScr();
      LCD.setFont(BigNumbers);
      LCD.print(LCD_hour,7,14);
      LCD.print(LCD_min,49,14);
      LCD.print(".",36,10);
      LCD.print(".",36,0);
      LCD.setFont(SmallFont);
      LCD.print("MINUTE:", CENTER, 0);
      LCD.update();
      curr = digitalRead(TRIG_SLEEP);
      curr_show = digitalRead(TRIG_SHOW);
      if (curr == LOW && curr_show == HIGH) {
        t_minute = t_minute + 1;
        if (t_minute > 59)
          t_minute = 0;
      }
      else if (curr == HIGH && curr_show == LOW) {
        t_minute = t_minute - 1;
        if (t_minute < 0)
          t_minute = 59;
      }
      if (t_minute >= 0 && t_minute < 10)
        LCD_min = "0";
      else
        LCD_min = "";
      LCD_min = LCD_min + String(t_minute);
      delay(200);
    }
    // Wait until all buttons are released
    while (curr_show != HIGH || curr != HIGH) {
      curr = digitalRead(TRIG_SLEEP);
      curr_show = digitalRead(TRIG_SHOW);
    }
    /* The GMT */
    int gmt_abs;
    // Wait until all buttons are pressed
    while (curr_show != LOW || curr != LOW) {
      LCD.clrScr();
      LCD.setFont(BigNumbers);
      LCD.print(LCD_gmt_hour,7,14);
      LCD.print(LCD_gmt_minute,49,14);
      LCD.print(".",36,10);
      LCD.print(".",36,0);
      LCD.setFont(SmallFont);
      LCD.print("GMT:", CENTER, 0);
      if (gmt >= 0)
        LCD.print("+", 0, 20);
      else
        LCD.print("-", 0, 20);
      LCD.update();
      curr = digitalRead(TRIG_SLEEP);
      curr_show = digitalRead(TRIG_SHOW);
      if (curr == LOW && curr_show == HIGH) {
        gmt = gmt + 30;
        if (gmt > 720)
          gmt = -690;
      }
      else if (curr == HIGH && curr_show == LOW) {
        gmt = gmt - 30;
        if (gmt < -690)
          gmt = 720;
      }
      gmt_abs = abs(gmt);
      if ((gmt_abs / 60) >= 0 && (gmt_abs / 60) < 10)
        LCD_gmt_hour = "0";
      else
        LCD_gmt_hour = "";
      LCD_gmt_hour = LCD_gmt_hour + String(gmt_abs / 60);
      if ((gmt_abs % 60) >= 0 && (gmt_abs % 60) < 10)
        LCD_gmt_minute = "0";
      else
        LCD_gmt_minute = "";
      LCD_gmt_minute = LCD_gmt_minute + String(gmt_abs % 60);
      delay(200);
    }
    // Wait until all buttons are released
    while (curr_show != HIGH || curr != HIGH) {
      curr = digitalRead(TRIG_SLEEP);
      curr_show = digitalRead(TRIG_SHOW);
    }
    prev = curr;
    prev_show = curr_show;
    LCD.clrScr();
    RTC.adjust(DateTime(t_year, t_month, t_day, t_hour, t_minute, 0));
  }

  TOTP = generateOTP(Key,time);

  if (!sleep) {
    if (!mode) {
      displayTime(LCD_dow, LCD_date, LCD_hour, LCD_min);
    }
    else {
      showDuration(timeElapsed);
      LCD.setFont(TinyFont);
      LCD.print("ENTER  OTP:",CENTER,5);
      LCD.setFont(BigNumbers);
      LCD.print(TOTP,CENTER,15);
      LCD.setFont(TinyFont);
      LCD.update();
      //Serial.println(TOTP);
    }
  }
  else {
    LCD.clrScr();
  }
}

String generateOTP(uint8_t* Key, long steps) {
  // TOTP ---- An HOTP that uses time as variable
  //      ---- HOTP uses hash from SHA-1 hash function
  String TOTP = "";
  uint8_t byteArray[8]; 
  byteArray[0] = 0x00;
  byteArray[1] = 0x00;
  byteArray[2] = 0x00;
  byteArray[3] = 0x00;
  byteArray[4] = (int)((steps >> 24) & 0xFF);
  byteArray[5] = (int)((steps >> 16) & 0xFF);
  byteArray[6] = (int)((steps >> 8) & 0xFF);
  byteArray[7] = (int)((steps & 0xFF));
  
  Sha1.initHmac(Key,20);
  Sha1.writebytes(byteArray, 8);
  uint8_t* hash = Sha1.resultHmac();
  
  int offset = hash[19] & 0xF;
  long truncatedHash = 0;
  int j;
  for (j = 0; j < 4; ++j) {
    truncatedHash <<= 8;
    truncatedHash |= hash[offset + j];
  }
  
  truncatedHash &= 0x7FFFFFFF;
  truncatedHash %= 1000000;

  // Generate an output
  uint8_t dig6, dig5, dig4, dig3, dig2, dig1;
  dig6 = truncatedHash/100000;
  dig5 = (truncatedHash%100000)/10000;
  dig4 = (truncatedHash%10000)/1000;
  dig3 = (truncatedHash%1000)/100;
  dig2 = (truncatedHash%100)/10;
  dig1 = (truncatedHash%10);
  TOTP = String(dig6) + String(dig5) + String(dig4) +
         String(dig3) + String(dig2) + String(dig1);

  return TOTP;
}

void setupTime () {
  // Sets the RTC to the date & time of compilation
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // Sets the RTC manually
  //RTC.adjust(DateTime(2019, 4, 10, 21, 0, 0));
}

void displayTime (String LCD_dow, String LCD_date, String LCD_hour, String LCD_min) {
  LCD.clrScr();
  LCD.setFont(BigNumbers);
  LCD.print(LCD_hour,7,14);
  LCD.print(LCD_min,49,14);
  LCD.print(".",36,10);
  LCD.print(".",36,0);
  LCD.setFont(SmallFont);
  LCD.print(LCD_dow,LEFT,0);
  LCD.print(LCD_date,RIGHT,0);
  LCD.update();
}

void showDuration (int timeElapsed) {
  // Define the bar's thickness
  int y = 47 - BARTH;
  LCD.drawRect(0,y,83,47);
  // Remove any lines in the screen buffer
  for (int x = 1; x <= 82; x++)
    LCD.clrLine(x,y + 1,x,47);
  // Draw the lines
  for (int x = 1; x <= (82*(timeStep - timeElapsed - 1))/
      (timeStep-1); x++)
  {
    LCD.drawLine(x,y + 1,x,47);
    if (x == 81)
      LCD.drawLine(x + 1,y + 1,x + 1,47);
  }
  LCD.update();
}

void intTamperSetup(void) {
  *digitalPinToPCMSK(TRIG_TAMPER) |= bit(digitalPinToPCMSKbit(TRIG_TAMPER));
  PCIFR |= bit(digitalPinToPCICRbit(TRIG_TAMPER));  // clear any outstanding interrupt
  PCICR |= bit(digitalPinToPCICRbit(TRIG_TAMPER));  // enable interrupt for the group
}

void goToSleep(void) {
  ADCSRA = 0;                    //disable the ADC
  power_twi_disable();
  power_timer0_disable();
  ACSR = (1 << ACD);             //disable the analog comparator
  EICRA = _BV(ISC01) || _BV(ISC00); //configure INT0 to trigger on rising edge
  EIMSK = _BV(INT0);             //enable INT0
  // turn off I2C pull-ups
  digitalWrite (A4, LOW);
  digitalWrite (A5, LOW);
  // turn off I2C
  TWCR &= ~(_BV(TWEN) | _BV(TWIE) | _BV(TWEA));
  power_all_disable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  //stop interrupts to ensure the BOD timed sequence executes as required
  cli();
  sleep_enable();
  //disable brown-out detection while sleeping (20-25µA)
  uint8_t mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);
  uint8_t mcucr2 = mcucr1 & ~_BV(BODSE);
  MCUCR = mcucr1;
  MCUCR = mcucr2;
  //sleep_bod_disable();           //for AVR-GCC 4.3.3 and later, this is equivalent to the previous 4 lines of code
  
  sei();                         //ensure interrupts enabled so we can wake up again
  sleep_cpu();                   //go to sleep
  sleep_disable();               //wake up here
  power_twi_enable();
  power_timer0_enable();
  // activate I2C and clock
  Wire.begin();
  RTC.begin();
  LCD.clrScr();
  sleep = false;
  mode = false;
}

//external interrupt 0 wakes the MCU
ISR(INT0_vect) {
  while (digitalRead(TRIG_SLEEP) == LOW) {
    EIMSK = 0;
  }
}

void initState(void) {
  LCD.clrScr();
  LCD.setFont(SmallFont);
  LCD.print("INIT STATE", CENTER, 10);
  LCD.update();
  while (digitalRead(TRIG_TAMPER) != HIGH || digitalRead(TRIG_SLEEP) != LOW || digitalRead(TRIG_SHOW) != LOW);
  while (digitalRead(TRIG_TAMPER) != HIGH || digitalRead(TRIG_SLEEP) != HIGH || digitalRead(TRIG_SHOW) != HIGH);
  detect = true;
  delay(2000);
  LCD.clrScr();
}

ISR(PCINT0_vect) {
  if (digitalRead(TRIG_TAMPER) == LOW && detect) {/*
    LCD.clrScr();
    LCD.enableSleep();*/
    for (int i = 0; i < 16; i++)
      EEPROM.update(i, 0);
    while(true);
  }
}
