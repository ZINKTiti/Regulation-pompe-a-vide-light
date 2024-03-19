
// Vacuum controller, Tryggve Kallhovd 2016.
// Encoder Library: https://www.pjrc.com/teensy/td_libs_Encoder.html

#include <EEPROM.h>
//#include <LiquidCrystal.h>

#include <Encoder.h>

#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR 0x27
#define BACKLIGHT_PIN 3
#define En_pin 2
#define Rw_pin 1
#define Rs_pin 0
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7

LiquidCrystal_I2C lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

// Pin definitions:

Encoder thrEnc(3, 4);
int encButPin = 5;

int relayPin = 11;

int vacSensPin = A0;


// Function definitions:

int addr1 = 0;
int addr2 = 1;
byte value1;
byte value2;

boolean encBut;
boolean oldBut;
boolean encSel = 1;

int encPos1;
int oldPos1;
int encPos2;
int oldPos2;

boolean vacSwitch;

int vacuum;
int vacVal;
int oldVal;


void setup() {

  Serial.begin(9600);

  lcd.begin (16,2);
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.setCursor(0, 1);
  //lcd.print("Vacuum:");
  lcd.print("Mesure:");
  lcd.setCursor(13, 1);
  lcd.print("mBr");

  value1 = EEPROM.read(0);
  value2 = EEPROM.read(1);
  encPos1 = (value1 * 4);
  encPos2 = (value2 * 4);

  pinMode(encButPin, INPUT);
  digitalWrite(encButPin, HIGH);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  pinMode(vacSensPin, INPUT);

  // initialize the lcd
  //lcd.init();

  // Print a message to the LCD.
  lcd.backlight();

}
void loop() {

  // Toggle encoder:

  encBut = digitalRead(encButPin);
  if (encBut != oldBut) {
    if (encBut == 0) {
      encSel = !encSel;
    }
    if (encSel == 1) {
      thrEnc.write(encPos1);
      lcd.setCursor(0, 0);
      //lcd.print("Threshold: -");
      lcd.print("Reglage:   -");

    } else if (encSel == 0) {
      thrEnc.write(encPos2);
      lcd.setCursor(0, 0);
      //lcd.print("Margin: +/- ");
      lcd.print("Marge:  +/- ");
      lcd.setCursor(14, 0);
      lcd.print(" ");
    }
    oldBut = encBut;
  }

  // Encoder:

  if (encSel) {
    encPos1 = thrEnc.read();
    if (encPos1 != oldPos1) {
      oldPos1 = encPos1;
    }
    if (encPos1 >= (99 * 4)) {
      encPos1 = (99 * 4);
      thrEnc.write(99 * 4);
    } else if (encPos1 <= (10 * 4)) {
      encPos1 = (10 * 4);
      thrEnc.write(10 * 4);
    }
    int val = ((encPos1) / 4);
    EEPROM.write(addr1, val);
    lcd.setCursor(12, 0);
    lcd.print((encPos1 / 4) * 10);
  }
  else  {
    encPos2 = thrEnc.read();
    if (encPos2 != oldPos2) {
      oldPos2 = encPos2;
    }
    if (encPos2 >= (50 * 4)) {
      encPos2 = (50 * 4);
      thrEnc.write(50 * 4);
    } else if (encPos2 <= (5 * 4)) {
      encPos2 = (5 * 4);
      thrEnc.write(5 * 4);
    }
    int val = ((encPos2) / 4);
    EEPROM.write(addr2, val);
    lcd.setCursor(12, 0);
    lcd.print(encPos2 / 4);
    if ((encPos2 / 4) < 10) {
      lcd.setCursor(13, 0);
      lcd.print(" ");
    }
  }

  // Vacuum switch:

  if (vacSwitch == 1) {
    if (vacuum >= ((encPos1 / 4) * 10) + (encPos2 / 4)) {
      vacSwitch = 0;
    }
  } else if (vacSwitch == 0) {
    if (vacuum < ((encPos1 / 4) * 10) - (encPos2 / 4)) {
      vacSwitch = 1;
    }
    digitalWrite(relayPin, vacSwitch);
  }

  // Vacuum sensor: (MPX5100DP)

  vacuum = 0;
  for (byte i = 0; i < 10; i++) {
    vacuum += analogRead(vacSensPin);
  }
  vacuum /= 10;
  vacuum = map(vacuum, 41, 962, 0, 1000);

  vacVal = (vacuum + oldVal) / 2;
  oldVal = vacVal;
  if (vacVal < 1) {
    vacVal = 0;
  }
  lcd.setCursor(9, 1);
  lcd.print(vacVal);
  Serial.println(vacVal);
  if (vacVal > 0) {
    lcd.setCursor(8, 1);
    lcd.print("-");
  } else {
    lcd.setCursor(8, 1);
    lcd.print(" ");
  }
  if (vacVal < 10) {
    lcd.setCursor(10, 1);
    lcd.print(" ");
  } else if (vacVal < 100) {
    lcd.setCursor(11, 1);
    lcd.print(" ");
  }
}
