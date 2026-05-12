#include "serre_final.h"

#define HOME 1

#include <WiFiEspAT.h>
#include <PubSubClient.h>
#include <DHT.h>

#define AT_BAUD_RATE 115200

#if HOME
#define DEVICE_NAME "2438400"
#else
#define DEVICE_NAME "2438400"
#endif

#define MQTT_PORT 1883
#define MQTT_USER "etdshawi"
#define MQTT_PASS "shawi123"

// Serveur MQTT du prof
const char* mqttServer = "216.128.180.194";

WiFiClient wifiClient;
PubSubClient client(wifiClient);
int conveyorSpeed = 0;

// Variables pour gestion WiFi non-bloquante
bool wifiConnected = false;
unsigned long lastWifiAttempt = 0;
const unsigned long wifiRetryInterval = 5000; // 5 secondes
unsigned long lastMqttReconnectAttempt = 0;
const unsigned long mqttReconnectInterval = 3000; // 3 secondes

// Variables pour l'envoi périodique des données
unsigned long lastDataSend = 0;
const unsigned long dataSendInterval = 10000; // 10 secondes

// Règle de sécurité - le convoyeur doit être actif
bool conveyorActif = false;  // Variable pour la règle de sécurité

// Fonction d'initialisation WiFi non-bloquante
void wifiInit() {
  // Initialisation du module WiFi.
  Serial3.begin(AT_BAUD_RATE);
  WiFi.init(Serial3);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println();
    Serial.println("La communication avec le module WiFi a échoué!");
    wifiConnected = false;
    // Ne pas bloquer - continuer sans WiFi
    return;
  }

  Serial.println("Tentative de connexion au WiFi...");
  // La connexion se fera de manière asynchrone dans manageWiFi()
}

// Gestion de la connexion WiFi non-bloquante
void manageWiFi() {
  // Si déjà connecté, vérifier que la connexion est toujours active
  if (wifiConnected) {
    if (WiFi.status() != WL_CONNECTED) {
      wifiConnected = false;
      Serial.println();
      Serial.println("WiFi déconnecté!");
    }
    return;
  }
  
  // Tentative de reconnexion périodique
  unsigned long currentMillis = millis();
  if (currentMillis - lastWifiAttempt >= wifiRetryInterval) {
    lastWifiAttempt = currentMillis;
    
    // Vérifier si le module WiFi est présent
    if (WiFi.status() == WL_NO_MODULE) {
      return;
    }
    
    // Essayer de se connecter
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      Serial.println();
      Serial.println("WiFi connecté!");
      IPAddress ip = WiFi.localIP();
      Serial.print("Adresse IP: ");
      Serial.println(ip);
      printWifiStatus();
      
      // Configurer et connecter MQTT une fois le WiFi établi
      setupMqtt();
    }
  }
}

// Configuration MQTT
void setupMqtt() {
  if (!wifiConnected) return;
  
  client.setServer(mqttServer, MQTT_PORT);
  client.setCallback(mqttEvent);
  
  // Essayer de se connecter immédiatement
  reconnectMqtt();
}

// Reconnexion MQTT non-bloquante
void reconnectMqtt() {
  if (!wifiConnected) return;

  unsigned long currentMillis = millis();
  if (currentMillis - lastMqttReconnectAttempt >= mqttReconnectInterval) {
    lastMqttReconnectAttempt = currentMillis;

    if (!client.connected()) {
      Serial.print("Tentative de connexion MQTT... ");
      // Connexion au broker
      if (client.connect(DEVICE_NAME, MQTT_USER, MQTT_PASS)) {
        Serial.println("Connecté!");
        client.subscribe("etu_07/#");
        Serial.println("Abonné aux topics MQTT: etu_07/#");

      } else {
        Serial.print("Échec, état=");
        Serial.print(client.state());
        Serial.println(" - nouvelle tentative plus tard");
      }
    }
  }
}

// Gestion des messages reçus du serveur MQTT
void mqttEvent(char* topic, byte* payload, unsigned int length) {
  // Convertir le payload en string
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Message reçu [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);
  
  // Traiter les commandes selon le topic
  String topicStr = String(topic);
  
  
  if (topicStr == "etu_07/convVit") {
    // Contrôle de la vitesse du convoyeur
    int vitesse = message.toInt();
    if (vitesse >= -100 && vitesse <= 100) {
      
      // RÈGLE DE SÉCURITÉ: le convoyeur doit être actif pour pouvoir s'activer
      if (conveyorActif) {
        // Le convoyeur est actif, on peut appliquer la vitesse
        conveyorSpeed = vitesse;
        
        Serial.print("Vitesse du convoyeur réglée à: ");
        Serial.println(conveyorSpeed);
        Serial.println("(convoyeur actif - commande acceptée)");
        
        // Appliquer la vitesse au moteur (utilise vos fonctions existantes)
        controlerMoteur();
        mettreAJourAffichageMatrice();
      } else {
        // Règle de sécurité: convoyeur inactif, on ignore la commande
        Serial.println("ERREUR - RÈGLE DE SÉCURITÉ: Convoyeur inactif, commande ignorée");
        Serial.println("Activez le convoyeur avant d'envoyer des commandes de vitesse");
      }
    } else {
      Serial.print("Vitesse invalide: ");
      Serial.println(vitesse);
    }
  }
}

// Fonction pour activer le convoyeur (à appeler quand le convoyeur devient actif)
void activerConvoyeur() {
  conveyorActif = true;
  Serial.println("Convoyeur activé - les commandes MQTT sont maintenant acceptées");
}

