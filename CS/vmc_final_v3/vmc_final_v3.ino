/*
Ma VMC double flux
Alexandre Buissé
2023/04/13
*/

// Import des bibliotheques
#include <DHT.h>
#include <SoftwareSerial.h>

// Pins d'interruptions relay des ventilateurs
const int OUT_FAN_PIN = 7;
const int IN_FAN_PIN = 8;

// Pins PWM et RPM des ventilateurs
const int OUT_PWM_PIN = 9;
const int OUT_SENSOR_PIN = 2;
const int IN_PWM_PIN = 10;
const int IN_SENSOR_PIN = 3;

// Pins et types des sondes DHT
const int DHT1PIN = 4; // pour temp extérieur à l'insuflation
const int DHT2PIN = 5;
const int DHT3PIN = 6;
#define DHT1TYPE DHT22
#define DHT2TYPE DHT22
#define DHT3TYPE DHT22

// Pins de HC05
const int RXPIN = 13;
const int TXPIN = 12;

// Commmandes
const char CMD_BUTTON_0 = '0';
const char CMD_BUTTON_1 = '1';
const char CMD_BUTTON_2 = '2';
const char CMD_BUTTON_3 = '3';
const char CMD_BUTTON_4 = '4';
const char CMD_BUTTON_5 = '5';
const char CMD_BUTTON_6 = '6';
const char CMD_BUTTON_7 = '7';
const char CMD_BUTTON_S = 's';
const char CMD_BUTTON_B = 'b';
const char CMD_BUTTON_P = '+';
const char CMD_BUTTON_M = '-';
const char CMD_BUTTON_INFO = 'i';

// Initialisation des sondes DHT
DHT dht1(DHT1PIN, DHT1TYPE);
DHT dht2(DHT2PIN, DHT2TYPE);
DHT dht3(DHT3PIN, DHT3TYPE);

// Initialisation du HC05
SoftwareSerial mySerial(RXPIN, TXPIN);

// Définitions des variables
float vitesse = 0;
float old_vitesse_e = 0;
float old_vitesse_i = 0;
float vitesse_e = 0;
float vitesse_i = 0;
int mode = 1; // par défaut la ventilation est allumée
int sonde = 0;

void setup() {
  // Initialisation
  
  // Sondes DHT
  dht1.begin();
  dht2.begin();
  dht3.begin();

  // HC05
  pinMode(RXPIN, INPUT);
  pinMode(TXPIN, OUTPUT);

  // Ventilateurs
  
  // Configure Timer 1 for PWM @ 25 kHz.
  TCCR1A = 0;           // undo the configuration done by...
  TCCR1B = 0;           // ...the Arduino core library
  TCNT1  = 0;           // reset timer
  TCCR1A = _BV(COM1A1)  // non-inverted PWM on ch. A
         | _BV(COM1B1)  // same on ch; B
         | _BV(WGM11);  // mode 10: ph. correct PWM, TOP = ICR1
  TCCR1B = _BV(WGM13)   // ditto
         | _BV(CS10);   // prescaler = 1
  ICR1   = 320;         // TOP = 320

  // ON/OFF
  pinMode(OUT_FAN_PIN, OUTPUT);
  pinMode(IN_FAN_PIN, OUTPUT);
  digitalWrite(OUT_FAN_PIN, HIGH);
  digitalWrite(IN_FAN_PIN, HIGH);
  // Gestion de la vitesse
  pinMode(OUT_PWM_PIN, OUTPUT);
  pinMode(IN_PWM_PIN, OUTPUT);
  
  mySerial.begin(9600);
  Serial.begin(9600);

}

