#pragma once 
#ifndef serre_final_h
#define serre_final_h

#include <Arduino.h>
#include <OneButton.h>
#include "DHT.h"
#include <LCD_I2C.h>
#include <HCSR04.h>
#include <AccelStepper.h>
#include <U8g2lib.h>

// Définition des constantes
// --- Moteur DC avec L293D ---
extern const int input1;
extern const int input2;

// --- LCD ---
extern LCD_I2C lcd;

// --- DHT11 ---
#define DHTTYPE DHT11
extern const int DHT_11;
extern DHT dht;
extern float temperature;
extern float humidite;

// --- Photoresistance ---
extern const int PHOTORESISTANCE;
extern int valeurPhotoresistance;
extern int valeurMin;
extern int valeurMax;
extern bool calibrationActive;

// --- HC-SR04 ---
extern const int ECHO;
extern const int TRIGGER;
extern UltraSonicDistanceSensor distanceSensor;
extern int distance;

//-------LED--------
extern const int LED;
extern int pageActuelle;

// --- BOUTON ---
extern const int BOUTON;
extern OneButton button;

// --- JOYSTICK ---
extern const int JOYSTICKX;
extern const int JOYSTICKY;
extern const int boutonJoystick;
extern OneButton joystickBtn;

// --- Moteur à pas ---
#define MOTOR_INTERFACE_TYPE 4
#define IN_1 31
#define IN_2 33
#define IN_3 35
#define IN_4 37
extern AccelStepper myStepper;

// Énumérations
enum AppState { DEMARRAGE, DHT_STATE, LUM_DIST, CALIBRATION };
enum EtatConvoyeur { INACTIF, ACTIF, CONSTANT };

// NOUVELLE STRUCTURE POUR L'IRRIGATION
typedef enum {FERME, OUVERTURE, OUVERT, FERMETURE, ARRET} IrrigationState;

extern int vitesseActuelle;
extern int vitesseConstante;

class Irrigation {
private:
  int _ledPin;
  int _pin1, _pin2, _pin3, _pin4;
  int _positionFermee;
  int _positionOuverte;
  int* _distanceRef;
  bool* _clickFlagRef;
  IrrigationState _etatActuel;
  bool _estEnMouvement;

public:
  // Constructeur
  Irrigation(int ledPin, int pin1, int pin2, int pin3, int pin4);
  
  
   int getPosition() { return myStepper.currentPosition(); }
  
   int getPositionPct() {
    int pct = map(myStepper.currentPosition(), _positionFermee, _positionOuverte, 0, 100);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return pct;
  }
  
   void setClosedOpenedPos(int closed, int opened) {
    _positionFermee = closed;
    _positionOuverte = opened;
  }
  
   int setDistance(int &dist) {
    _distanceRef = &dist;
    return 0;
  }
  
   void setBtnClickFlag(bool &clickFlag) {
    _clickFlagRef = &clickFlag;
  }
  
   int isMoving() {
    return (_etatActuel == OUVERTURE || _etatActuel == FERMETURE);
  }
  
  int getCurrentState() {
    return _etatActuel;
  }
  
  // Fonctions publiques
  void demarrerOuverture();
  void demarrerFermeture();
  void arreter();
  void update();
};

#define CLK_PIN 30
#define DIN_PIN 34
#define CS_PIN  32

// Variables globales 
extern AppState currentState;
extern IrrigationState etat_courant;
extern EtatConvoyeur etatConvoyeur;
extern const int POSITIONOUVERTE;
extern const int POSITIONFERMER;
extern int pourcentage;
extern Irrigation irrigation;  // Déclaration de l'objet global

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

void vanneLCD();
void afficherLCD(unsigned long tempsActuel);
void onClick();
void onDoubleClick();
void configurerAffichage();
void clearAffichage();
void mettreAJourAffichageMatrice();
void afficherInactif();
void afficherActif();
void afficherAvancer();
void afficherReculer();
void controlerMoteur();
void lireJoystick();
void onJoystickClick();
void onJoystickLongClick();
void gestionConvoyeur();

void setLine1(String line1);
void setLine2(String line2);
void updateAffichage();
void setRefreshRate(int refreshRate);
int getRefreshRate();
void preparerTexteAffichage();
void clearAffichageMatrice(); 

#endif