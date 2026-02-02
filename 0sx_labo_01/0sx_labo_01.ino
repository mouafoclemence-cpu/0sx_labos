const int LED = 13;           // le numero de la broche.
const int VITESSE = 9600;     // vitesse en bauds.
const int NOMBREDECYCLE = 5;  //Avant dernier chiffre de mon numero etudiant 0 = 10; 10/2
int etat = 1;                 // État de la LED.
int dureDunClignotement = 250;
int luminositeMax = 255;
int pause = 2048 /luminositeMax;
int etatEteint = 250;
int cycleAllume = 1000;


void setup() {
  Serial.begin(VITESSE);    //initialisation de la vitesse de communication
  pinMode(LED, OUTPUT);  // configuration de la LED en mode sortie.
}


void loop() {


  switch (etat) {

    case 1:                                      //Clignotement.
      Serial.println("Clignotement – 2438400");  //affichage
      for (int i = 0; i < NOMBREDECYCLE; i++) {  // Pour éteindre et allumer 5 fois.
        digitalWrite(LED, HIGH);                 //La LED a une intensité de 5v.
        delay(dureDunClignotement);              //pause de 250ms.
        digitalWrite(LED, LOW);                  // La LED a une intensité de 0v.
        delay(dureDunClignotement);              //pause de 250ms.
      }
      etat = 2;  // Passe a l'état suivante.
      break;

    case 2:
      Serial.println("Variation  – 2438400");  // Affichage
      for (int i = 0; i <= luminositeMax; i++) {         //l'intensite augmente de 1 a chaque passage apres une pause de 8ms.
        analogWrite(LED, i);
        delay(pause);
      }
      etat = 3;  //Passe a l'état suivante.
      break;

    case 3:
      Serial.println("Allume  – 2438400");  //Affichage.
      digitalWrite(LED, LOW);               //Débute éteins pendant 250ms.
      delay(etatEteint);
      digitalWrite(LED, HIGH);
      delay(cycleAllume);
      digitalWrite(LED, LOW);
      delay(cycleAllume);
      etat = 1;
      break;
  }
}
