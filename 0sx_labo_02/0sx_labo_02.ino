const int BROCHESLEDS[] = { 8, 9, 10, 11 };
const int BROCHEBOUTON = 2;
const int BROCHEPOTENTIOMETRE = A0;

int taille = 4;
unsigned long intervalLecture = 20;      // 20ms pour lire la valeur du potentiometre
unsigned long dernierMomentLecture = 0;  // Dernier moment de lecture du pot

unsigned long dernierMomentRebond = 0;  // Pour l'anti-rebond


int boutonStable = HIGH;      // position du bouton avant d'appuyer
int etatActuelBouton = HIGH;  // initialisation du bouon après chaque appuies

int valeurPotentiometre = 0;
int valeurConvertie = 0;  // Variable pour stocker la valeur convertie
int min_depar = 0;
int max_depart = 1023;
int min_arrivee = 0;
int max_arrivee = 20;
unsigned long baud = 9600;


void setup() {
  for (int i = 0; i < taille; i++) {
    pinMode(BROCHESLEDS[i], OUTPUT);
  }
  pinMode(BROCHEBOUTON, INPUT_PULLUP);
  Serial.begin(baud);
  Serial.println("Prêt !");
}

void convertion() {
  if (millis() - dernierMomentLecture >= intervalLecture) {
    valeurPotentiometre = analogRead(BROCHEPOTENTIOMETRE);
    valeurConvertie = map(valeurPotentiometre, min_depar, max_depart, min_arrivee, max_arrivee);
    dernierMomentLecture = millis();
  }
}

void lectureValeur() {
  int lecture = digitalRead(BROCHEBOUTON);
  if (lecture != boutonStable) {
    dernierMomentRebond = millis();  // Réinitialise si changement
    boutonStable = lecture;
  }

  if ((millis() - dernierMomentRebond) > intervalLecture) {  //etat stable depuis longtemps
    if (lecture != etatActuelBouton) {
      etatActuelBouton = lecture;

      if (etatActuelBouton == LOW) {  //bouton presser;
        Serial.print("[");
        for (int i = 0; i < valeurConvertie; i++) {
          if (i < valeurConvertie) {
            Serial.print("*");
          }
        }
        for (int i = valeurConvertie; i < intervalLecture; i++) {

          Serial.print(".");
        }
        Serial.print("]");
        Serial.print(valeurConvertie * 100 / intervalLecture);
        Serial.println("%");
      }
    }
  }
}
void etatLeds() {
  for (int i = 0; i < taille; i++) {
    digitalWrite(BROCHESLEDS[i], LOW);
  }
  if (valeurConvertie >= 0 && valeurConvertie <= 5) {
    digitalWrite(BROCHESLEDS[0], HIGH);

  } else if (valeurConvertie > 5 && valeurConvertie <= 10) {
    digitalWrite(BROCHESLEDS[1], HIGH);
  } else if (valeurConvertie > 10 && valeurConvertie <= 15) {
    digitalWrite(BROCHESLEDS[2], HIGH);
  } else {
    digitalWrite(BROCHESLEDS[3], HIGH);
  }
}
void loop() {
  unsigned long tempsEcouler = millis();  //temps écoulé depuis le demarage en milliseconde
  convertion();
  lectureValeur();
  etatLeds();
}
