#pragma once 
#ifndef Convoyeur_h
#define Convoyeur_h

#include <Arduino.h>
#include <OneButton.h>
#include "DHT.h"  //librairie du capteur de temperature et d'humidité
#include <LCD_I2C.h>
#include <HCSR04.h>
#include <AccelStepper.h>  //librairie moteur pas à pas

 //librairie matrice display module max7219
#include <U8g2lib.h>

// Définition des constantes
// --- Moteur DC avec L293D ---
 extern const int input1;
 extern const int input2;

// --- LCD ---
extern LCD_I2C lcd;

// --- DHT11 ---
#define DHTTYPE DHT11  // type de capteur d'humidité et de temperature
extern const int DHT_11 ;
extern DHT dht;  //creation de l'objet DHT
extern float temperature ;
extern float humidite ;

// --- Photoresistance ---
extern const int PHOTORESISTANCE;
extern int valeurPhotoresistance;
extern int valeurMin;
extern int valeurMax;
extern bool calibrationActive;

// --- HC-SR04 ---
extern const int ECHO;
extern const int TRIGGER;
extern UltraSonicDistanceSensor distanceSensor;  //creaction d'objet pour le capteur a distance.
extern int distance;

//-------LED--------
extern const int LED;
extern int pageActuelle;

// --- BOUTON ---
extern const int BOUTON;
extern OneButton button;

// --- LED DOT matrice display module max7219 ---
extern const int BROCHECLK;
extern const int BROCHECS;  
extern const int BROCHEDIN;

// --- JOYSTICK ---
extern const int JOYSTICKX;
extern const int JOYSTICKY;
extern const int boutonJoystick;
extern OneButton joystickBtn;

// --- Moteur à pas ---
#define MOTOR_INTERFACE_TYPE 4  //type de moteur

#define IN_1 31
#define IN_2 33
#define IN_3 35
#define IN_4 37
extern AccelStepper myStepper;  //creation de l'objet moteur pas pas.


// Énumérations
enum AppState { DEMARRAGE, DHT_STATE, LUM_DIST, CALIBRATION };
enum EtatIrrigation { FERME, OUVERTURE, OUVERT, FERMETURE, ARRET };
enum EtatConvoyeur { INACTIF , ACTIF, CONSTANT };

#define CLK_PIN 30
#define DIN_PIN 34
#define CS_PIN  32


// Variables globales 
extern AppState currentState;
extern EtatIrrigation etat_courant;
extern EtatConvoyeur etatConvoyeur ;
extern const int POSITIONOUVERTE;
extern const int POSITIONFERMER;
extern int pourcentage;

// Déclarations des fonctions
void sousTention();
void etatFermer();
void etatOuverture();
void etatOuvert();
void etatFermeture();
void etatArret();
void position();
void clignotementLed(unsigned long tempsActuel);
void gestionIrrigation(unsigned long tempsActuel);
void lirePhotoresistance(unsigned long tempsActuel);
void lireTemperatureHumidite(unsigned long now);
void calibrage();
int lireLuminosite();
void allumeEteindLed();
void lireLaDistance(unsigned long tempsActuel);
void portserie(unsigned long tempsActuel);
void lumDist();
void tempHum();
void MinMaxLCD();
void vanneLCD();
void afficherLCD(unsigned long tempsActuel);
void onClick();
void onDoubleClick();
void confiAffichage();
void clearAffichage();

void configurerAffichage();
void mettreAJourAffichageMatrice();
void afficherInactif();
void afficherActif();
void afficherAvancer();
void afficherReculer();
void controlerMoteur();
void lireJoystick();
void onJoystickClick();
void  onJoystickLongClick();
void gestionConvoyeur();
#endif    