void loop() {
  
  int res = 0;
  int rpm = 0;
  int rpm_e = 0;
  int rpm_i = 0;
  float temp_moy;
  float hygrometrie_moy;
  float ext_temp;
  float ext_hygro;
  bool receive = false;
  char raw_message = "";
  char message = 'a';
  
  // Si CMD_BUTTON_O ou CMD_BUTTON_1 ou CMD_BUTTON_S enclenché juste avant, on boucle sur le mode auto ou le mode 2
  if (mode == 1 || mode == 2){

    res = getHumidity(1);
    
    ext_temp = getTemp(dht1);
    Serial.println((String)"Température extérieure : "+int(ext_temp)+" °C."); 
    
    if (ext_temp >= 1) {
      if (mode == 1) {
        Serial.println(F("Test ok mode 1"));
       
        vitesse = automode(res);
        vitesse_e = vitesse;
        vitesse_i = vitesse-30;
      }
      
      if (mode == 2) {
        Serial.println(F("Test ok mode 2"));
        vitesse = 70;
        vitesse_e = vitesse;
        vitesse_i = vitesse-30;
      }

      rpm_e = get_speed(int(vitesse_e));
      rpm_i = get_speed(int(vitesse_i));

      Serial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      Serial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
    }
    else {
      // on coupe la VMC, même l'extraction car risque de gel des condensats
      vitesse_e = 0;
      vitesse_i = 0;
      rpm_e = 0;
      rpm_i = 0;
      Serial.println((String)"Température extérieur trop faible pour démarrer la VMC ("+float(ext_temp)+" °C)");
    }
  }
  
  // On attend de recevoir un message (sans bloquer le programme)
  if (mySerial.available()) {
    raw_message = mySerial.read();
    message = msgFilter(raw_message);
    if (message != 'a') {
      receive = true;
    }
    else {
      return;
    }
  }

  if (message == CMD_BUTTON_0) {
    // Si VMC éteinte, on allume en mode automatique
    if (vitesse == 0){
      Serial.println(F("Allumage en mode automatique."));
      mySerial.println(F("Allumage en mode automatique."));
      
      res = getHumidity(1);
      ext_temp = getTemp(dht1);
      
      vitesse = automode(res);
      vitesse_e = vitesse;
      vitesse_i = vitesse-30;
      rpm_e = get_speed(int(vitesse_e));
      rpm_i = get_speed(int(vitesse_i));

      hygrometrie_moy = getHumidity(2);
      Serial.println((String)"Hygrométrie moyenne : "+int(hygrometrie_moy)+" %.");
      mySerial.println((String)"Hygrométrie moyenne : "+int(hygrometrie_moy)+" %.");
      
      Serial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      mySerial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      Serial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
      mySerial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");

      Serial.println((String)"Température extérieure : "+int(ext_temp)+" °C.");
      mySerial.println((String)"Température extérieure  : "+int(ext_temp)+" °C.");

      mode = 1;
    }
    // Si VMC allumée, on l'éteint
    else{
      Serial.println(F("Arrêt de la VMC."));
      mySerial.println(F("Arrêt de la VMC."));

      vitesse = 0;
      mode = -1;
    }

  }    
    
  else if (message == CMD_BUTTON_1) {
    if (mode != 1) {
      Serial.println(F("Mode automatique."));
      mySerial.println(F("Mode automatique."));
      // mode automatique
      res = getHumidity(1);
      vitesse = automode(res);
      vitesse_e = vitesse;
      vitesse_i = vitesse-30;
      rpm_e = get_speed(int(vitesse_e));
      rpm_i = get_speed(int(vitesse_i));
      Serial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      mySerial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      Serial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
      mySerial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
  
      hygrometrie_moy = getHumidity(2);
      Serial.println((String)"Hygrométrie moyenne : "+int(hygrometrie_moy)+" %.");
      mySerial.println((String)"Hygrométrie moyenne : "+int(hygrometrie_moy)+" %.");
    
    }
    else {
      Serial.println(F("Mode automatique déjà activé."));
      mySerial.println(F("Mode automatique déjà activé."));
    }
    
    mode = 1;
  }
  
  else if (message == CMD_BUTTON_P) {
    Serial.println(F("Vitesse +"));
    mySerial.println(F("Vitesse +"));
    if (vitesse < 91){
      vitesse = vitesse+10; // on augmente par palier de 10
      vitesse_e = vitesse;
      vitesse_i = vitesse-30;

      // Si on a dépassé les 91% on force à 91%
      if (vitesse > 91){
        vitesse = 91;
        vitesse_e = vitesse;
        vitesse_i = vitesse-30;
      }
      rpm_e = get_speed(int(vitesse_e));
      rpm_i = get_speed(int(vitesse_i));
      Serial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      mySerial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      Serial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
      mySerial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
    }
    else{
      Serial.println("Vitesse maximum atteinte !");
      mySerial.println("Vitesse maximum atteinte !");
    }

    mode = 0;
  }
    
  else if (message == CMD_BUTTON_M) {
    Serial.println(F("Vitesse -"));
    mySerial.println(F("Vitesse -"));
    if (vitesse > 35 && vitesse_i > 5){
      vitesse = vitesse-10; // on diminue par palier de 10
      vitesse_e = vitesse;
      vitesse_i = vitesse-30;
      rpm_e = get_speed(int(vitesse_e));
      rpm_i = get_speed(int(vitesse_i));
      Serial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      mySerial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      Serial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
      mySerial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
    }
    else{
      vitesse = 35;
      vitesse_e = vitesse;
      vitesse_i = vitesse-30;
      Serial.println(F("Vitesse minimum atteinte !"));
      mySerial.println(F("Vitesse minimum atteinte !"));
    }  

    mode = 0;
  }

  else if (message == CMD_BUTTON_2) {
    Serial.println(F("Vitesse + extraction"));
    mySerial.println(F("Vitesse + extraction"));
    if (vitesse_e < 91){
      vitesse_e = vitesse_e+10; // on augmente par palier de 10

      // Si on a dépassé les 91% on force à 91%
      if (vitesse_e > 91){
        vitesse_e = 91;
      }
      rpm = get_speed(int(vitesse_e));
      Serial.println((String)"Nouvelle vitesse extraction : "+rpm+" rpm.");
      mySerial.println((String)"Nouvelle vitesse extraction : "+rpm+" rpm.");
    }
    else{
      Serial.println("Vitesse maximum d'extraction atteinte !");
      mySerial.println("Vitesse maximum d'extraction atteinte !");
    }

    mode = 0;
  }

  else if (message == CMD_BUTTON_3) {
    Serial.println(F("Vitesse - extraction"));
    mySerial.println(F("Vitesse - extraction"));
    if (vitesse_e > 35){
      vitesse_e = vitesse_e-10; // on diminue par palier de 10

      // Si on a dépassé les 35% on force à 35%
      if (vitesse_e < 35){
        vitesse_e = 35;
      }
      
      rpm = get_speed(int(vitesse_e));
      Serial.println((String)"Nouvelle vitesse extraction : "+rpm+" rpm.");
      mySerial.println((String)"Nouvelle vitesse extraction: "+rpm+" rpm.");
    }
    else{
      vitesse_e = 35;
      Serial.println(F("Vitesse minimum d'extraction atteinte !"));
      mySerial.println(F("Vitesse minimum d'extraction atteinte !"));
    }  

    mode = 0;
  }

  else if (message == CMD_BUTTON_4) {
    Serial.println(F("Vitesse + insuflation"));
    mySerial.println(F("Vitesse + insuflation"));
    if (vitesse_i < 91){
      vitesse_i = vitesse_i+10; // on augmente par palier de 10

      // Si on a dépassé les 91% on force à 91%
      if (vitesse_i > 91){
        vitesse_i = 91; //~ 3003 RPM
      }
      rpm = get_speed(int(vitesse_i));
      Serial.println((String)"Nouvelle vitesse insuflation : "+rpm+" rpm.");
      mySerial.println((String)"Nouvelle vitesse insuflation : "+rpm+" rpm.");
    }
    else{
      Serial.println("Vitesse maximum d'insuflation atteinte !");
      mySerial.println("Vitesse maximum d'insuflation atteinte !");
    }

    mode = 0;
  }

  else if (message == CMD_BUTTON_5) {
    Serial.println(F("Vitesse - insuflation"));
    mySerial.println(F("Vitesse - insuflation"));
    if (vitesse_i > 5){
      vitesse_i = vitesse_i-10; // on diminue par palier de 10
      
      // Si on a dépassé les 5% on force à 5%
      if (vitesse_i < 5){
        vitesse_i = 5;
      }
      
      rpm = get_speed(int(vitesse_i));
      Serial.println((String)"Nouvelle vitesse insuflation : "+rpm+" rpm.");
      mySerial.println((String)"Nouvelle vitesse insuflation: "+rpm+" rpm.");
    }
    else{
      vitesse_i = 5; //~ 165 RPM
      Serial.println(F("Vitesse minimum d'insuflation atteinte !"));
      mySerial.println(F("Vitesse minimum d'insuflation atteinte !"));
    }  

    mode = 0;
  }
    
  else if (message == CMD_BUTTON_6) {
    if (digitalRead(OUT_FAN_PIN) == HIGH) {
      Serial.println(F("Bypass extraction."));
      mySerial.println(F("Bypass extraction."));
      digitalWrite(OUT_FAN_PIN, LOW);
    }
    else if (digitalRead(OUT_FAN_PIN) == LOW) {
      Serial.println(F("Marche extraction."));
      mySerial.println(F("Marche extraction."));
      digitalWrite(OUT_FAN_PIN, HIGH);
    }

    mode = 0;
  }
    
  else if (message == CMD_BUTTON_7) {
    if (digitalRead(IN_FAN_PIN) == HIGH) {
      Serial.println(F("Bypass insuflation."));
      mySerial.println(F("Bypass insuflation."));
      digitalWrite(IN_FAN_PIN, LOW);
    }
    else if (digitalRead(IN_FAN_PIN) == LOW) {
      Serial.println(F("Marche insuflation."));
      mySerial.println(F("Marche insuflation."));
      digitalWrite(IN_FAN_PIN, HIGH);
    }

    mode = 0;
  }

  else if (message == CMD_BUTTON_S) {
      
    if (vitesse != 70 && vitesse_i != (70-30)) {
      Serial.println(F("Mode silencieux"));
      mySerial.println(F("Mode silencieux"));

      vitesse = 70;
      vitesse_e = vitesse;
      vitesse_i = vitesse-30;
      rpm_e = get_speed(int(vitesse_e));
      rpm_i = get_speed(int(vitesse_i));
      Serial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      Serial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
      mySerial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      mySerial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");             
    }
    
    else {
      Serial.println(F("Mode silencieux déjà activé"));
      mySerial.println(F("Mode silencieux déjà activé"));
    }
      
    mode = 2;
  }

  else if (message == CMD_BUTTON_B) {
    if (digitalRead(OUT_FAN_PIN) == LOW && digitalRead(IN_FAN_PIN) == LOW) {
      digitalWrite(OUT_FAN_PIN, HIGH);
      digitalWrite(IN_FAN_PIN, HIGH);
      }
    if (vitesse != 95) {
      Serial.println(F("Mode boost"));
      mySerial.println(F("Mode boost"));
    }
    else {
      Serial.println(F("Mode boost déjà activé"));
      mySerial.println(F("Mode boost déjà activé"));
    }
    
    vitesse = 95;
    vitesse_e = vitesse;
    vitesse_i = vitesse-30;
    rpm_e = get_speed(int(vitesse_e));
    rpm_i = get_speed(int(vitesse_i));
    Serial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
    mySerial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
    Serial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
    mySerial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
    mode = 0;
  }

  else if (message == CMD_BUTTON_INFO) {
    Serial.println(F("Etat de la ventilation"));
    Serial.println((String)"Mode : "+mode);
    mySerial.println(F("Etat de la ventilation"));
    mySerial.println((String)"Mode : "+mode);

    // Affichage du mode de fonctionnement
    if (mode == -1) {
      Serial.println(F("La VMC est à l'arrêt."));
      mySerial.println(F("La VMC est à l'arrêt."));
    }
    else if (mode == 0) {
      Serial.println(F("La VMC est en mode manuel."));
      mySerial.println(F("La VMC est en mode manuel."));
    }
    else if (mode == 1) {
      Serial.println(F("La VMC est en mode automatique."));
      mySerial.println(F("La VMC est en mode automatique."));
    }
    else if (mode == 2) {
      Serial.println(F("La VMC est en mode silencieux."));
      mySerial.println(F("La VMC est en mode silencieux."));
    }

    // Affichage du fonctionnement de la ventillation
    if (digitalRead(OUT_FAN_PIN) == LOW && digitalRead(IN_FAN_PIN) == LOW) {
      Serial.println(F("La VMC est à l'arrêt car la ventilation est en bypass."));
      mySerial.println(F("La VMC est à l'arrêt car la ventilation est en bypass."));
    }      
    else if (digitalRead(IN_FAN_PIN) == LOW) {
      Serial.println(F("L'insufflation est en bypass."));
      mySerial.println(F("L'insufflation est en bypass."));
      rpm_e = get_speed(int(vitesse_e));
      Serial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      mySerial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
    }
    else if (digitalRead(OUT_FAN_PIN) == LOW) {
      Serial.println(F("L'extraction est en bypass."));
      mySerial.println(F("L'extraction est en bypass."));
      rpm_i = get_speed(int(vitesse_i));
      Serial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
      mySerial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
    }
    else{
      rpm_e = get_speed(int(vitesse_e));
      rpm_i = get_speed(int(vitesse_i));
      Serial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      mySerial.println((String)"Vitesse d'extraction : "+rpm_e+" rpm.");
      Serial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
      mySerial.println((String)"Vitesse d'insufflation : "+rpm_i+" rpm.");
    }

    // Affichage de l'hygrométrie/température moyenne et l'hygrométrie/température extérieure
    temp_moy = (getTemp(dht2) + getTemp(dht3))/2;
    Serial.println((String)"Température moyenne : "+float(temp_moy)+" °C.");
    mySerial.println((String)"Température moyenne  : "+float(temp_moy)+" °C.");
    
    hygrometrie_moy = getHumidity(2);
    Serial.println((String)"Hygrométrie moyenne : "+int(hygrometrie_moy)+" %.");
    mySerial.println((String)"Hygrométrie moyenne : "+int(hygrometrie_moy)+" %.");
    
    ext_temp = getTemp(dht1);
    Serial.println((String)"Température extérieure : "+float(ext_temp)+" °C.");
    mySerial.println((String)"Température extérieure  : "+float(ext_temp)+" °C.");
    if (ext_temp < 1) {
      mySerial.println((String)"Température extérieur trop faible pour démarrer la VMC ("+float(ext_temp)+" °C)");
    }

    ext_hygro = getHumidity(3);
    Serial.println((String)"Hygrométrie extérieure : "+int(ext_hygro)+" %.");
    mySerial.println((String)"Hygrométrie extérieure : "+int(ext_hygro)+" %.");
  }

  else if (message == 'h') {
    Serial.println(F("Manuel d'utilisation"));
    mySerial.println(F("Manuel d'utilisation"));
    mySerial.println(F("0 : ON/OFF"));
    mySerial.println(F("1 : Mode automatique"));
    mySerial.println(F("+ : Vitesse +"));
    mySerial.println(F("- : Vitesse -"));
    mySerial.println(F("2 : Vitesse + extraction"));
    mySerial.println(F("3 : Vitesse - extraction"));
    mySerial.println(F("4 : Vitesse + insuflation"));
    mySerial.println(F("5 : Vitesse - insuflation"));
    mySerial.println(F("6 : Bypass extraction"));
    mySerial.println(F("7 : Bypass insuflation"));
    mySerial.println(F("s : Mode silencieux"));
    mySerial.println(F("b : Mode boost"));
    mySerial.println(F("i : Etat de la ventilation"));
  }
    
  else if (receive == true && message != 'a') {
    Serial.println((String)"valid msg : "+message);
    if (message != 0 || message != 1 || message != 2 || message != 3 || 
    message != 4 || message != 5 || message != 6 || message != 7 || message != 'h' || message != '\r' || 
    message != '\n' || message != 's' || message != 'b') {
      Serial.println(F("La commande n'existe pas, envoyer 'h' pour recevoir le manuel d'utilisation"));
      mySerial.println(F("La commande n'existe pas, envoyer 'h' pour recevoir le manuel d'utilisation"));
    }
  }
       
  // changement de vitesse
  if (old_vitesse_e != vitesse_e) {
    Serial.println((String)"vitesse d'extraction actuelle : "+old_vitesse_e+" %.");
    // set la ventilation
    Serial.println((String)"changement de vitesse d'extraction à : "+int(vitesse_e)+" %.");
    pinMode(OUT_PWM_PIN, OUTPUT);
    analogWrite25k(OUT_PWM_PIN, int(vitesse_e*3.2));

    old_vitesse_e = vitesse_e;
  }
  if (old_vitesse_i != vitesse_i) {
    Serial.println((String)"vitesse d'insuflation actuelle : "+old_vitesse_i+" %.");
    // set la ventilation
    Serial.println((String)"changement de vitesse d'insuflation à : "+int(vitesse_i)+" %.");
    pinMode(IN_PWM_PIN, OUTPUT);
    analogWrite25k(IN_PWM_PIN, int(vitesse_i*3.2));

    old_vitesse_i = vitesse_i;

  }

  // Gestion du on/off des ventillateurs pour les modes 1 et 2
  // vitesse_i et vitesse_e sont mis à 0 dans ces modes si ext_temp < 1
  if (mode == 1 || mode == 2){
    // insufflation
    if (vitesse_i == 0 && digitalRead(IN_FAN_PIN) == HIGH) {
      digitalWrite(IN_FAN_PIN, LOW);
    }
    // pour rallumer le ventilateur 
    if (vitesse_i > 0 && digitalRead(IN_FAN_PIN) == LOW) {
      digitalWrite(IN_FAN_PIN, HIGH);
    }
  
    // extraction
    if (vitesse_e == 0 && digitalRead(OUT_FAN_PIN) == HIGH) {
      digitalWrite(OUT_FAN_PIN, LOW);
    }
    // pour rallumer le ventilateur 
    if (vitesse_e > 0 && digitalRead(OUT_FAN_PIN) == LOW) {
      digitalWrite(OUT_FAN_PIN, HIGH);
    }
  }

  delay(5000);

}

