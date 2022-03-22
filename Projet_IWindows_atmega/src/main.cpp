//---------------------------------------------Libs--------------------------------------------------------------//
#include <Arduino.h>
#include <Stepper.h>
#include <stdio.h>
#include "DHT.h"
//OLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_Sensor.h>
//---------constantes-----------//
#define pin_buzzer 25
#define pin_c_mvmt 23
#define pin_b_alarme 27
#define pin_b_auto 47
#define pin_b_manuel 49
#define pin_b_volet 51
#define pin_b_porte 53
#define DHTType DHT11
#define DHT11Pin 10
#define DHT11ExtPin 11
#define DELAY 500 // Delay between two measurements in ms
#define VIN 5 // V power voltage
#define R 5000 //ohm resistance value
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
//---------variables----------//
int etat_porte = 0; // porte fermee par defaut
int etat_volet = 0; // volet ferme par defaut
int bouton_manuel = 0;
int bouton_volet = 0;
int bouton_porte = 0;
int bouton_auto = 0;
int bouton_alarme= 0;
int rearmer_alarme=0;
int etat_alarme=0;
int chgmnt_mode = 0;
int mode = 1; //1 manuel/ 2 auto / 3 alarme
int node_mode = 1;
const int sensorPin = A0; // Pin connected to sensor
//Variables
int sensorVal; // Analog value from the sensor
float lux=0,humi=0,tempC=0,humiExt=0,tempCExt=0;
float lux_moy=0,humi_moy=0,tempC_moy=0,humiExt_moy=0,tempCExt_moy=0;
float lux_sum=0,humi_sum=0,tempC_sum=0,humiExt_sum=0,tempCExt_sum=0;
int nombre_pour_moyenne = 0;
int lux_cible=0, humi_cible=0,temp_cible=0;
int passage;
unsigned long lastMillis = 0;
unsigned long lastMillis_cible = 0;
// Les deux DHT11
DHT HT(DHT11Pin,DHTType);
DHT HTExt(DHT11ExtPin,DHTType);
// initialize the stepper library
Stepper stepper_porte(200, 2,4,3,5);
Stepper stepper_volet(200, 6,8,7,9);
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
//-------proto fonction----------//
void actionner(char type);
void alarme();
int check_mode();

//---------------------------------------Setup------------------------------------------------------//
void setup()
{
  Serial.begin(115200);
  Serial3.begin(115200);//vers l'esp8266
  pinMode(pin_b_manuel, INPUT);//bouton mode manuel
  pinMode(pin_b_porte, INPUT);//bouton action porte
  pinMode(pin_b_volet, INPUT);//bouton action volet
  pinMode(pin_b_auto, INPUT); //bouton mode automatique
  pinMode(pin_b_alarme, INPUT); //bouton mode alarme
  pinMode(pin_c_mvmt, INPUT);//capteur de mvmnt
  pinMode(pin_buzzer, OUTPUT); //sortie buzzer
  // set the speed at 5 rpm:
  stepper_porte.setSpeed(5);
  stepper_volet.setSpeed(100);
  //Pour DHT11
  HT.begin();
  HTExt.begin();
  //Pour OLED I2C
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Addresse 0x3D pour 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display(); 
  display.clearDisplay();
}

//---------------------------------------Fonction lisant arrivée esp----------------------------------//
void serialEvent3() {
  int fin = 0;
  char waste;
  String inString="";
  String topic;
  topic = inString.c_str();
  while (Serial3.available() and fin != 1) { 
    char inChar = Serial3.read();
    inString += inChar;
  }
  Serial.println(inString);
  topic = inString.c_str();
  if (topic[1] =='c'){
    sscanf(inString.c_str(),"[%c#%d#%d#%d#]",&waste,&temp_cible,&humi_cible,&lux_cible);
  }
  else if (topic[1] =='P'){
    actionner('P');
    //actionner porte
  }
  else if (topic[1] =='V'){
    actionner('V');
    //actionner volet
  }
  else if (topic[1] =='m'){
    //acquerir le mode
    sscanf(inString.c_str(),"[%c#%d#]",&waste,&node_mode);
  }
  else if (topic[1] =='a'){
    rearmer_alarme = 1;
  }
}

