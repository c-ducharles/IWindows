#include <Arduino.h>
#include <string.h>
#include <ESP8266WiFi.h>
#include <MQTT.h>
MQTTClient client;
WiFiClient wificlient;
/* SSID du réseaux Wifi */
const char ssid[] = "Mi9T";
const char pass[] = "";
/* Addresse IP du Broker MQTT (Mosquitto) */
const char *mqtt_broker = "mqtt.fluux.io";
const char *topic = "esp8266/test";
const char *mqtt_username = "fluux";
const char *mqtt_password = "public";
const int mqtt_port = 1883;
/* Topics MQTT */
const char* mqqtTopicIN_cible = "UTBM/IWindows/cible";
const char* mqqtTopicIN_action_manuel = "UTBM/IWindows/action_manuel";
const char* mqqtTopicOUT_Meteo = "UTBM/IWindows/Meteo";
const char* mqqtTopicIN_node_mode = "UTBM/IWindows/node_mode";
const char* mqqtTopicIN_alarme = "UTBM/IWindows/rearmer_alarme";
int temp=0,humi=0,lux=0;
char porte='0',volet='0';

void connect() {
  Serial.println("[DEBUG#ESP Checking wifi...]");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("[DEBUG#ESP Wifi connected, MQTT connecting...]");
  while (!client.connect("BONZI", "try", "try")) {
    Serial.println("+");
    delay(100);
  }
  Serial.println("[DEBUG#ESP MQTT connected!]");
  client.subscribe(mqqtTopicIN_cible);
  client.subscribe(mqqtTopicIN_action_manuel);
  client.subscribe(mqqtTopicIN_node_mode);
  client.subscribe(mqqtTopicIN_alarme);
}
//----------------------recevoir et envoyer
void messageReceived(String &topic, String &payload) {
  if (topic == "UTBM/IWindows/cible"){
    Serial.println(payload);
  }
  else if(topic=="UTBM/IWindows/node_mode"){
    Serial.print("[m#");
    Serial.print(payload);
    Serial.print("#]");
  }
  else if (topic == "UTBM/IWindows/action_manuel"){
    if (payload == "P"){
      Serial.print("[");
      Serial.print(payload);
      Serial.println("#]");// un # après
    }
    else if (payload == "V"){
      Serial.print("[");
      Serial.print(payload);
      Serial.println("#]");// un # après
    }
  }
  else if (topic == "UTBM/IWindows/rearmer_alarme"){
        Serial.print("[a#");
        Serial.print(payload);
        Serial.println("#]");// un # après
  }
}
  /*
  Serial.print("[DEBUG#ESP : "); 
  Serial.print(topic);
  Serial.print(" - "); 
  Serial.print(payload);
  Serial.println("]");*/

String inString;
void serialEvent() {
  while (Serial.available()) {
    
    char inChar = Serial.read();
    inString += inChar;
    if (inChar == ']') { 
      client.publish(mqqtTopicOUT_Meteo, inString);
      inString = "";//reset de la string jusqu'à la prochaine commande
    }
  }
  
}
unsigned long lastMillis = 0;
void alive() {
  // publish a message roughly every second.
  if (millis() - lastMillis > 10000) {
    lastMillis = millis();
    client.publish("UTBM/IWindows/esp/aliveAntoine", "coucou");
  }
}
void setup() {
  // Initialisation des ports et sorties (ESP envoie/reçoit vers ATMega avec serial, ATMega envoie/Reçoit vers ESP avec serial3)
  Serial.begin(115200);//vers ATMega
 
  Serial.println("[DEBUG#ESP Welcome]");
  WiFi.begin(ssid, pass);
  
  client.begin(mqtt_broker, 1883, wificlient);
  client.onMessage(messageReceived);
  connect();
  //---envoi des valeurs initiales
  client.publish("UTBM/IWindows/temp_cible", "20");
  client.publish("UTBM/IWindows/humi_cible", "40");
  client.publish("UTBM/IWindows/lux_cible", "200");
  client.publish("UTBM/IWindows/rearmer_alarme", "0");
}
void loop() {
  client.loop();
  if (!client.connected()) {
    connect();
  }
  serialEvent();
  alive();
}