// Fonction msgFilter(raw_message) pour filter les commandes reçu, retourne un message filtré
char msgFilter(char raw_message) {
//  char okchar[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  char message = 'a';

  Serial.println((String)"defaut msg : "+message);
  Serial.println((String)"raw_message : "+raw_message);
  Serial.println((String)"strlen(raw_message) : "+strlen(raw_message));

//  if (strlen(raw_message) == 1) { // not working
  if (isalpha(raw_message) || isdigit(raw_message) || raw_message == '-' || raw_message == '+') {
    message = char(raw_message);
  }

  Serial.println((String)"return msg : "+message);
  
  return message;
}

// Fonction getTemp(sonde) pour lire la température de la sonde en paramètre
// Retourne la température de la sonde
int getTemp(DHT sonde) {
  float temp = 0;

  temp = sonde.readTemperature();
  
  return temp;
}

// Fonction getHumidity(mode) pour lire la moyenne d'humidité des 3 sondes
// Retourne res ou des variables d'humidité en fonction du mode avec lequel la fonction est appellée
int getHumidity(int mode) {

  // attente de 5s pour lire les capteurs
  // delay(5000);
  
  float hygrometrie_moy = 0;
  int res = 1;

  // Obtenir l'humidité et la température de chaque sondes
  float h1 = dht1.readHumidity(); // sonde air extérieur
  float h2 = dht2.readHumidity();
  float h3 = dht3.readHumidity();
  
  //hygrometrie_moy = (h1+h2+h3)/3; // si 3 sondes
  hygrometrie_moy = (h2+h3)/2;

  Serial.println((String)"Hum : "+hygrometrie_moy+" %.");

  if (mode == 1){
    if (hygrometrie_moy>=53 && hygrometrie_moy<75) {
      res = 2;
    }
    else if (hygrometrie_moy>75) {
      res = 3;
    }
    else if (hygrometrie_moy<53) {
      res = 2; //sinon trop peu de renouvellement d'air
    }
    
    //Serial.println((String)"Hum 1 : "+h1+" %.");
    Serial.println((String)"Hum 2 : "+h2+" %.");
    Serial.println((String)"Hum 3 : "+h3+" %.");
    
    return res;
  }
  else if (mode == 2){ 
    return int(hygrometrie_moy);
  }
  else if (mode == 3){
    return int(h1);
  }
}

