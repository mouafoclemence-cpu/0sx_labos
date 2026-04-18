#include <OneButton.h>
#include "DHT.h"  //librairie du capteur de temperature et d'humidité
#include <LCD_I2C.h>
#include <HCSR04.h>
#include <AccelStepper.h>  //librairie moteur pas à pas

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

int pageActuelle = 1;

//------BOUTON------
const int BOUTON = 4;
OneButton button(BOUTON, true, true);

//Etat du system----capteur d'environnement.
enum AppState { DEMARRAGE,
                DHT_STATE,
                LUM_DIST,
                CALIBRATION };
AppState currentState = DEMARRAGE;

//----------moteur à pas------
#define MOTOR_INTERFACE_TYPE 4  //type de moteur

#define IN_1 31
#define IN_2 33
#define IN_3 35
#define IN_4 37
AccelStepper myStepper(MOTOR_INTERFACE_TYPE, IN_1, IN_3, IN_2, IN_4);  //creation de l'objet moteur pas pas.

const int POSITIONOUVERTE = 2038;
const int POSITIONFERMER = 0;
int pourcentage = 0;

//Etat du system----irrigation.
enum EtatIrrigation { FERME,
                      OUVERTURE,
                      OUVERT,
                      FERMETURE,
                      ARRET };
EtatIrrigation etat_courant = FERMETURE;

//fermeture une fois alimenté.
void sousTention() {
  myStepper.moveTo(POSITIONFERMER);
  etat_courant = FERMETURE;
}

// niveau trop bas → ouvrir la vanne
void etatFermer() {
  int niveauMin = 20;  // Seuil d'ouverture (niveau trop bas)
  if (distance > 0 && distance < niveauMin) {
    myStepper.moveTo(POSITIONOUVERTE);
    etat_courant = OUVERTURE;
  }
}

// ouverture terminée
void etatOUverture() {
  if (myStepper.distanceToGo() == 0) {
    etat_courant = OUVERT;
  }
}

// niveau suffisant → fermer la vanne
void etatOuvert() {
  int niveauMax = 25;  // Seuil de fermeture (niveau suffisant)
  if (distance >= niveauMax) {
    myStepper.moveTo(POSITIONFERMER);
    etat_courant = FERMETURE;
  }
}

// fermeture terminée
void etatFermeture() {
  if (myStepper.distanceToGo() == 0) {
    etat_courant = FERME;
  }
}

// Arrêt 🛑
void etatArret() {
  myStepper.stop();                               // arrête le moteur
  myStepper.moveTo(myStepper.currentPosition());  // annule la cible
  etat_courant = ARRET;                           // bloque l'irrigation
}

// mapper la positio entre 0-100
void position() {
  int nbreDePas = myStepper.currentPosition();
  int pourcentageMin = 0;
  int pourcentageMax = 100;
  pourcentage = map(nbreDePas, POSITIONFERMER, POSITIONOUVERTE, pourcentageMin, pourcentageMax);
  if (pourcentage < 0) {
    pourcentage = 0;
  }
  if (pourcentage > 100) {
    pourcentage = 100;
  }
}

//faire clignoter la LED
void clignotementLed(unsigned long tempsActuel) {
  const unsigned long delaiLed = 100;
  static bool etatLed = false;
  static unsigned long dernierTps = 0;
  if (etat_courant == OUVERTURE || etat_courant == FERMETURE) {
    if ((tempsActuel - dernierTps) >= delaiLed) {
      dernierTps = tempsActuel;
      etatLed = !etatLed;
      digitalWrite(LED, etatLed);
    }
  } else {
    allumeEteindLed();
  }
}

// SYSTEME d'irrigation
void gestionIrrigation(unsigned long tempsActuel) {
  switch (etat_courant) {
    case FERME:
      etatFermer();
      break;
    case OUVERTURE:
      etatOUverture();
      break;
    case OUVERT:
      etatOuvert();
      break;
    case FERMETURE:
      etatFermeture();
      break;
    case ARRET:
      etatArret();
      break;
  }
  if (etat_courant != ARRET) {
    myStepper.run();
  }
}

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
    if (isnan(h) || isnan(t)) {       // valide si c'est un nombre.
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
    Serial.print(humidite);
    Serial.print(",van:");
    Serial.println(pourcentage);

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

void vanneLCD() {
  lcd.setCursor(0, 0);
  lcd.print("Vanne : ");
  lcd.print(pourcentage);
  lcd.print(" %");

  lcd.setCursor(0, 1);
  lcd.print("Etat : ");
  switch (etat_courant) {
    case FERME:
      lcd.print("FERME");
      break;
    case OUVERTURE:
      lcd.print("OUVERTURE");
      break;
    case OUVERT:
      lcd.print("OUVERT");
      break;
    case FERMETURE:
      lcd.print("FERMETURE");
      break;
    case ARRET:
      lcd.print("ARRET");
      break;
  }
  lcd.print("   ");
}

void afficherLCD(unsigned long tempsActuel) {
  unsigned long delaisRafraichissement = 250;
  static unsigned long dernierTemps = 0;
  int valeurAlum = lireLuminosite();
  if ((tempsActuel - dernierTemps) >= delaisRafraichissement) {
    lcd.clear();
    if (etat_courant == OUVERTURE || etat_courant == FERMETURE || etat_courant == ARRET) {
      vanneLCD();
    } else {

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
    }
    dernierTemps = millis();
  }
}

// Fonction appelée lors d'un simple clic
void onClick() {
  if (etat_courant == ARRET) {  // Si en état d'arrêt d'urgence, ouvre
    myStepper.moveTo(POSITIONOUVERTE);
    etat_courant = OUVERTURE;
    vanneLCD();
    return;
  }
  if (myStepper.isRunning()) {  // Si mouvement arrêt d'urgence,
    etatArret();
    return;
  } else {
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

  myStepper.setMaxSpeed(1000);                    // Vitesse maximale (steps/seconde)
  myStepper.setSpeed(2000);                       // Vitesse de déplacement
  myStepper.setCurrentPosition(POSITIONOUVERTE);  //reinitialise la position actuelle du moteur
  myStepper.setAcceleration(100);                 //demarrage et arret progressif
  sousTention();
}


void loop() {
  unsigned long tempsActuel = millis();
  lirePhotoresistance(tempsActuel);
  lireTemperatureHumidite(tempsActuel);
  lireLaDistance(tempsActuel);

  calibrage();


  gestionIrrigation(tempsActuel);
  position();
  clignotementLed(tempsActuel);

  afficherLCD(tempsActuel);

  portserie(tempsActuel);

  button.tick();
}