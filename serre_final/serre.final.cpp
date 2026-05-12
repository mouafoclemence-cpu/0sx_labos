#include "serre_final.h"

// Définition des variables
const int input1 = 45;
const int input2 = 47;
LCD_I2C lcd(0x27, 16, 2);
const int DHT_11 = 10;
DHT dht(DHT_11, DHTTYPE);
float temperature = 0;
float humidite = 0;
const int PHOTORESISTANCE = A0;
int valeurPhotoresistance = 0;
int valeurMin = 1023;
int valeurMax = 0;
bool calibrationActive = false;
const int ECHO = 11;
const int TRIGGER = 12;
UltraSonicDistanceSensor distanceSensor(TRIGGER, ECHO);
int distance = 0;
const int LED = 9;
int pageActuelle = 1;
const int BOUTON = 4;
OneButton button(BOUTON, true, true);
const int JOYSTICKX = A1;
const int JOYSTICKY = A2;
const int boutonJoystick = 2;
OneButton joystickBtn(boutonJoystick, true, true);
AccelStepper myStepper(MOTOR_INTERFACE_TYPE, IN_1, IN_3, IN_2, IN_4);

// Variables globales
AppState currentState = DEMARRAGE;
IrrigationState etat_courant = FERMETURE;
EtatConvoyeur etatConvoyeur = INACTIF;
const int POSITIONOUVERTE = 2038;
const int POSITIONFERMER = 0;
int pourcentage = 0;
int vitesseActuelle = 0;
int vitesseConstante = 0;

U8G2_MAX7219_8X8_F_4W_SW_SPI u8g2(U8G2_R0, CLK_PIN, DIN_PIN, CS_PIN, U8X8_PIN_NONE, U8X8_PIN_NONE);

// ========== VARIABLES POUR L'AFFICHAGE ==========
String ligne1Affichage = "";
String ligne2Affichage = "";
int refreshRateAffichage = 250;
unsigned long dernierRefreshAffichage = 0;

// Création de l'objet Irrigation
Irrigation irrigation(LED, IN_1, IN_3, IN_2, IN_4);

// ========== FONCTIONS D'AFFICHAGE ==========

void setLine1(String line1) {
  ligne1Affichage = line1;
}

void setLine2(String line2) {
  ligne2Affichage = line2;
}

void updateAffichage() {
  unsigned long tempsActuel = millis();
  
  if (tempsActuel - dernierRefreshAffichage >= refreshRateAffichage) {
    dernierRefreshAffichage = tempsActuel;
    
    lcd.clear();
    
    lcd.setCursor(0, 0);
    String ligne1 = ligne1Affichage;
    ligne1 = ligne1.substring(0, 16);
    lcd.print(ligne1);
    
    lcd.setCursor(0, 1);
    String ligne2 = ligne2Affichage;
    ligne2 = ligne2.substring(0, 16);
    lcd.print(ligne2);
  }
}

void clearAffichage() {
  lcd.clear();
  ligne1Affichage = "";
  ligne2Affichage = "";
}

void setRefreshRate(int refreshRate) {
  refreshRateAffichage = refreshRate;
}

int getRefreshRate() {
  return refreshRateAffichage;
}

// ========== IMPLEMENTATION DE LA CLASSE IRRIGATION ==========

Irrigation::Irrigation(int ledPin, int pin1, int pin2, int pin3, int pin4) {
  _ledPin = ledPin;
  _pin1 = pin1;
  _pin2 = pin2;
  _pin3 = pin3;
  _pin4 = pin4;
  _positionFermee = 0;
  _positionOuverte = 2038;
  _etatActuel = FERMETURE;
  _estEnMouvement = false;
  _distanceRef = nullptr;
  _clickFlagRef = nullptr;
}

void Irrigation::demarrerOuverture() {
  myStepper.moveTo(_positionOuverte);
  _etatActuel = OUVERTURE;
  _estEnMouvement = true;
}

