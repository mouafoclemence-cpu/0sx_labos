#include "Convoyeur.h"


// Définition des constantes
// --- Moteur DC avec L293D ---
const int input1 = 45;
const int input2 = 47;

// --- LCD ---
LCD_I2C lcd(0x27, 16, 2);

// --- DHT11 ---
const int DHT_11 = 10;
DHT dht(DHT_11, DHTTYPE);  //creation de l'objet DHT
float temperature = 0;
float humidite = 0;

// --- Photoresistance ---
const int PHOTORESISTANCE = A0;
int valeurPhotoresistance = 0;
int valeurMin = 1023;
int valeurMax = 0;
bool calibrationActive = false;

// --- HC-SR04 ---
const int ECHO = 11;
const int TRIGGER = 12;
UltraSonicDistanceSensor distanceSensor(TRIGGER, ECHO);
int distance = 0;


// --- LED ---
const int LED = 9;
int pageActuelle = 1;

// --- BOUTON ---
const int BOUTON = 4;
OneButton button(BOUTON, true, true);

// --- JOYSTICK ---
const int JOYSTICKX = A1;
const int JOYSTICKY = A2;
const int boutonJoystick = 2;
OneButton joystickBtn(boutonJoystick, true, true);

// --- Moteur à pas ---
AccelStepper myStepper(MOTOR_INTERFACE_TYPE, IN_1, IN_3, IN_2, IN_4);

// Définition des variables globales
AppState currentState = DEMARRAGE;
EtatIrrigation etat_courant = FERMETURE;
EtatConvoyeur etatConvoyeur = INACTIF;
const int POSITIONOUVERTE = 2038;
const int POSITIONFERMER = 0;


int pourcentage = 0;


// Variables du convoyeur
int vitesseActuelle = 0;
int vitesseConstante = 0;  // Vitesse mémorisée pour le mode CONSTANT


U8G2_MAX7219_8X8_F_4W_SW_SPI u8g2(
  U8G2_R0,
  CLK_PIN,
  DIN_PIN,
  CS_PIN,
  U8X8_PIN_NONE,
  U8X8_PIN_NONE);

// ========== FONCTIONS D'AFFICHAGE MATRICE ==========

void configurerAffichage() {
  u8g2.begin();
  u8g2.setContrast(5);
  clearAffichage();
}

void clearAffichage() {
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}


// Affiche un X (état INACTIF)
void afficherInactif() {
  u8g2.clearBuffer();
  u8g2.drawLine(0, 0, 7, 7);
  u8g2.drawLine(7, 0, 0, 7);
  u8g2.sendBuffer();
}

// Affiche !! (état ACTIF mais pas de mouvement)
void afficherActif() {
  u8g2.clearBuffer();
  u8g2.drawBox(3, 0, 1, 5);
  u8g2.drawBox(4, 0, 1, 5);    // (x1,y1, largeur,hauteur)
  u8g2.drawBox(3, 6, 1, 1);
  u8g2.drawBox(4, 6, 1, 1);
  u8g2.sendBuffer();
}

// Flèche vers la droite (AVANCE)
const uint8_t flecheDroite[] PROGMEM = {
  B00001000,
  B00001100,
  B00001110,
  B11111111,
  B11111111,
  B00001110,
  B00001100,
  B00001000,
};

void afficherAvancer() {
  u8g2.clearBuffer();
  u8g2.drawXBMP(0, 0, 8, 8, flecheDroite);
  u8g2.sendBuffer();
}

// Flèche vers la gauche (RECULE)
const uint8_t flecheGauche[] PROGMEM = {
  B00010000,
  B00110000,
  B01110000,
  B11111111,
  B11111111,
  B01110000,
  B00110000,
  B00010000,
};

void afficherReculer() {
  u8g2.clearBuffer();
  u8g2.drawXBMP(0, 0, 8, 8, flecheGauche);
  u8g2.sendBuffer();
}


void mettreAJourAffichageMatrice() {
  switch (etatConvoyeur) {
    case INACTIF:
       afficherInactif();  // Affiche X
      break;
    case ACTIF:
      if (vitesseActuelle > 5) {
      afficherAvancer();  // Affiche -> quand on appuie sur axe X
    } else if (vitesseActuelle < -5) {
      afficherReculer();  // Affiche <- quand on appuie sur axe Y
    } else {
      afficherActif();  // Affiche !! quand pas de mouvement
    }
    
      break;
    case CONSTANT:
     if (vitesseConstante > 10) {
      afficherAvancer();  // Affiche -> en mode constant
    } else if (vitesseConstante < -10) {
        afficherReculer();  // Affiche <- en mode constant
      }
      break;
  }
}

// ========== FONCTIONS DU CONVOYEUR ==========
void controlerMoteur() {
  int vitesseMoteur = 0;

  // Déterminer la vitesse à appliquer selon l'état
  if (etatConvoyeur == INACTIF) {
    vitesseMoteur = 0;
  } else if (etatConvoyeur == ACTIF) {
    vitesseMoteur = vitesseActuelle;
  } else if (etatConvoyeur == CONSTANT) {
    vitesseMoteur = vitesseConstante;
  }

  // Appliquer la vitesse au moteur
  int pwmValue = map(abs(vitesseMoteur), 0, 100, 0, 255);

  if (vitesseMoteur > 5) {
    // Avancer
    analogWrite(input1, pwmValue);
    analogWrite(input2, 0);
  } else if (vitesseMoteur < -10) {
    // Reculer
    analogWrite(input1, 0);
    analogWrite(input2, pwmValue);
  } else {
    // Arrêt
    analogWrite(input1, 0);
    analogWrite(input2, 0);
  }
}

