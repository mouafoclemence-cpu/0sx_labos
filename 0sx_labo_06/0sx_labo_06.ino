#include "Convoyeur.h"

void setup() {
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(BOUTON, INPUT_PULLUP);
   // Initialisation des broches du moteur DC avec le L293D
  pinMode(input1, OUTPUT);
  pinMode(input2, OUTPUT);
   // Initialiser les broches du joystick
  pinMode(boutonJoystick, INPUT_PULLUP);
   pinMode(JOYSTICKX, INPUT);
  pinMode(JOYSTICKY, INPUT);

  lcd.begin();
  lcd.backlight();  // Allume le rétroéclairage
  Serial.begin(9600);
  dht.begin();  // initialisation du du capteur de temperature et d'humidité


  button.attachClick(onClick);
  button.attachDoubleClick(onDoubleClick);
   joystickBtn.attachClick(onJoystickClick);
  joystickBtn.attachLongPressStart(onJoystickLongClick);

  myStepper.setMaxSpeed(1000);// Vitesse maximale (steps/seconde)
  myStepper.setSpeed(2000);// Vitesse de déplacement
  myStepper.setCurrentPosition(POSITIONOUVERTE); //reinitialise la position actuelle du moteur
  myStepper.setAcceleration(100); //demarrage et arret progressif
  sousTention();

 configurerAffichage();
  // État initial: INACTIF avec symbole X
  afficherInactif();
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
 joystickBtn.tick();
 
 gestionConvoyeur();

  // Petite pause pour éviter de surcharger le processeur
  delay(10);

}