void Irrigation::demarrerFermeture() {
  myStepper.moveTo(_positionFermee);
  _etatActuel = FERMETURE;
  _estEnMouvement = true;
}

void Irrigation::arreter() {
  myStepper.stop();
  myStepper.moveTo(myStepper.currentPosition());
  _etatActuel = ARRET;
  _estEnMouvement = false;
}

void Irrigation::update() {
  int niveauMin = 20;
  int niveauMax = 25;
  
  switch (_etatActuel) {
    case FERME:
      if (_distanceRef != nullptr && *_distanceRef > 0 && *_distanceRef < niveauMin) {
        demarrerOuverture();
      }
      break;
      
    case OUVERTURE:
    if (_distanceRef != nullptr && *_distanceRef >= niveauMax) {                 // La distance a augmenté, on doit refermer
        
        demarrerFermeture();
      }
      else if(myStepper.distanceToGo() == 0) {
        _etatActuel = OUVERT;
        _estEnMouvement = false;
      }
      break;
      
    case OUVERT:
      if (_distanceRef != nullptr && *_distanceRef >= niveauMax) {
        demarrerFermeture();
      }
      break;
      
    case FERMETURE:
     if (_distanceRef != nullptr && *_distanceRef > 0 && *_distanceRef < niveauMin) {
        // La distance a diminué, on doit rouvrir
        demarrerOuverture();
      }
      else if (myStepper.distanceToGo() == 0) {
        _etatActuel = FERME;
        _estEnMouvement = false;
      }
      break;
      
    case ARRET:
      break;
  }
  
  if (_etatActuel != ARRET && _etatActuel != FERME && _etatActuel != OUVERT) {
    myStepper.run();
  }
  
  if (isMoving()) {
    static unsigned long dernierTps = 0;
    unsigned long tempsActuel = millis();
    const unsigned long delaiLed = 100;
    static bool etatLed = false;
    
    if ((tempsActuel - dernierTps) >= delaiLed) {
      dernierTps = tempsActuel;
      etatLed = !etatLed;
      digitalWrite(_ledPin, etatLed);
    }
  } else {
    int valeurbasse = 30;
    int valeurAlum = lireLuminosite();
    if (valeurAlum <= valeurbasse) {
      digitalWrite(_ledPin, HIGH);
    } else {
      digitalWrite(_ledPin, LOW);
    }
  }
}

// ========== FONCTIONS D'AFFICHAGE MATRICE ==========

void configurerAffichage() {
  u8g2.begin();
  u8g2.setContrast(5);
  clearAffichageMatrice(); 
}