// ========== FONCTIONS DE LECTURE JOYSTICK ==========

void lireJoystick() {
  // Lire l'axe X pour avancer/reculer
  int lectureX = analogRead(JOYSTICKX);

  // Convertir la valeur (0-1023) en vitesse (-100 à 100)
  vitesseActuelle = map(lectureX, 0, 1023, -100, 100);

  // Zone morte: entre -5 et 5 = pas de mouvement
  if (abs(vitesseActuelle) < 5) {
    vitesseActuelle = 0;
  }

  // Limiter entre -100 et 100
 if (vitesseActuelle > 100) {
    vitesseActuelle = 100;
} else if (vitesseActuelle < -100) {
    vitesseActuelle = -100;
}
}

void onJoystickClick() {
  // Clic simple : bascule entre ACTIF et CONSTANT
  if (etatConvoyeur == ACTIF) {
    // Sauvegarder la vitesse actuelle et passer en mode CONSTANT
    vitesseConstante = vitesseActuelle;
    etatConvoyeur = CONSTANT;
    mettreAJourAffichageMatrice();
  } else if (etatConvoyeur == CONSTANT) {
    // Repasser en mode ACTIF
    etatConvoyeur = ACTIF;
    vitesseActuelle = vitesseConstante;
  }
}


void onJoystickLongClick() {
  if (etatConvoyeur == INACTIF) {
    // Long clic: ACTIF -> symbole !!
    etatConvoyeur = ACTIF;
    vitesseActuelle = 0;
    vitesseConstante = 0;
   mettreAJourAffichageMatrice();
  } else {
    // Long clic depuis ACTIF ou CONSTANT: retour à INACTIF
   etatConvoyeur = INACTIF;
    vitesseActuelle = 0;
    vitesseConstante = 0;
    mettreAJourAffichageMatrice();
  }

  // Arrêter le moteur immédiatement
  controlerMoteur();
}
void gestionConvoyeur() {
  // Si en mode ACTIF, lire le joystick
  if (etatConvoyeur == ACTIF) {
    lireJoystick();        // Lire la position du joystick
    controlerMoteur();     // Contrôler le moteur selon la vitesse
    mettreAJourAffichageMatrice();  // Mettre à jour l'affichage
  } else if (etatConvoyeur == CONSTANT) {
    controlerMoteur();     // Utilise vitesseConstante
    mettreAJourAffichageMatrice();  // Met à jour l'affichage (avec clignotement si arrêt)
  } else {
    // Mode INACTIF: moteur arrêté, affichage X
    controlerMoteur();
  }
}


// ========== FONCTIONS D'IRRIGATION ==========


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
  if (pourcentage < 0) pourcentage = 0;
  if (pourcentage > 100) pourcentage = 100;
}
//faire clignoter la LED
void clignotementLed(unsigned long tempsActuel) {
  const unsigned long delaiLed = 100;
   bool etatLed = false;
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
    case FERME: etatFermer(); break;
    case OUVERTURE: etatOUverture(); break;
    case OUVERT: etatOuvert(); break;
    case FERMETURE: etatFermeture(); break;
    case ARRET: etatArret(); break;
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
    if (valeurPhotoresistance < 0) valeurPhotoresistance = 0;
    if (valeurPhotoresistance > 1023) valeurPhotoresistance = 1023;  //pour éviter les valeurs hors plage
  }
}

void lireTemperatureHumidite(unsigned long now) {
  static unsigned long lastTime = 0;
  const unsigned long rate = 5000;
  if (now - lastTime >= rate) {
    lastTime = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (isnan(h) || isnan(t)) {  // valide si c'est un nombre.
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

void lumDist() {
  int valeurAlum = lireLuminosite();
  lcd.setCursor(0, 0);
  lcd.print("Lum: ");
  lcd.print(valeurAlum);
  lcd.print("% ");
  lcd.setCursor(0, 1);
  lcd.print("Dist: ");
  lcd.print(distance);
  lcd.print(" cm   ");
}

void tempHum() {
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("c ");
  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(humidite);
  lcd.print(" %   ");
}

void MinMaxLCD() {
  lcd.setCursor(0, 0);
  lcd.print("Lum Min: ");
  lcd.print(valeurMin);
  lcd.setCursor(0, 1);
  lcd.print("Lum Max: ");
  lcd.print(valeurMax);
  lcd.print(" %   ");
}

void vanneLCD() {
  lcd.setCursor(0, 0);
  lcd.print("Vanne : ");
  lcd.print(pourcentage);
  lcd.print(" %");
  lcd.setCursor(0, 1);
  lcd.print("Etat : ");
  switch (etat_courant) {
    case FERME: lcd.print("FERME"); break;
    case OUVERTURE: lcd.print("OUVERTURE"); break;
    case OUVERT: lcd.print("OUVERT"); break;
    case FERMETURE: lcd.print("FERMETURE"); break;
    case ARRET: lcd.print("ARRET"); break;
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

void onClick() {
  if (etat_courant == ARRET) {
    myStepper.moveTo(POSITIONOUVERTE);
    etat_courant = OUVERTURE;
    vanneLCD();
    return;
  }
  if (myStepper.isRunning()) {
    etatArret();
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