// Fonction pour désactiver le convoyeur
void desactiverConvoyeur() {
  conveyorActif = false;
  conveyorSpeed = 0;
  Serial.println("Convoyeur désactivé - les commandes MQTT sont ignorées pour sécurité");
}

// Procédure Afficher le status de la connection WiFi sur le port série
void printWifiStatus() {
  // Imprimer le SSID du réseau auquel vous êtes connecté:
  char ssid[33];
  WiFi.SSID(ssid);
  Serial.print("SSID: ");
  Serial.println(ssid);

  // Imprimer le BSSID du réseau auquel vous êtes connecté:
  uint8_t bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  printMacAddress(mac);

  // Imprimer l'adresse IP de votre carte:
  IPAddress ip = WiFi.localIP();
  Serial.print("Adresse IP: ");
  Serial.println(ip);

  // Imprimer la force du signal reçu:
  long rssi = WiFi.RSSI();
  Serial.print("Force du signal (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}

// Envoi périodique des données des capteurs selon le format JSON demandé
void sendSensorData() {
  unsigned long currentTime = millis();
  if (currentTime - lastDataSend < dataSendInterval) return;
  if (!client.connected()) return;
  if (!wifiConnected) return;

  lastDataSend = currentTime;

  char message[256];
  char szTemp[10];
  char szHum[10];
  
  // Convertir les valeurs en chaîne de caractères avec 2 décimales
  dtostrf(temperature, 5, 2, szTemp);
  dtostrf(humidite, 5, 2, szHum);
  
  // Lecture de la luminosité (valeur entre 0 et 100)
  int lumiere = lireLuminosite();
  
  // Récupérer l'état et la position de l'irrigation
  int irrState = irrigation.getCurrentState();
  int irrPos = pourcentage;

  // Création du message JSON selon le format exact de l'énoncé
  snprintf(message, sizeof(message), 
           "{\"name\":\"%s\",\"temp\":\"%s\",\"hum\":\"%s\",\"millis\":%lu,\"lum\":%d,\"irrState\":%d,\"irrPos\":%d,\"convVit\":%d}",
           DEVICE_NAME, szTemp, szHum, currentTime, lumiere, irrState, irrPos, conveyorSpeed);

  Serial.print("Envoi des données: ");
  Serial.println(message);

  // Publier le message sur le topic "etd/XX" comme demandé dans l'énoncé
  // Remplacez 07 par votre numéro
  if (!client.publish("etd/07", message)) {
    Serial.println("Incapable d'envoyer le message!");
  } else {
    Serial.println("Message envoyé avec succès");
  }
}

void setup() {
  // === INITIALISATION DES PINS ===
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(BOUTON, INPUT_PULLUP);
  pinMode(input1, OUTPUT);
  pinMode(input2, OUTPUT);
  pinMode(boutonJoystick, INPUT_PULLUP);
  pinMode(JOYSTICKX, INPUT);
  pinMode(JOYSTICKY, INPUT);

  // === INITIALISATION LCD ===
  lcd.begin();
  lcd.backlight();
  
  // Configuration du nouvel affichage
  setRefreshRate(250);
  clearAffichage();
  
  // === INITIALISATION SERIE ET CAPTEURS ===
  Serial.begin(115200);
  dht.begin();

  // === INITIALISATION BOUTONS ===
  button.attachClick(onClick);
  button.attachDoubleClick(onDoubleClick);
  joystickBtn.attachClick(onJoystickClick);
  joystickBtn.attachLongPressStart(onJoystickLongClick);

  // === INITIALISATION MOTEUR PAS À PAS ===
  myStepper.setMaxSpeed(1000);
  myStepper.setSpeed(2000);
  myStepper.setCurrentPosition(POSITIONOUVERTE);
  myStepper.setAcceleration(100);
  
  // === INITIALISATION IRRIGATION ===
  irrigation.setClosedOpenedPos(POSITIONFERMER, POSITIONOUVERTE);
  irrigation.setDistance(distance);
  
  // === INITIALISATION SYSTÈME ===
  sousTention();
  configurerAffichage();
  afficherInactif();
  
  // Initialiser le convoyeur comme inactif (règle de sécurité)
  conveyorActif = false;
  conveyorSpeed = 0;

  // === INITIALISATION WIFI (non-bloquante) ===
  Serial.println("Initialisation du module WiFi...");
  wifiInit();
  
  // Premier affichage
  setLine1("Systeme pret");
  setLine2("Appuyez btn");
}

void loop() {
  unsigned long tempsActuel = millis();
  
  // === LECTURE DES CAPTEURS (prioritaire) ===
  lirePhotoresistance(tempsActuel);
  lireTemperatureHumidite(tempsActuel);
  lireLaDistance(tempsActuel);
  calibrage();
  
  // === GESTION DE L'IRRIGATION ===
  irrigation.update();
  etat_courant = (IrrigationState)irrigation.getCurrentState();
  pourcentage = irrigation.getPositionPct();
  
  // === AFFICHAGE LCD ===
  position();
  afficherLCD(tempsActuel);
  portserie(tempsActuel);
  
  // === GESTION DES BOUTONS ===
  button.tick();
  joystickBtn.tick();
  
  // === GESTION DU CONVOYEUR ===
  gestionConvoyeur();
  
  // === GESTION WIFI ET MQTT (non-bloquante) ===
  manageWiFi();           // Gérer la connexion WiFi
  reconnectMqtt();        // Gérer la connexion MQTT
  client.loop();          // Maintenir la connexion MQTT
  sendSensorData();       // Envoyer les données périodiquement
}