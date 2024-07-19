/*
  ESP32-IoT Angelini Test
  Trasmissione bluetooth del valore del sensore PIR
  Ricezione comando accensione/spegnimento led
  Ricezione comando impostazione soglia PIR
  Ricezione comando modalità: acceso, spento, rileva movimento
*/

#define builtInLed 13
#define pinPir 14

#include <WiFi.h>
#include <PubSubClient.h>
#include "BluetoothSerial.h"
// OTA
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// OTA

//////////////////////////////////
//           WIFI               //
//////////////////////////////////
const char* ssid = "RRP-24-ARG0";
const char* password = "1234567890";
const char* mqtt_server = "192.168.100.188";

//////////////////////////////////
//         PARAMETRI            //
//////////////////////////////////
// Configurazione Mosquitto

#define nomeClient        "nome ***"

#define Messaggio_topic   "Hfarm/IoT/messaggio"
#define statoLed_topic    "Hfarm/IoT/statoLed"
#define sogliaPir_topic   "Hfarm/IoT/sogliaPir"
#define allarme_topic     "Hfarm/IoT/allarme"

// Creazione oggetti

WiFiClient esp32Client;
PubSubClient client(esp32Client);
WiFiUDP Udp;
BluetoothSerial SerialBT;
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

bool statoLed=0;
bool statoLedOld;
int sogliaPir=500;
// Variabili comunicazione
String dataPC;
String dataBT;
char serPC;
char serBT;
String dataMQTT;
int sensorePir=518;
int statoPir;
int mode=0;
int allarme;
char valore[10];


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  Serial.begin(115200);

// connessione bluetooth
  SerialBT.begin(nomeClient);                   //Bluetooth device name
  SerialBT.println("Bluetooth avviato!");
  delay(100);

// connessione alla rete wifi  
  SerialBT.print("Connecting to ");
  SerialBT.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    SerialBT.print(".");
  }


  
// Print local IP address and start web server
  sendBTWifi();

// Avvio la connessione con il server mqtt (mosquitto)
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(builtInLed, OUTPUT);
  pinMode(pinPir, INPUT);
  digitalWrite(builtInLed, 0);  // turn the LED on (HIGH is the voltage level)
}


// the loop function runs over and over again forever
void loop() {
 

  
  if (!client.connected()) {
    reconnect();
  }  


  BTSerialRicevi();                     // RICEVO MESSAGGI BLUETOOTH


  client.loop();
}


void callback(char* topic, byte* payload, unsigned int length) {        // Ricezione messaggi MQTT dal broker
  
  String response;  
  for (int i = 0; i < length; i++) {
    response+=(char)payload[i];
  }
  
  String topicS=topic;
  //SerialBT.println(topicS);  
  
  if (topicS=="Hfarm/IoT/statoLed") {
    //SerialBT.println(response);
    statoLedOld=statoLed;    
    statoLed=response.toInt();
    if (statoLed==0) {
      mode=0;
      snprintf (valore, 10, "%d", 0);
      client.publish(allarme_topic, valore);
    }    
    else if (statoLed==1) {
      mode=1;
      snprintf (valore, 10, "%d", 0);
      client.publish(allarme_topic, valore);
    }
  }
  else if (topicS=="Hfarm/IoT/sogliaPir") {
    sogliaPir=response.toInt();
    SerialBT.println(response);
    SerialBT.println(sogliaPir);
  }  
  else if (topicS=="Hfarm/IoT/allarme") {
    allarme=response.toInt();
    if (allarme==1) {
      mode=2;      
    }
    else if (allarme==0) {
      //mode=0;  
    }
    
  }  
}

void reconnect() {                            // CONNESSIONE BROKER MQTT
  // Loop until we're reconnected
  while (!client.connected()) {
    SerialBT.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(nomeClient)) {
      SerialBT.println("connected");      
      client.subscribe(Messaggio_topic,1);
      client.subscribe(statoLed_topic,1);
      client.subscribe(sogliaPir_topic,1);
      client.subscribe(allarme_topic,1);
      
    } else {
      SerialBT.print("failed, rc=");
      SerialBT.print(client.state());
      SerialBT.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      yield();
      delay(1000);
    }
  }
}


void sendBTWifi() {
  SerialBT.println("");
  SerialBT.println("WiFi connected.");
  SerialBT.println("IP address: ");
  SerialBT.println(WiFi.localIP());
}


void BTSerialRicevi() {                    // GESTIONE CONTROLLO BLUETOOTH
  if (SerialBT.available()) {
    
    while (SerialBT.available()) {
      serBT=SerialBT.read();
      dataBT+=serBT;
      yield();        
    }          
    //Serial.println(dataBT); 
    dataBT.trim();
    
    if (dataBT.length()>0) {
          
      if (dataBT=="reset") {      
        //restartESP32();  
        }             
      
      else if (dataBT.startsWith("accendi")) {                           
        mode=1;
        statoLed=1;
        SerialBT.print("Led acceso! ");    
        SerialBT.println(statoLed);   
        snprintf (valore, 10, "%d", 0);
        client.publish(allarme_topic, valore);     
      } 

      else if (dataBT.startsWith("spegni")) {                           
        mode=0;
        statoLed=0;
        SerialBT.print("Led spento! ");    
        SerialBT.println(statoLed);   
        snprintf (valore, 10, "%d", 0);
        client.publish(allarme_topic, valore);          
      } 

      else if (dataBT.startsWith("alarm")) {                           
        mode=2;
        //statoLed=0;
        SerialBT.println("Allarme attivato!");   
        snprintf (valore, 10, "%d", 1);
        client.publish(allarme_topic, valore);      
        
      } 


      else if (dataBT.startsWith("sogPir")) {                    
        String sogliaPirS=dataBT.substring(6);
        sogliaPir=sogliaPirS.toInt();     
        SerialBT.print("Soglia Pir: ");    
        SerialBT.println(sogliaPir);
      }
      
      else if (dataBT.startsWith("help")) {                       // HELP -> Comando        
        SerialBT.println("Comandi:"); 
        SerialBT.println("");
        SerialBT.println("reset          -> Resetta Esp32");    
        SerialBT.println("accendi        -> Accende il led");        
        SerialBT.println("spegni         -> Spegne il led");   
        SerialBT.println("alarm          -> Attiva il sensore");          
        SerialBT.println("sogPir(50-4000)-> Soglia sensore PIR");  
        SerialBT.println("info           -> Mostra valori attuali");              
        SerialBT.println("help           -> Questo comando.");
        SerialBT.println("");
        //delay(50);       
      }

      else if (dataBT.startsWith("info")) {                       // HELP -> Comando                
        sendBTWifi();
        printValues();
      }

      else {
        SerialBT.println("Valore sbagliato. Nessun comando riconosciuto.");          
      }
    }
  }
    dataBT="";
    //delay(1);
}


void printValues() {                        // LEGGE TUTTI I VALORI DEI SENSORI
    
  SerialBT.println("Valori attuali:");
  SerialBT.println("");  
  SerialBT.print("Soglia sensore PIR: ");
  SerialBT.println(sogliaPir);  
  SerialBT.print("Stato led: ");
  SerialBT.println(statoLed);
  SerialBT.print("Sensore PIR: ");
  SerialBT.println(sensorePir);
  SerialBT.println("");    

}

