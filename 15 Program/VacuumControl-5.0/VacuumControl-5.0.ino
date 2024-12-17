// Vacuum controller, origin Tryggve Kallhovd 2016.
// Vacuum Controller, modification ZINK Thierry 2024.


#include <EEPROM.h>               // Libray EEPROM: https://docs.arduino.cc/learn/built-in-libraries/eeprom/#read
#include <Encoder.h>              // Encoder Library: https://www.pjrc.com/teensy/td_libs_Encoder.html
#include <LiquidCrystal_I2C.h>    // Ecran LCD: https://github.com/johnrickman/LiquidCrystal_I2C

// LCD definition

#define I2C_ADDR 0x27                   //Lcd i2c addresse 0x27
LiquidCrystal_I2C lcd(I2C_ADDR,16,2);   //Instance Afficheur LCD, i2c addresse 0x27, 16 caracères sur 2 lignes

// Pin definitions:

Encoder thrEnc(2, 3);   // Instance encodeur port 2 (interrupt) et 3 (interrupt)
int encButPin = 5;      // Click encodeur central
int relayPin = 11;      // Pin command du relay
int vacSensPin = A0;    // Pin sensor MPX5100_DP


// Variables definitions:

int adrrconsigne = 2;   //Adresse EEPROM Valeur Règlage 
int addrhyste = 4;      //Adresse EEPROM Valeur Marge
int valeurconsigne;     //Valeur de consigne
int valeurhyste;        //Valeur de hysteresise

boolean encBut = 1;
boolean oldBut = 1;
boolean encSel = 1;

int encPos1;
int oldPos1;
int encPos2;
int oldPos2;

boolean vacSwitch;

int vacuum;
int vacVal;
int oldVal;

// Debug

boolean debug = 1;              // Mode Debug Valeur à 1 sinon 0  

// Fonctions definitions:

void writeIntIntoEEPROM(int address, int number)  // ecriture en memoire dun nombre number dans l'adresse address
{ 
  byte byte1 = number >> 8;           // Extraire l'octet de poids fort (octet le plus significatif)
  byte byte2 = number & 0xFF;         // Extraire l'octet de poids faible (octet le moins significatif)
  EEPROM.write(address, byte1);       // Écrire l'octet de poids fort dans l'EEPROM
  EEPROM.write(address + 1, byte2);   // Écrire l'octet de poids faible à l'adresse suivante
}

int readIntFromEEPROM(int address)                // lecture en memoire dun nombre number dans l'adresse address
{
  byte byte1 = EEPROM.read(address);      // Lire l'octet de poids fort
  byte byte2 = EEPROM.read(address + 1);  // Lire l'octet de poids faible
  return (byte1 << 8) | byte2;            // Combiner les deux octets pour reformer l'entier
}

void setup() {

  Serial.begin(9600);

  lcd.init();                     // initialize the lcd
  lcd.backlight();                // Allume le back light
  lcd.setCursor(0, 0);            // Position Ecriture colone 0 ligne 0
  lcd.print("Reglage:   -");      // Ecriture 
  lcd.setCursor(0, 1);            // Position Ecriture colone 0 ligne 0
  lcd.print("Mesure:");           // Ecriture
  lcd.setCursor(13, 1);           // Position Ecriture colone 13 ligne 1
  lcd.print("mBr");               // Ecriture

  //Initialisation par lecture des valeurs dans l'EEPROM
  
  valeurconsigne = readIntFromEEPROM(adrrconsigne); // Lecture de la valeur en mémoire dans l'adresse 1 du reglage
  valeurhyste = readIntFromEEPROM(addrhyste);       // Lecture de la valeur en mémoire dans l'adresse 2 de l'hysteresis
  encPos1 = (valeurconsigne* 4);
  encPos2 = (valeurhyste * 4);
  thrEnc.write(encPos1);

  // Initialisation des port
  
    // Configuration du bouton avec pull-up interne
  pinMode(encButPin, INPUT);
  digitalWrite(encButPin, HIGH);

    // Configuration du relais en sortie, désactivé au départ
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

    // Configuration de la broche du capteur en entrée
  pinMode(vacSensPin, INPUT);
  
}

