#include <LCD_I2C.h>

LCD_I2C lcd(0x27, 16, 2);

const int LED = 8;
const int THERMITANCE = A0;
const int JOYSTICKX = A1;
const int JOYSTICKY = A2;
const int boutonJoystick = 2;

const float R1 = 10000;
const float c1 = 1.129148e-03, c2 = 2.34125e-04, c3 = 8.76741e-08;

int Vo;
float logR2, R2, T, Tc, Tf;
float temperature = 0.0;
int luminosite = 0;
unsigned long tempsLuminosite = 0;
bool etatLuminositePrecedent = false;

bool demarage = false;
int etatLed = LOW;
bool etatSysteme = false;

float tempMin = 24.0;
float tempMax = 25.0;

float vitesseX = 0.0;
float vitesseY = 0.0;
const float vMax = 5.0;
unsigned long dernierTempsX = 0;
unsigned long dernierTempsY = 0;
float dernierPosX = 0;
float dernierPosY = 0;

unsigned long tempsAffichage = 3000;  // 3 secondes
unsigned long dernierMomentAffiche = 0;
int pageActuelle = 1;  // 1 = capteur, 2 = position
unsigned long dernierAntiRebond = 0;
const int DELAI_ANTI_REBOND = 200;

unsigned long dernierAffichageLCD = 0;
const int INTERVALLE_LCD = 100;  // 100ms

unsigned long dernierEnvoiSerie = 0;
const int INTERVALLE_SERIE = 100;  // 100ms

// Caractères personnalisés
byte caractere1[8] = {
  B11100,
  B10100,
  B10100,
  B11100,
  B00111,
  B00101,
  B00101,
  B00111,
};

byte degre[8] = {
  B01110,
  B10001,
  B10001,
  B01110,
  B00000,
  B00000,
  B00000,
  B00000,
};

void setup() {

  pinMode(LED, OUTPUT);
  pinMode(boutonJoystick, INPUT_PULLUP);

  lcd.begin();
  lcd.backlight();
  lcd.createChar(2, caractere1);
  lcd.createChar(1, degre);

  Serial.begin(115200);

  // Initialisation des temps
  dernierTempsX = millis();
  dernierTempsY = millis();
  dernierMomentAffiche = millis();
}

void loop() {
  unsigned long tempsActuel = millis();

  lectureTemperature();
  allumeEteindLed();
  vitesseJoystick(tempsActuel);
  lecturBouton(tempsActuel);

  if (tempsActuel - dernierAffichageLCD >= INTERVALLE_LCD) {
    affichage();
    dernierAffichageLCD = tempsActuel;
  }

  if (tempsActuel - dernierEnvoiSerie >= INTERVALLE_SERIE) {
    envoiSerie();
    dernierEnvoiSerie = tempsActuel;
  }
}

void lectureTemperature() {
  static unsigned long dernierAffichageTemp = 0;
  unsigned long tempsActuel = millis();

  if (tempsActuel - dernierAffichageTemp >= 1000) {
    Vo = analogRead(THERMITANCE);
    R2 = R1 * (1023.0 / (float)Vo - 1.0);
    logR2 = log(R2);
    T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
    Tc = T - 273.15;
    temperature = Tc;

    luminosite = map(Vo, 0, 1023, 0, 100);
    dernierAffichageTemp = millis();
  }
}

void allumeEteindLed() {
  if (temperature > tempMax && etatLed == LOW) {
    digitalWrite(LED, HIGH);
    etatLed = HIGH;
    etatSysteme = true;
  } else if (temperature < tempMin && etatLed == HIGH) {
    digitalWrite(LED, LOW);
    etatLed = LOW;
    etatSysteme = false;
  }
}

void vitesseJoystick(unsigned long tempsActuel) {
  int lectureX = analogRead(JOYSTICKX);
  int lectureY = analogRead(JOYSTICKY);

  vitesseX = TacheDeplacementX(lectureX, tempsActuel);
  vitesseY = TacheDeplacementY(lectureY, tempsActuel);

  if (vitesseX > 100) {
    vitesseX = 100;
  }
  if (vitesseX < -100) {
    vitesseX = -100;
  }
  if (vitesseY > 100) {
    vitesseY = 100;
  }
  if (vitesseY < -100) {
    vitesseY = -100;
  }

  dernierPosX = vitesseX;
  dernierPosY = vitesseY;
}

float TacheDeplacementX(int x, unsigned long tempsActuel) {
  const float taux = 100;  // 100ms minimum entre les calculs

  if (tempsActuel - dernierTempsX < taux) {
    return dernierPosX;
  }

  float deltaT = (tempsActuel - dernierTempsX) / 1000.0;  // Conversion en secondes. le temps écoulé entre 2 mesure
  dernierTempsX = tempsActuel;

  float vitesse = map(x, 0, 1023, -vMax, vMax);
  float pos = dernierPosX + vitesse * deltaT;

  return pos;
}

float TacheDeplacementY(int y, unsigned long tempsActuel) {
  const float taux = 100;

  if (tempsActuel - dernierTempsY < taux) {
    return dernierPosY;
  }

  float deltaT = (tempsActuel - dernierTempsY) / 1000.0;
  dernierTempsY = tempsActuel;

  float vitesse = map(y, 0, 1023, -vMax, vMax);
  float pos = dernierPosY + vitesse * deltaT;

  return pos;
}

void lecturBouton(unsigned long tempsActuel) {
  static int dernierEtatBouton = HIGH;
  int lecture = digitalRead(boutonJoystick);

  if (lecture == LOW && dernierEtatBouton == HIGH && (tempsActuel - dernierAntiRebond) > DELAI_ANTI_REBOND) {

    dernierAntiRebond = tempsActuel;

    // Changer de page
    if (pageActuelle == 1) {
      pageActuelle = 2;
    } else {
      pageActuelle = 1;
    }
    lcd.clear();
  }

  dernierEtatBouton = lecture;
}

void affichage() {
  if (!demarage) {
    // Affichage de démarrage (3 secondes)
    lcd.setCursor(0, 0);
    lcd.print("Ngo Hot Mbem    ");

    lcd.setCursor(15, 1);
    lcd.write(2);

    if (millis() - dernierMomentAffiche >= tempsAffichage) {
      demarage = true;
      lcd.clear();
    }
  } else {
    // Affichage normal selon la page
    if (pageActuelle == 1) {
      // Page 1 : Température/Luminosité
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print((int)temperature);
      lcd.write(1);  // Symbole degré
      lcd.print("C ");

      lcd.setCursor(0, 1);
      lcd.print("AC: ");
      if (etatLed == HIGH) {
        lcd.print("ON ");
      } else {
        lcd.print("OFF");
      }
      lcd.print("    ");

    } else {
      // Page 2 : Position du monte-charge
      lcd.setCursor(0, 0);
      lcd.print("X: ");
      lcd.print((int)vitesseX);  // pour convertir en entier.
      lcd.print(" cm   ");

      lcd.setCursor(0, 1);
      lcd.print("Y: ");
      lcd.print((int)vitesseY);
      lcd.print(" cm   ");
    }
  }
}

void envoiSerie() {
  int lectureX = analogRead(JOYSTICKX);
  int lectureY = analogRead(JOYSTICKY);

  Serial.print("etd:2438400");
  Serial.print(",x:");
  Serial.print(lectureX);
  Serial.print(",y:");
  Serial.print(lectureY);
  Serial.print(",sys:");
  if (etatSysteme == true) {
    Serial.println(1);
  } else {
    Serial.println(0);
  }
}