void clearAffichageMatrice() {
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

void afficherInactif() {
  u8g2.clearBuffer();
  u8g2.drawLine(0, 0, 7, 7);
  u8g2.drawLine(7, 0, 0, 7);
  u8g2.sendBuffer();
}

void afficherActif() {
  u8g2.clearBuffer();
  u8g2.drawBox(1, 0, 2, 5);
  u8g2.drawBox(1, 6, 2, 2);
  u8g2.drawBox(5, 0, 2, 5);
  u8g2.drawBox(5, 6, 2, 2);
  u8g2.sendBuffer();
}

const uint8_t flecheDroite[] PROGMEM = {
  B00001000, B00001100, B00001110, B11111111,
  B11111111, B00001110, B00001100, B00001000,
};

void afficherAvancer() {
  u8g2.clearBuffer();
  u8g2.drawXBMP(0, 0, 8, 8, flecheDroite);
  u8g2.sendBuffer();
}

const uint8_t flecheGauche[] PROGMEM = {
  B00010000, B00110000, B01110000, B11111111,
  B11111111, B01110000, B00110000, B00010000,
};

void afficherReculer() {
  u8g2.clearBuffer();
  u8g2.drawXBMP(0, 0, 8, 8, flecheGauche);
  u8g2.sendBuffer();
}

void mettreAJourAffichageMatrice() {
  switch (etatConvoyeur) {
    case INACTIF:
      afficherInactif();
      break;
    case ACTIF:
      if (vitesseActuelle > 5) {
        afficherAvancer();
      } else if (vitesseActuelle < -5) {
        afficherReculer();
      } else {
        afficherActif();
      }
      break;
    case CONSTANT:
      if (vitesseConstante > 10) {
        afficherAvancer();
      } else if (vitesseConstante < -10) {
        afficherReculer();
      }
      break;
  }
}

// ========== FONCTIONS DU CONVOYEUR ==========

void controlerMoteur() {
  int vitesseMoteur = 0;
  if (etatConvoyeur == INACTIF) {
    vitesseMoteur = 0;
  } else if (etatConvoyeur == ACTIF) {
    vitesseMoteur = vitesseActuelle;
  } else if (etatConvoyeur == CONSTANT) {
    vitesseMoteur = vitesseConstante;
  }

  int pwmValue = map(abs(vitesseMoteur), 0, 100, 0, 255);

  if (vitesseMoteur > 5) {
    analogWrite(input1, pwmValue);
    analogWrite(input2, 0);
  } else if (vitesseMoteur < -10) {
    analogWrite(input1, 0);
    analogWrite(input2, pwmValue);
  } else {
    analogWrite(input1, 0);
    analogWrite(input2, 0);
  }
}

void lireJoystick() {
  int lectureX = analogRead(JOYSTICKX);
  vitesseActuelle = map(lectureX, 0, 1023, -100, 100);
  if (abs(vitesseActuelle) < 5) {
    vitesseActuelle = 0;
  }
  if (vitesseActuelle > 100) vitesseActuelle = 100;
  if (vitesseActuelle < -100) vitesseActuelle = -100;
}

void onJoystickClick() {
  if (etatConvoyeur == ACTIF) {
    vitesseConstante = vitesseActuelle;
    etatConvoyeur = CONSTANT;
    mettreAJourAffichageMatrice();
  } else if (etatConvoyeur == CONSTANT) {
    etatConvoyeur = ACTIF;
    vitesseActuelle = vitesseConstante;
  }
}

void onJoystickLongClick() {
  if (etatConvoyeur == INACTIF) {
    etatConvoyeur = ACTIF;
    vitesseActuelle = 0;
    vitesseConstante = 0;
    mettreAJourAffichageMatrice();
  } else {
    etatConvoyeur = INACTIF;
    vitesseActuelle = 0;
    vitesseConstante = 0;
    mettreAJourAffichageMatrice();
  }
  controlerMoteur();
}

void gestionConvoyeur() {
  if (etatConvoyeur == ACTIF) {
    lireJoystick();
    controlerMoteur();
    mettreAJourAffichageMatrice();
  } else if (etatConvoyeur == CONSTANT) {
    controlerMoteur();
    mettreAJourAffichageMatrice();
  } else {
    controlerMoteur();
  }
}

// ========== FONCTIONS D'IRRIGATION ==========

void sousTention() {
  irrigation.demarrerFermeture();
  etat_courant = FERMETURE;
}

void position() {
  pourcentage = irrigation.getPositionPct();
}

void clignotementLed(unsigned long tempsActuel) {
  // Géré par la classe Irrigation
}

void gestionIrrigation(unsigned long tempsActuel) {
  irrigation.update();
  etat_courant = (IrrigationState)irrigation.getCurrentState();
}

// ========== FONCTIONS DE LECTURE CAPTEURS ==========

void lirePhotoresistance(unsigned long tempsActuel) {
  const unsigned long delaisPhotoresistance = 1000;
  static unsigned long dernierMoment = 0;
  if ((tempsActuel - dernierMoment) >= delaisPhotoresistance) {
    dernierMoment = tempsActuel;
    valeurPhotoresistance = analogRead(PHOTORESISTANCE);
    if (valeurPhotoresistance < 0) valeurPhotoresistance = 0;
    if (valeurPhotoresistance > 1023) valeurPhotoresistance = 1023;
  }
}

void lireTemperatureHumidite(unsigned long now) {
  static unsigned long lastTime = 0;
  const unsigned long rate = 5000;
  if (now - lastTime >= rate) {
    lastTime = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (isnan(h) || isnan(t)) {
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
  int allumage = map(valeurPhotoresistance, valeurMin, valeurMax, 0, 100);
  if (allumage < 0) allumage = 0;
  if (allumage > 100) allumage = 100;
  return allumage;
}

void allumeEteindLed() {
  // Géré par la classe Irrigation
}

void lireLaDistance(unsigned long tempsActuel) {
  unsigned long delaisDistance = 250;
  static unsigned long dernierTemps = 0;
  if ((tempsActuel - dernierTemps) >= delaisDistance) {
    dernierTemps = tempsActuel;
    distance = distanceSensor.measureDistanceCm();
    if (distance < 0) distance = 0;
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
    Serial.print(pourcentage);
    Serial.print(",Conv:");
    Serial.println(vitesseActuelle);
    dernierTemps = millis();
  }
}

// ========== FONCTIONS LCD ==========


void vanneLCD() {
  //gardée pour compatibilité mais n'est plus utilisée
}

// NOUVELLE FONCTION pour préparer le texte
void preparerTexteAffichage() {
  String ligne1 = "";
  String ligne2 = "";
  
  // N'afficher la vanne que si l'irrigation est en mouvement OU si on est en mode arrêt manuel
  if (irrigation.isMoving() || etat_courant == ARRET) {
    ligne1 = "Vanne:" + String(pourcentage) + "%";
    
    switch(etat_courant) {
      case FERME: ligne2 = "Etat:FERME"; break;
      case OUVERTURE: ligne2 = "Etat:OUVERTURE"; break;
      case OUVERT: ligne2 = "Etat:OUVERT"; break;
      case FERMETURE: ligne2 = "Etat:FERMETURE"; break;
      case ARRET: ligne2 = "Etat:ARRET"; break;
      default: ligne2 = "Etat:?"; break;
    }
  }
  else {
    // Affichage normal des différents états
    switch (currentState) {
      case DEMARRAGE:
        ligne1 = "Systeme pret";
        ligne2 = "Appuyez btn";
        break;
        
      case LUM_DIST:
        {
          int valeurAlum = lireLuminosite();
          ligne1 = "Lum:" + String(valeurAlum) + "%";
          ligne2 = "Dist:" + String(distance) + "cm";
          if(distance < 10 && distance > 0) {
            ligne2 = "TROP PRES!";
          }
        }
        break;
        
      case DHT_STATE:
        ligne1 = "Temp:" + String(temperature, 1) + "C";
        ligne2 = "Hum:" + String(humidite, 0) + "%";
        break;
        
      case CALIBRATION:
        ligne1 = "LUM Min:" + String(valeurMin);
        ligne2 = "LUM Max:" + String(valeurMax);
        break;
        
      default:
        ligne1 = "Serre OK";
        ligne2 = "";
        break;
    }
  }
  
  setLine1(ligne1);
  setLine2(ligne2);
}

// FONCTION afficherLCD corrigée
void afficherLCD(unsigned long tempsActuel) {
  preparerTexteAffichage();
  updateAffichage();
}

void onClick() {
  if (etat_courant == ARRET) {
    irrigation.demarrerOuverture();
    vanneLCD();
    return;
  }
  if (myStepper.isRunning()) {
    irrigation.arreter();
    return;
  } else {
    switch (currentState) {
      case DEMARRAGE: currentState = LUM_DIST; break;
      case LUM_DIST: currentState = DHT_STATE; break;
      case DHT_STATE: currentState = LUM_DIST; break;
      case CALIBRATION: currentState = LUM_DIST; break;
    }
  }
}

void onDoubleClick() {
  if (currentState == DEMARRAGE || currentState == LUM_DIST || currentState == DHT_STATE) {
    currentState = CALIBRATION;
  }
}