//----------------------------------Fonction convertion en lux-------------------------------//
int sensorRawToPhys(int raw){
  // Conversion rule
  float Vout = float(raw) * (VIN / float(1024));// Conversion analog to voltage
  float RLDR = (R * (VIN - Vout))/Vout; // Conversion voltage to resistance
  int phys=500/(RLDR/1000); // Conversion resitance to lumen
  return phys;
}

//----------------------------------Fonction affichage acceuil oled------------------------------//
void oledDisplayHeader(){
 display.setTextSize(1);
 display.setTextColor(WHITE);
 display.setCursor(0, 0);
 display.print("Humidite");
 display.setCursor(60, 0);
 display.print("Temperature");
 display.setCursor(30, 18);
 display.print("Interieur");
 display.setCursor(30, 33);
 display.print("Exterieur");
}

//----------------------------------Fonction affichage bas oled------------------------------//
void oledDisplayBottom(String mode_affichage){
 display.setTextSize(1);
 display.setTextColor(WHITE);
 display.setCursor(20, 50);
 display.print("mode : ");
 display.setCursor(60, 50);
 display.print(mode_affichage);
}

//----------------------------------Fonction affichage infos meteo-------------------------------//
void oledDisplay(int size, int x,int y, float value, String unit){
 int charLen=12;
 int xo=x+charLen*3.2;
 int xunit=x+charLen*3.6;
 int xval = x; 
 display.setTextSize(size);
 display.setTextColor(WHITE);
 
 if (unit=="%"){
   display.setCursor(x, y);
   display.print(value,0);
   display.setTextSize(size-1);
   display.print(unit);
 } else {
   if (value>99){
    xval=x;
   } else {
    xval=x+charLen;
   }
   display.setCursor(xval, y);
   display.print(value,0);
   display.drawCircle(xo, y+2, 2, WHITE);  // print degree symbols (  )
   display.setCursor(xunit, y);
   display.setTextSize(size-1);
   display.print(unit);
 }
}

//----------------------------------Fonction lecture infos meteo-------------------------------//
void meteo(){
if (millis() - lastMillis > 3000) {
     lastMillis = millis();
     humi = HT.readHumidity();
     tempC = HT.readTemperature();
     humiExt = HTExt.readHumidity();
     tempCExt = HTExt.readTemperature();
    
     display.clearDisplay();
     oledDisplayHeader();
     
     oledDisplay(1,0,20,humi,"%");
     oledDisplay(1,78,20,tempC,"C");
     oledDisplay(1,0,35,humiExt,"%");
     oledDisplay(1,78,35,tempCExt,"C");
     switch (mode) {
        case 1:
          oledDisplayBottom("manuel");
          break;
        case 2:
          oledDisplayBottom("auto");
          break;
         case 3:
          oledDisplayBottom("alarme");
          break;
        default:
          oledDisplayBottom("");
          break;
      }
     
     display.display();
    
     sensorVal = analogRead(sensorPin);
     lux=sensorRawToPhys(sensorVal);
     Serial3.print("[");
     Serial3.print(tempC);
     Serial3.print("#");
     Serial3.print(tempCExt);
     Serial3.print("#");
     Serial3.print(humi);
     Serial3.print("#");
     Serial3.print(humiExt);
     Serial3.print("#");
     Serial3.print(lux);
     Serial3.print("#");
     Serial3.print(etat_porte);
     Serial3.print("#");
     Serial3.print(etat_volet);
     Serial3.print("#");
     Serial3.print(mode);
     Serial3.print("#");
     Serial3.print(etat_alarme);
     Serial3.println("]");
 }
}