int automode(int res){
  if (res==1){
    vitesse = 50; // vitesse à 50%
  }
  else if (res==2){
    vitesse = 79; // vitesse à 79%
  }
  else if (res==3){
    vitesse = 91; // vitesse à 91%
  }
  return vitesse;
}

// see https://arduino.stackexchange.com/questions/25609/set-pwm-frequency-to-25-khz
// Edgar Bonet post
void analogWrite25k(int pin, int value) {
    switch (pin) {
        case 9:
            OCR1A = value;
            break;
        case 10:
            OCR1B = value;
            break;
        default:
            // no other pin will work
            break;
    }
}

// Fonction get_speed(vitesse) : prend une vitesse en % en input et retourne sa correspondance en RPM
// Basée sur les spécifications du ventilateur Arctic P12 Max
int get_speed(float vitesse) {
  float max_rpm = 3300; // 100% PWM
  float min_rpm = 200; // 5% PWM
  float rpm = 0;

//  Serial.println((String)"vitesse input : "+vitesse+" %.");
//  rpm = ((vitesse*3.2)*max_rpm)/ICR1; // Methode 1
  rpm = (vitesse*max_rpm)/100; // Méthode 2 qui conduit au même résultat
//  Serial.println((String)"rpm : "+rpm);
  if (rpm > max_rpm) {
    Serial.println((String)"Changing rpm result from "+rpm+" to "+max_rpm);
    rpm = max_rpm;
  }
  else if (rpm < min_rpm) {
    Serial.println((String)"Changing rpm result from "+rpm+" to "+min_rpm);
    rpm = min_rpm;
  }
  return int(rpm);
}
