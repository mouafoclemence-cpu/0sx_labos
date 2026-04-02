#include <OneButton.h>
#include "DHT.h"  //librairie du capteur de temperature et d'humidité
#include <LCD_I2C.h>
#include <HCSR04.h>

//-----LCD------
LCD_I2C lcd(0x27, 16, 2);

//------DHT11-----
#define DHTTYPE DHT11  // type de capteur d'humidité et de temperature
const int DHT_11 = 7;
DHT dht(DHT_11, DHTTYPE);  //creation de l'objet DHT
float temperature = 0;
float humidite = 0;

//----Photoresistance-------
const int PHOTORESISTANCE = A0;
int valeurPhotoresistance = 0;
int valeurMin = 1023;
int valeurMax = 0;
bool calibrationActive = false;

//----HC-SR04------------
const int ECHO = 11;
const int TRIGGER = 12;
UltraSonicDistanceSensor distanceSensor(TRIGGER, ECHO);  //creaction d'objet pour le capteur a distance.
int distance = 0;

//-------LED--------
const int LED = 9;

//------BOUTON------
const int BOUTON = 4;
OneButton button(BOUTON, true, true);

//Etat du system----
enum AppState { DEMARRAGE,
                DHT_STATE,
                LUM_DIST,
                CALIBRATION };
AppState currentState = DEMARRAGE;


int pageActuelle = 1;


void lirePhotoresistance(unsigned long tempsActuel) {
  const unsigned long delaisPhotoresistance = 1000;
  static unsigned long dernierMoment = 0;

  if ((tempsActuel - dernierMoment) >= delaisPhotoresistance) {
    dernierMoment = tempsActuel;
    valeurPhotoresistance = analogRead(PHOTORESISTANCE);
    if (valeurPhotoresistance < 0) {
      valeurPhotoresistance = 0;
    }
    if (valeurPhotoresistance > 1023) {  //pour éviter les valeurs hors plage
      valeurPhotoresistance = 1023;
    }
  }
}

void lireTemperatureHumidite(unsigned long now) {
  static unsigned long lastTime = 0;
  const unsigned long rate = 5000;
  if (now - lastTime >= rate) {
    lastTime = now;
    float t = dht.readTemperature();  // Température
   float h = dht.readHumidity();     // Humidité aussi !
    if (isnan(h) || isnan(t)) {     // valide si c'est un nombre.
      Serial.println(F("Echec de lecture du DHT!"));
    }
    temperature = t;
    humidite = h;
  }
}

void calibrage() {
  if (currentState == CALIBRATION && !calibrationActive) {
    if (valeurPhotoresistance < valeurMin) {
      valeurMin = valeurPhotoresistance;
    }
    if (valeurPhotoresistance > valeurMax) {
      valeurMax = valeurPhotoresistance;
    }
  }
}

int lireLuminosite() {
  int allumage = map(valeurPhotoresistance, valeurMin, valeurMax, 0, 100);  // lumiosite en pourcentage.
  if (allumage < 0) {
    allumage = 0;
  }
  if (allumage > 100) {
    allumage = 100;
  }
  return allumage;
}

void allumeEteindLed() {
  int valeurbasse = 30;
  int valeurAlum = lireLuminosite();

  if (valeurAlum <= valeurbasse) {
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }
}


void lireLaDistance(unsigned long tempsActuel) {
  unsigned long delaisDistance = 250;

  static unsigned long dernierTemps = 0;
  if ((tempsActuel - dernierTemps) >= delaisDistance) {  // conserve la voie
    dernierTemps = tempsActuel;
    distance = distanceSensor.measureDistanceCm();
    if (distance < 0) {
      distance = 0;
    }
  }
}

void portserie(unsigned long tempsActuel) {
  static unsigned long dernierTemps = 0;
  unsigned long delaisDistance = 3000;
  if ((tempsActuel - dernierTemps) >= delaisDistance) {
    int valeurAlum = lireLuminosite();
    Serial.print("Lum:");
    Serial.print(valeurAlum);
    Serial.print(",Min:");
    Serial.print(valeurMin);
    Serial.print(",Max:");
    Serial.print(valeurMax);
    Serial.print(",Dis:");
    Serial.print(distance);
    Serial.print(",T:");
    Serial.print(temperature);
    Serial.print(",H:");
    Serial.println(humidite);
    dernierTemps = millis();
  }
}

void lumDist() {
  int valeurAlum = lireLuminosite();
  lcd.setCursor(0, 0);
  lcd.print("Lum: ");
  lcd.print(valeurAlum);
  lcd.print("% ");

  lcd.setCursor(0, 1);
  lcd.print("Dist: ");
  lcd.print(distance);
  lcd.print(" cm");
  lcd.print("   ");
}

void tempHum() {
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("c ");

  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(humidite);
  lcd.print(" %");
  lcd.print("   ");
}

void MinMaxLCD() {
  lcd.setCursor(0, 0);
  lcd.print("Lum Min: ");
  lcd.print(valeurMin);

  lcd.setCursor(0, 1);
  lcd.print("Lum Max: ");
  lcd.print(valeurMax);
  lcd.print(" %");
  lcd.print("   ");
}


void afficherLCD(unsigned long tempsActuel) {
  unsigned long delaisRafraichissement = 250;
  static unsigned long dernierTemps = 0;
  int valeurAlum = lireLuminosite();
  if ((tempsActuel - dernierTemps) >= delaisRafraichissement) {
    lcd.clear();
    switch (currentState) {
      case DEMARRAGE:
        lcd.setCursor(0, 0);
        lcd.print("Systeme pret   ");
        lcd.setCursor(0, 1);
        lcd.print("Appuyez btn    ");
        lcd.print(" ");

        break;

      case LUM_DIST:
        if (pageActuelle == 1) {
          lumDist();
        }
        break;
      case DHT_STATE:
        if (pageActuelle == 1) {
          tempHum();
        }
        break;

      case CALIBRATION:
        MinMaxLCD();
        break;
    }
    dernierTemps = millis();
  }
}

// Fonction appelée lors d'un simple clic
void onClick() {
  switch (currentState) {
    case DEMARRAGE:
      currentState = LUM_DIST;
      
      break;

    case LUM_DIST:
      currentState = DHT_STATE;
      break;

    case DHT_STATE:
      currentState = LUM_DIST;
      break;

    case CALIBRATION:
      currentState = LUM_DIST;
      break;
  }
}

//Fonction appelée lors d'un double-clic

void onDoubleClick() {
  if (currentState == DEMARRAGE || currentState == LUM_DIST || currentState == DHT_STATE) {
    currentState = CALIBRATION;

  }
}



void setup() {
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(BOUTON, INPUT_PULLUP);
  lcd.begin();
  lcd.backlight();  // Allume le rétroéclairage
  Serial.begin(9600);
  dht.begin();  // initialisation du du capteur de temperature et d'humidité
  button.attachClick(onClick);
  button.attachDoubleClick(onDoubleClick);
}


void loop() {
  unsigned long tempsActuel = millis();
  lirePhotoresistance(tempsActuel);
  lireTemperatureHumidite(tempsActuel);
  calibrage();
  allumeEteindLed();
  lireLaDistance(tempsActuel);
  portserie(tempsActuel);
  afficherLCD(tempsActuel);
  button.tick();
}