//-------------------------------Fontion loop, appelant les fonctions annexes de temps à autre------------------------//
void loop()
{
  meteo(); //---lance la fonction meteo qui capte les infos, affiche et envoie à l'esp 
  if (mode == 1) //mode manuel
  {
    bouton_porte = digitalRead(pin_b_porte); //bouton ouverture de porte manuel
    bouton_volet = digitalRead(pin_b_volet); //bouton ouverture volet manuel
    if (bouton_porte == HIGH)
      {
        actionner('P');
        delay(10); //debouncing
        //while(digitalRead(pin_b_porte) == HIGH);
      }
    else if (bouton_volet == HIGH)
      {
        actionner('V');
        delay(10); //debouncing
        //while(digitalRead(pin_b_volet) == HIGH);
      }
  } 
  else if (mode == 2) //mode auto
  {
    if ((millis() - lastMillis_cible) < 9000) {
      lux_sum += lux;
      tempC_sum += tempC;
      tempCExt_sum += tempCExt;
      humi_sum += humi;
      humiExt_sum += humiExt;
      nombre_pour_moyenne += 1;
    }
    else{
      lux_moy = lux_sum/nombre_pour_moyenne;
      tempC_moy = tempC_sum/nombre_pour_moyenne;
      tempCExt_moy = tempCExt_sum/nombre_pour_moyenne;
      humi_moy = humi_sum/nombre_pour_moyenne;
      humiExt_moy = humiExt_sum/nombre_pour_moyenne;
      // conditions ouvrir fenetre
      if ((tempC_moy < temp_cible and tempC_moy < tempCExt_moy and etat_porte == 0) or (tempC_moy > temp_cible and tempC_moy > tempCExt_moy and etat_porte == 0)){
        actionner('P');
      }
      //conditions fermer fenetre
      else if ((tempC < temp_cible and tempC_moy > tempCExt_moy and etat_porte == 1) or (tempC_moy > temp_cible and tempC_moy < tempCExt_moy and etat_porte == 1)){
        actionner('P');
      }
      //---fermer volet----
      if (lux_moy < lux_cible and etat_volet == 1){
        actionner('V');
      }
      //--ouverture volet----
      else if (lux_moy > lux_cible and etat_volet == 0){
        actionner('V');
       }
      lux_moy = 0;
      tempC_moy = 0;
      tempCExt_moy = 0;
      humi_moy = 0;
      humiExt_moy = 0;
      lux_sum =0;
      tempC_sum =0;
      tempCExt_sum =0;
      humi_sum =0;
      humiExt_sum =0;
      nombre_pour_moyenne = 0;
      lastMillis_cible = millis();
    }
  }
  else if (mode == 3) //mode alarme
  {
    if (etat_porte == 1) //si la porte est ouverte on la ferme
      {
        actionner('P');
      }
    if (etat_volet == 1) //si le volet est ouvert on le ferme
      {
        actionner('V');
      }
    alarme();
   }
  chgmnt_mode = check_mode();
  delay(100);
}

int check_mode()
{
  int action = 0; //stocke le bouton qui a été appuyé
  bouton_manuel=digitalRead(pin_b_manuel);//on recupere les infos des boutons
  bouton_auto=digitalRead(pin_b_auto);
  bouton_alarme=digitalRead(pin_b_alarme);
  if ((bouton_manuel == HIGH or node_mode == 1) and mode!=1)
  {
      display.clearDisplay();
        display.setCursor(0,0);
        display.setTextSize(2);
        display.print("Mode");
        display.setCursor(0,40);
        display.setTextSize(2);
        display.print("manuel");
        display.display();
      action = 1;
      delay(10); //debouncing
      while(digitalRead(pin_b_manuel) == HIGH);
  } 
  else if ((bouton_auto == HIGH or node_mode == 2) and mode!=2)
  {
      display.clearDisplay();
        display.setCursor(0,0);
        display.setTextSize(2);
        display.print("Mode");
        display.setCursor(0,40);
        display.setTextSize(2);
        display.print("auto");
        display.display(); 
      action = 2;
      delay(10); //debouncing
      while(digitalRead(pin_b_auto) == HIGH);
  } 
  else if (bouton_alarme == HIGH)
  {
    if (mode!= 3)
    {
      display.clearDisplay();
        display.setCursor(0,0);
        display.setTextSize(2);
        display.print("Mode");
        display.setCursor(0,40);
        display.setTextSize(2);
        display.print("alarme");
        display.display();
      action = 3;
      delay(10); //debouncing
      while(digitalRead(pin_b_alarme) == HIGH);
    }
    else
    {
      action = 3;
      rearmer_alarme = 1;//renvoi que le mode alarme a été appuye meme si l'on est deja dans ce mode (pour le reamorcement de l'alarme)
      delay(10); //debouncing
      while(digitalRead(pin_b_alarme) == HIGH);
    }
  }
  else if (node_mode == 3)
  {
    Serial.println("alarme");
    if (mode != 3)
    {
      Serial.println("mode alarme"); 
      action = 3;
      delay(10);
    } 
  }
  else if (rearmer_alarme==1)
    {
      Serial.println("check_alarme");
      action = 3;
    }
  
  if (action != 0)
  {
    mode = action;
    node_mode=0;
    if (mode != 3 && etat_alarme == 1)
    {
      etat_alarme = 0;
    }
    return action;
  }
  else
  {
    return 0;
  }
}

