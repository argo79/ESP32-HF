/*
  ESP32-IoT Angelini Test
  Trasmissione bluetooth del valore del sensore PIR
  Ricezione comando accensione/spegnimento led
  Ricezione comando impostazione soglia PIR
  Ricezione comando modalità: acceso, spento, rileva movimento
*/

#define builtInLed 13
#define pinPir 14
#define nomeClient        "IoTAngeliniBT"

#include <WiFi.h>
#include <PubSubClient.h>
#include "BluetoothSerial.h"

//////////////////////////////////
//           WIFI               //
//////////////////////////////////
const char* ssid = "blekgek-2";
const char* password = "radicofani27";
const char* mqtt_server = "93.40.0.250";

// Creazione oggetti

WiFiClient esp32Client;
PubSubClient client(esp32Client);
WiFiUDP Udp;


//////////////////////////////////
//          BLUETOOTH           //
//////////////////////////////////

BluetoothSerial SerialBT;
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

bool statoPir;
int mode;
int sogliaPir=500;
String dataBT;
char serBT;
char valore[10];


void setup() {
  // put your setup code here, to run once:
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

  pinMode(builtInLed, OUTPUT);
  pinMode(pinPir, INPUT);
  digitalWrite(builtInLed, 0);  // turn the LED on (HIGH is the voltage level)
}

void loop() {
  // put your main code here, to run repeatedly:
  switch (mode) {
    case 0:      
      digitalWrite(builtInLed, 0);  // turn the LED on (HIGH is the voltage level)
      break;

    case 1:
      digitalWrite(builtInLed, 1);  // turn the LED on (HIGH is the voltage level)
      break;

    case 2:
      digitalWrite(builtInLed, 0);
      statoPir=digitalRead(pinPir);
      if (statoPir>0) {
        digitalWrite(builtInLed, 1);
        SerialBT.print("Il sensore si è attivato! ");
        SerialBT.println(statoPir);
        delay(sogliaPir);
        digitalWrite(builtInLed, 0);

      }
      else digitalWrite(builtInLed, 0);
      break;
    
    default:
      //digitalWrite(builtInLed, 0);
      break;
  }  

  BTSerialRicevi();                     // RICEVO MESSAGGI BLUETOOTH

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
        SerialBT.print("Led acceso! ");    
        SerialBT.println(mode);           
        snprintf (valore, 10, "%d", 0);
        //client.publish(allarme_topic, valore);     
      } 

      else if (dataBT.startsWith("spegni")) {                           
        mode=0;
        SerialBT.print("Led spento! ");    
        SerialBT.println(mode);   
        snprintf (valore, 10, "%d", 0);
        //client.publish(allarme_topic, valore);          
      } 

      else if (dataBT.startsWith("alarm")) {                           
        mode=2;
        //statoLed=0;
        SerialBT.println("Allarme attivato!");   
        snprintf (valore, 10, "%d", 1);
        //client.publish(allarme_topic, valore);      
        
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
  SerialBT.print("Modalità: ");
  SerialBT.println(mode);
  SerialBT.print("Sensore PIR: ");
  SerialBT.println(statoPir);
  SerialBT.println("");    

}