void loop() {

  // Analyse le Bouton de l'encodeur:
  // change entre "Règlage" (la consigne) et "Marge" (l'hystérésis) à chaque appui
  // encSel = 1 affiche la consigne
  // encSel = 0 affiche l'hystérésis
  // Sur l'afficheur en ligne 0 affichage de la consigne et d l'hystérésis alternativement
  // La valeur affichée peut être ajustée avec l'encodeur
  // Sur l'afficheur en ligne 1 affichage de la valeur mesurée

  encBut = digitalRead(encButPin);
  if (encBut != oldBut) {         // Vérifie s'il y a un changement d'état
    if (encBut == 0) {
      encSel = !encSel;           // Bascule la valeur de encSel
    }
    if (encSel == 1) {            // Affiche la consigne
      thrEnc.write(encPos1);
      lcd.setCursor(0, 0);
      lcd.print("Reglage:   -");

    } else if (encSel == 0) {     //Affiche l'hystérésis
      thrEnc.write(encPos2);
      lcd.setCursor(0, 0);
      lcd.print("Marge:  +/- ");
      lcd.setCursor(14, 0);
      lcd.print(" ");
    }
    oldBut = encBut;              // Met à jour l'état précédent du bouton
  }

  // Lecture de l'encodeur:

  if (encSel) {
    encPos1 = thrEnc.read();
    if (encPos1 != oldPos1) {
      oldPos1 = encPos1;
    }
    
    // Si l'encodeur a été tourné et encSel = 1 (Régage de la consigne)
    // Test des valeurs limites de la consigne qui doit être >= 10 et <= 99
    // Si en dehors des limites la consigne est mis à la limite correspondante
    if (encPos1 >= (99 * 4)) {
      encPos1 = (99 * 4);
      thrEnc.write(99 * 4);
    } else if (encPos1 <= (10 * 4)) {
      encPos1 = (10 * 4);
      thrEnc.write(10 * 4);
    }
    
    // La consigne est mise en mémoire dans l'EEPROM et affichée
    int val = ((encPos1) / 4);
    writeIntIntoEEPROM(adrrconsigne, val);
    lcd.setCursor(12, 0);
    lcd.print((encPos1 / 4) * 10);
  }
  else  {
    encPos2 = thrEnc.read();
    if (encPos2 != oldPos2) {
      oldPos2 = encPos2;
    }

    // Si l'encodeur a été tourné et encSel = 0 (Régage de l'hystérésis)
    // Test des valeurs limites de l'hystérésis qui doit être >= 5 et <= 50
    // Si en dehors des limites l'hystérésis est mise à la limite correspondante  
    if (encPos2 >= (50 * 4)) {
      encPos2 = (50 * 4);
      thrEnc.write(50 * 4);
    } else if (encPos2 <= (5 * 4)) {
      encPos2 = (5 * 4);
      thrEnc.write(5 * 4);
    }
    
    // La valeur de l'hystérésis est mise en mémoire dans l'EEPROM et affichée
    int val = ((encPos2) / 4);
    writeIntIntoEEPROM(addrhyste, val);
    lcd.setCursor(12, 0);
    lcd.print(encPos2 / 4);
    if ((encPos2 / 4) < 10) {
      lcd.setCursor(13, 0);
      lcd.print(" ");
    }
  }

  // Contrôle du relai en fonction de la valeur de vide mesurée, de la consigne "encPos1" et de l'hytérésis "encPos2":

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

  // mesure de la valeur avec le capteur MPX5100DP

  vacuum = 0;
  
  // mesure de 10 valeurs du vide
  for (byte i = 0; i < 10; i++) {
    vacuum += analogRead(vacSensPin);
  }
  vacuum /= 10;
  
  // Mise à l'échelle réelle
  vacuum = map(vacuum, 41, 962, 0, 1000);

  // Moyennage avec la valeur précédent
  vacVal = (vacuum + oldVal) / 2;
  oldVal = vacVal;
  if (vacVal < 1) {
    vacVal = 0;
  }
  
  // Affichage de la nouvelle valeur

  lcd.setCursor(9, 1);
  lcd.print(vacVal);
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

  // Debug

  if (debug=1) {
    Serial.print("Sensor:");
    Serial.print(vacVal);
    Serial.print("_");
    Serial.print("Consigne Enc:");
    Serial.print(encPos1);
    Serial.print("_");
    Serial.print("Consigne Hyst:");
    Serial.print(encPos2);
    Serial.print("_");
    Serial.print("Etat Relay:");
    Serial.print(vacSwitch);
    Serial.print("_");
    Serial.print("EEPROM Consigne:");
    Serial.print(readIntFromEEPROM(adrrconsigne));
    Serial.print("_");
    Serial.print(EEPROM.read(2));
    Serial.print("/");
    Serial.print(EEPROM.read(3));
    Serial.print("_");
    Serial.print("EEPROM Hyste:");
    Serial.print(readIntFromEEPROM(addrhyste));
    Serial.print("_");
    Serial.print(EEPROM.read(4));
    Serial.print("/");
    Serial.println(EEPROM.read(5));
  }
}