void actionner(char type)
{
    if (type=='P')
    {
     if (etat_porte==0) //si porte fermee
      {
        display.clearDisplay();
        display.setCursor(0,0);
        display.setTextSize(2);
        display.print("Ouverture");
        display.setCursor(0,40);
        display.setTextSize(2);
        display.print("fenetre");
        display.display();
        stepper_porte.step(+50);
        Serial.println("porte ouverte");
        etat_porte = 1;
        }
     else //si porte ouverte
       {
          display.clearDisplay();
          display.setCursor(0,0);
          display.setTextSize(2);
          display.print("Fermeture");
          display.setCursor(0,40);
          display.setTextSize(2);
          display.print("fenetre");
          display.display();
          stepper_porte.step(-50);
          Serial.println("porte fermee");
          etat_porte = 0;
        }   
    } else if (type=='V')
    {
      if (etat_volet==0) //si volet ferme
      {
        display.clearDisplay();
        display.setCursor(0,0);
        display.setTextSize(2);
        display.print("Ouverture");
        display.setCursor(0,40);
        display.setTextSize(2);
        display.print("volet");
        display.display();
        stepper_volet.step(+250);
        Serial.println("volet ouvert");
        etat_volet = 1;
        }
     else //si volet ouvert
       {
          display.clearDisplay();
          display.setCursor(0,0);
          display.setTextSize(2);
          display.print("Fermeture");
          display.setCursor(0,40);
          display.setTextSize(2);
          display.print("volet");
          display.display();
          stepper_volet.step(-250);
          Serial.println("volet ferme");
          etat_volet = 0;
        }   
    }
}

void alarme()
{                                                                     
  int detecteur = 0;
  if (chgmnt_mode==3 && rearmer_alarme==1)
  {
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(2);
    display.print("Rearmement");
    display.setCursor(0,40);
    display.setTextSize(2);
    display.print("alarme");
    display.display();
    noTone(pin_buzzer);
    rearmer_alarme = 0;
    etat_alarme = 0;
    delay(20);
  }
  
  if (chgmnt_mode==3 && rearmer_alarme == 0)
  {
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(2);
    display.print("Demarrage");
    display.setCursor(0,40);
    display.setTextSize(2);
    display.print("alarme");
    display.display();
    delay(1000);
    while(digitalRead(pin_c_mvmt)==HIGH); //debouncing
    for (int k = 0; k < 2; k += 1)
    {
      tone(pin_buzzer,100);
      delay(200);
      noTone(pin_buzzer);
      delay(200);
    }
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(2);
    display.print("Alarme");
    display.setCursor(0,40);
    display.setTextSize(2);
    display.print("armee !");
    display.display();
  }

  if (chgmnt_mode==3 && rearmer_alarme==1)
  {
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(2);
    display.print("Rearmement");
    display.setCursor(0,40);
    display.setTextSize(2);
    display.print("alarme");
    display.display();
    noTone(pin_buzzer);
    rearmer_alarme = 0;
    etat_alarme = 0;
    delay(20);
  }
  
  detecteur = digitalRead(pin_c_mvmt);
  if (detecteur == HIGH && etat_alarme == 0)
    {
    etat_alarme = 1;
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(2);
    display.print("Intrusion");
    display.setCursor(0,40);
    display.setTextSize(2);
    display.print("detectee");
    display.display();
    }
   
 if (etat_alarme == 1)
  {
    tone(pin_buzzer,500);
    delay(5);
    tone(pin_buzzer,1000);
    delay(5);
    noTone(pin_buzzer);
   }
 }