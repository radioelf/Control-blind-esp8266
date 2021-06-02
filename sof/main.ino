/*
Control de relés para el control de persianas, y Home Assistant a través de MQTT																		   
 
 Radioelf - mayo 2021
 * 
  http://radioelf.blogspot.com.es/

  Copyright (c) 2020 Radioelf.  All rights reserved.
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
  
23/5/21 Ver: 0.1.9 

*/
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>                                                        // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266HTTPClient
#include <PubSubClient.h>                                                             // https://github.com/knolleary/pubsubclient/releases/tag/v2.8
#include <Ticker.h>
#include <EEPROM.h>

const uint8_t FC_1 = 5;                                                               // Pin final carrera persiana 1 activado =1, reposo =0
#define FC2 4                                                                         // Pin final carrera persiana 2 activado =1, reposo =0
uint8_t FC_2 = FC2;                                                                   // Puede cambiar
const uint8_t DHTPin = 2;                                                             // Pin data DHT11
const uint8_t Up1Pin = 12;                                                            // Pin subir persiana 1 ON->1, OFF ->0
const uint8_t Down1Pin = 13;                                                          // Pin bajar persiana 1 ON->1, OFF ->0
const uint8_t Up2Pin = 14;                                                            // Pin subir persiana 2 ON->1, OFF ->0
const uint8_t Down2Pin = 16;                                                          // Pin bajar persiana 2 ON->1, OFF ->0

#define DebugSerial 0                                                                 // Debug 

#define NameLocation 3                                                                // Localización persiana

#define AllowHandbook 1                                                               // Permitimos modo automático/manual simultaneo (para pulsadores manuales)

#define DHTsensor 0                                                                   // Soporte para sensor DHT11

#define ForceEEPROM 0                                                                 // Forzamos escribir datos en Eepron

#define mDNS 0                                                                        // Multicast DNS

#define UPTIME_OVERFLOW  4294967295UL                                                 // Uptime overflow

#if AllowHandbook
#warning ATENCION: Se permite el uso simultaneo del control automático y manual (usar pulsadores)
#endif

#define alexaSuport 0                                                                 // Soporte para alexa (NO testeado!!)
#if alexaSuport
#include <fauxmoESP.h>                                                                // https://bitbucket.org/xoseperez/fauxmoesp/get/master.zip
#endif
//************************************************************************************
// En la primera programación el archivo state.txt se debe crear con el contenido "off"
// también se debe de configurar los valores de la EEPRON desde la pagina WEB (modo AP)
// en el segunda programación después de configurar desde web,comentar la siguiente linea
//#define IniConfig
#warning ATENCION Primera programación descomentar #define IniConfig
//************************************************************************************

// Configuración acceso red WiFi
const char *ssid = "ssidxxxxxx";
const char* password = "xxxxxxxx";

//************************************************************************************
// En platformio.ini para usar OTA se debe indicar la misma ip "upload_port = 192.168.0.8x"
//************************************************************************************
#if NameLocation == 1
IPAddress staticIP(192, 168, 1, xx);                                                  // Configuración IP estática taller
String nameLocate = "taller";
uint8_t delayUp1 = 14;                                                                // Tiempo máximo de subida para la persiana 1
uint8_t delayDown1 = 14;                                                              // Tiempo máximo de bajada para la persiana 1
uint8_t delayUp2 = 0;                                                                 // Tiempo máximo de subida para la persiana 2
uint8_t delayDown2 = 0;                                                               // Tiempo máximo de bajada para la persiana 2
#elif NameLocation == 2
IPAddress staticIP(192, 168, 1, xx);                                                  // Configuración IP estática comedor
String nameLocate = "comedor";
uint8_t delayUp1 = 17;                                                                // Tiempo máximo de subida para la persiana 1
uint8_t delayDown1 = 17;                                                              // Tiempo máximo de bajada para la persiana 1
uint8_t delayUp2 = 0;                                                                 // Tiempo máximo de subida para la persiana 2
uint8_t delayDown2 = 0;                                                               // Tiempo máximo de bajada para la persiana 2
#elif NameLocation == 3
IPAddress staticIP(192, 168, 1, xx);                                                  // Configuración IP estática cocina (ESP07)
#warning ATENCION: MODULO ESP07 NO utiliza OTA
String nameLocate = "cocina";
uint8_t delayUp1 = 20;                                                                // Tiempo máximo de subida para la persiana 1
uint8_t delayDown1 = 19;                                                              // Tiempo máximo de bajada para la persiana 1
uint8_t delayUp2 = 0;                                                                 // Tiempo máximo de subida para la persiana 2
uint8_t delayDown2 = 0;                                                               // Tiempo máximo de bajada para la persiana 2
#elif NameLocation == 4
IPAddress staticIP(192, 168, 1, xx);                                                  // Configuración IP estática baño
String nameLocate = "aseo";
uint8_t delayUp1 = 13;                                                                // Tiempo máximo de subida para la persiana 1
uint8_t delayDown1 = 14;                                                              // Tiempo máximo de bajada para la persiana 1
uint8_t delayUp2 = 0;                                                                 // Tiempo máximo de subida para la persiana 2
uint8_t delayDown2 = 0;                                                               // Tiempo máximo de bajada para la persiana 2
#elif NameLocation == 5
IPAddress staticIP(192, 168, 1, xx);                                                  // Configuración IP estática cuartillo
String nameLocate = "cuartillo";
uint8_t delayUp1 = 16;                                                                // Tiempo máximo de subida para la persiana 1
uint8_t delayDown1 = 16;                                                              // Tiempo máximo de bajada para la persiana 1
uint8_t delayUp2 = 0;                                                                 // Tiempo máximo de subida para la persiana 2
uint8_t delayDown2 = 0;                                                               // Tiempo máximo de bajada para la persiana 2
#elif NameLocation == 6
IPAddress staticIP(192, 168, 1, xx);                                                  // Configuración IP estática habitación
#warning ATENCION: MODULO ESP07 NO utiliza OTA
String nameLocate = "habitacion";
uint8_t delayUp1 = 17;                                                                // Tiempo máximo de subida para la persiana 1
uint8_t delayDown1 = 17;                                                              // Tiempo máximo de bajada para la persiana 1
uint8_t delayUp2 = 17;                                                                // Tiempo máximo de subida para la persiana 2
uint8_t delayDown2 = 17;                                                              // Tiempo máximo de bajada para la persiana 2
#elif NameLocation == 7
IPAddress staticIP(192, 168, 1, xx);                                                  // Configuración IP estática pasillo
String nameLocate = "pasillo";
uint8_t delayUp1 = 15;                                                                // Tiempo máximo de subida para la persiana 1
uint8_t delayDown1 = 15;                                                              // Tiempo máximo de bajada para la persiana 1
uint8_t delayUp2 = 15;                                                                // Tiempo máximo de subida para la persiana 2
uint8_t delayDown2 = 15;                                                              // Tiempo máximo de bajada para la persiana 2
#else
IPAddress staticIP(192, 168, 1, xx);                                                  // Configuración IP estática desconocido
String nameLocate = "Desconocido";
uint8_t delayUp1 = 3;                                                                 // Tiempo máximo de subida para la persiana 1
uint8_t delayDown1 = 3;                                                               // Tiempo máximo de bajada para la persiana 1
uint8_t delayUp2 = 0;                                                                 // Tiempo máximo de subida para la persiana 2
uint8_t delayDown2 = 0;                                                               // Tiempo máximo de bajada para la persiana 2
#warning ATENCION No tenemos una localización valida
#endif

IPAddress gateway(192, 168, 0, xx);
IPAddress dnServer(192, 168, 0, xx);
IPAddress subnet(255, 255, 255, 0);

// MQTT
const char* MQTT_SERVER = "192.168.0.xxx";                                            // Dirección IP del servidor MQTT
const uint16_t MQTT_PORT = 1883;                                                      // MQTT puerto broker
const char* userMQTT = "xxxxxxxx";
const char* passMQTT = "xxxxxxxx";

String clientId = "_blind";
bool MQTT_RETAIN = false;                                                             // MQTT bandera retain (retención)
uint8_t MQTT_QOS = 0;                                                                 // MQTT QoS mensajes
const char* willTopic = 0;                                                            // MQTT willTopic
const char* willMessage = 0;                                                          // MQTT willMessage

// Temporizador
unsigned long ultimaConsulta = 0;
unsigned long tiempoConsulta = 270000;                                                // 4.5 minutos (<MQTT_KEEPALIVE)
unsigned long periodRun1;                                                             // Periodo de marcha persiana 1
unsigned long periodRun2;                                                             // Periodo de marcha persiana 2
unsigned long fcRun1;                                                                 // Periodo activo final de carrera para persiana 1
unsigned long fcRun2;                                                                 // Periodo activo final de carrera para persiana 2

unsigned long periodFcI1;                                                             // Periodo conmutación on-off final carrera por intensidad persiana 1
unsigned long periodFcI2;                                                             // Periodo conmutación on-off final carrera por intensidad persiana 2
uint16_t OnOffI_FC = 250;                                                             // Periodo de paso a off del final carrera por intensidad

uint8_t ReleFalg = 0b00000000;                                                        // Banderas estado relés
uint16_t webFalg = 0b0000000000100000;                                                // Banderas de opciones pagina web
uint8_t updateWeb = 0;                                                                // Indicación actualizar opciones web
volatile uint8_t statusFC = 0;                                                        // Banderas indicación estado de los finales de carrera
uint8_t runBlind = 0;                                                                 // Banderas estado marcha persianas
uint8_t position1 = 50;                                                               // Posición actual persiana 1, Abierta =100%, Cerrada =0% 
uint8_t position2 = 50;                                                               // Posición actual persiana 2, Abierta =100%, Cerrada =0% 
uint8_t setPosition1 = position1;
uint8_t setPosition2 = position2;
uint8_t autoDetec = 0;                                                                // Auto detección posición persianas al iniciar
uint8_t error = 0;                                                                    // Indicación del tipo de error
volatile bool iniRunFc1 = false;                                                      // inicializado F.C persiana 1, 0 -> no , 1 -> inicializado
volatile bool iniRunFc2 = false;                                                      // inicializado F.C persiana 2, 0 -> no , 1 -> inicializado
bool activeBlind1 = true;                                                             // persiana 1 activa (editable desde web en modo AP)
bool activeBlind2 = false;                                                            // persiana 2 NO activa (editable desde web en modo AP)
volatile bool StopIsr1 = false;                                                       // bandera ISR timer 1
volatile bool StopIsr2 = false;                                                       // bandera ISR timer 2
bool CommandSTOP1 = false;                                                            // STOP persiana 1 desde MQTT
bool CommandSTOP2 = false;                                                            // STOP persiana 2 desde MQTT

bool Fs = false;
bool modeHandbook = false;                                                            // mode->false modo automático habilitado

uint8_t modeLimit = 2;                                                                // F.C 0-> NO usado, 1-> físicos, 2-> corriente
bool okMqtt = false;
uint8_t locate = 0;

const char* sofVersion = "0.1.9";
const String Compiler = String(__DATE__);                                             // Obtenemos la fecha de la compilación

// Temperatura DHT11
#if DHTsensor
#include <SimpleDHT.h>                                                                // https://github.com/winlinvip/SimpleDHT (NO ISR)
bool OkDht = false;
uint8_t tempInt = 0;
uint8_t humedadInt = 0;
#endif

#include "httpap.h"
#include "ESP8266_OTA.h"

#if DebugSerial
#include "uptime_formatter.h"                                                         // https://github.com/YiannisBourkelis/Uptime-Library
#endif

// Alexa
bool alexaRun = 0;                                                                    // Soporte para alexa NO habilitado (defecto)
#if alexaSuport
#warning ATENCION Suporte Alexa !!No TESTEADO-TESTED!!
uint8_t alexaFalg = 0b00000000;
String nameDeviceAlexaUp1;
String nameDeviceAlexadown1;
String nameDeviceAlexaStop1;
String nameDeviceAlexaUp2;
String nameDeviceAlexadown2;
String nameDeviceAlexaStop2;
fauxmoESP alexa;
#endif

void callback(char* topic, byte* payload, unsigned int length);
// Instancia a objetos
WiFiClient espClient;
PubSubClient client(MQTT_SERVER, MQTT_PORT, callback, espClient);

#if DHTsensor
SimpleDHT11 dht11 (DHTPin);
#endif
Ticker tickerMs1;
Ticker tickerMs2;

#if mDNS
MDNSResponder mdns;
#endif

ADC_MODE(ADC_VCC);

//////////////////////////////////////////////////////////////////////////////////////
// Paramos motor persiana 1
//////////////////////////////////////////////////////////////////////////////////////
ICACHE_RAM_ATTR void toggleseconds1() {
  if (APmode) {
    if (digitalRead(Up1Pin)) {
      bitSet (statusFC, 2);
      bitClear (statusFC, 1);
    }
    else {
      bitSet (statusFC, 1);
      bitClear (statusFC, 2);
    }
  }
  digitalWrite(Up1Pin, LOW);
  digitalWrite(Down1Pin, LOW);
  periodFcI1 = millis() + OnOffI_FC;
  bitClear(runBlind, 0);
  bitClear(runBlind, 1);
  if (iniRunFc1 == true) StopIsr1 = true;                                             // Si tenemos inicializado los F.C indicamos STOP por ISR
  iniRunFc1 = true;
  tickerMs1.detach();
}
//////////////////////////////////////////////////////////////////////////////////////
// Paramos motor persiana 2
//////////////////////////////////////////////////////////////////////////////////////
ICACHE_RAM_ATTR void toggleseconds2() {
  if (APmode) {
    if (digitalRead(Up2Pin)) {
      bitSet (statusFC, 4);
      bitClear (statusFC, 3);
    }
    else {
      bitSet (statusFC, 3);
      bitClear (statusFC, 4);
    }
  }
  digitalWrite(Up2Pin, LOW);
  digitalWrite(Down2Pin, LOW);
  periodFcI2 = millis() + OnOffI_FC;
  bitClear(runBlind, 2);
  bitClear(runBlind, 3);
  if (iniRunFc2 == true)  StopIsr2 = true;
  iniRunFc2 = true;
  tickerMs2.detach();
}

//////////////////////////////////////////////////////////////////////////////////////
// Abre el archivo para leer y obtiene el valor
//////////////////////////////////////////////////////////////////////////////////////
String readFile(String path) {
  File rFile = LittleFS.open(path, "r");
  String content = "off";
  if (rFile) {
    content = rFile.readStringUntil('\r');
#if DebugSerial
    debugPrintln(String(F("LittleFS: leído valor: ")) + content);
#endif
  }
  else {
    error == 0 ? error = 1: error = 100;                                              // Indicamos error en lectura FS
#if DebugSerial
    debugPrintln(String(F("LittleFS: Error lectura")));
#endif
  }
  rFile.close();
  return content;
}

//////////////////////////////////////////////////////////////////////////////////////
// Abre el archivo para escribir ("w" write) y sobre escribe el contenido
//////////////////////////////////////////////////////////////////////////////////////
void writeFile(String state, String path) {
  String stateRead = readFile("/state.txt");
#ifndef IniConfig
  if (stateRead == state) return;    
#endif          
  File wFile = LittleFS.open(path, "w+");
  if (wFile) {
    wFile.println(state);
#if DebugSerial
    debugPrintln(String(F("LittleFS: escrito estado: ")) + state);
#endif
  }
  else {
    error == 0 ? error = 2: error = 100;                                              // Indicamos error en escritura FS
#if DebugSerial
    debugPrintln(String(F("LittleFS: Error escritura")));
#endif
  }
  wFile.close();
}

//////////////////////////////////////////////////////////////////////////////////////
// Configuración
//////////////////////////////////////////////////////////////////////////////////////
void setup() {
  // configuramos pines
  pinMode(Up1Pin, OUTPUT);
  digitalWrite(Up1Pin, LOW);                                                          // Relé subir persiana 1 OFF
  pinMode(Down1Pin, OUTPUT);
  digitalWrite(Down1Pin, LOW);                                                        // Relé bajar persiana 1 OFF
  pinMode(Up2Pin, OUTPUT);
  digitalWrite(Up2Pin, LOW);                                                          // Relé subir persiana 2 OFF
  pinMode(Down2Pin, OUTPUT);
  digitalWrite(Down2Pin, LOW);                                                        // Relé bajar persiana 2 OFF
  pinMode(FC_1, INPUT);
  pinMode(FC_2, INPUT);

  Fs = LittleFS.begin();

#if DebugSerial
  Serial.begin(115200);                                                               // Serial
  delay(250);
  debugPrintln(String(F("\n\n<-------\n\n")));
  debugPrintln(String(F("SYSTEM: Reinicio por: ")) + ESP.getResetInfo());
  debugPrintln(String(F("SYSTEM: ID ESP: ")) + String(ESP.getChipId()));
  debugPrintln(String(F("SYSTEM: CPU frecuencia: ")) + String(ESP.getCpuFreqMHz()) + "MHz");
  debugPrintln(String(F("SYSTEM: Versión Core: ")) + String(ESP.getCoreVersion()));
  debugPrintln(String(F("SYSTEM: Versión SDK: ")) + String(ESP.getSdkVersion()));
  debugPrintln(String(F("SYSTEM: Versión: ")) + String (sofVersion));
  debugPrintln(String(F("SYSTEM: Compilado: ")) + Compiler);
  if (Fs) {
    debugPrintln(String(F("LittleFS: Sistema de archivos abierto")));
  }
  else {
    debugPrintln(String(F("LittleFS: Error al abrir el sistema de archivos")));
  }
  uint32_t realSize = ESP.getFlashChipRealSize();
  uint32_t ideSize = ESP.getFlashChipSize();
  FlashMode_t ideMode = ESP.getFlashChipMode();
  debugPrintln(String(F("Flash id: ")) + ESP.getFlashChipId());
  debugPrintln(String(F("Flash capacidad real: ")) + String(realSize));
  debugPrintln(String(F("Flash capacidad: ")) + String (ideSize) + " Bytes");
  debugPrintln(String(F("Flash velocidad: ")) + String (ESP.getFlashChipSpeed()) + "Hz");
  debugPrintln(String(F("Flash modo:  ")) + String (ideMode));
  if (ideSize != realSize) {
    debugPrintln(String(F("Configuración incorrecta de la Flash del Chip!")));
  }
  else {
    debugPrintln(String(F("Configuración correcta de la Flash del Chip.")));
  }
#endif

  EEPROM.begin(100);
  if (EEPROM.read(0) == 128 && ForceEEPROM == 0) {                                    // Tenemos una configuración en la EEPROM?
    delayUp1 = EEPROM.read(1);
    delayDown1 = EEPROM.read(2);
    delayUp2 = EEPROM.read(3);
    delayDown2 = EEPROM.read(4);
    activeBlind1 = EEPROM.read(5);
    activeBlind2 = EEPROM.read(6);
    modeLimit = EEPROM.read(7);                                                       // F.C 0-> NO usado, 1-> físicos, 2-> corriente
    autoDetec = EEPROM.read(8);
    alexaRun = bool(EEPROM.read(9));
    nameLocate = rxEeprom(10);
#if DebugSerial
    debugPrintln(String(F("CONFIG: Leemos la configuración de la EEPROM")));
#endif
  }
  else
  {
    EEPROM.write(0, 128);
    EEPROM.write(1, delayUp1);
    EEPROM.write(2, delayDown1);
    EEPROM.write(3, delayUp2);
    EEPROM.write(4, delayDown2);
    EEPROM.write(5, activeBlind1);
    EEPROM.write(6, activeBlind2);
    EEPROM.write(7, modeLimit);
    EEPROM.write(8, autoDetec);
    EEPROM.write(9, alexaRun);
    txEeprom(10, nameLocate);                                                         // nameLocate + EEPROM.commit()
    EEPROM.end();
#if DebugSerial
    debugPrintln(String(F("CONFIG: Programada configuración defecto en la EEPROM")));
#endif
  }
  
  if (Fs) {
  #ifdef IniConfig
    writeFile("off", "/state.txt");
    if (activeBlind1) {
      String position = String  (position1);
      writeFile(position, "/position1.txt");
    }
    if (activeBlind2) {
      String position = String  (position2);
      writeFile(position, "/position2.txt");
    }
#endif
    String stateMode = readFile("/state.txt");
    stateMode == "on" ? modeHandbook = true : modeHandbook = false;
    if (activeBlind1) {                                                               // Si anteriormente se realizó un reset por software
      stateMode = readFile("/position1.txt");
      uint8_t p1Int = stateMode.toInt();
      if (p1Int > 100) {                                                              // reset por software?
        position1 = p1Int - 101;
      }
    }
    if (activeBlind2) {
      stateMode = readFile("/position2.txt");
      uint8_t p2Int = stateMode.toInt();
      if (p2Int > 100) {
        position2 = p2Int - 101;
      }
    }
  }

  client.setBufferSize(1024);                                                         // Tamaño máximo de paquete MQTT
  client.setKeepAlive(300);                                                           // 300 segundos = 5 minutos

  if (modeLimit == 2) {
    uint8_t x;
    statusFC = 0;
    FC_2 = FC_1;                                                                      // Limites por consumo solo se usa FC_1
    if (activeBlind1 && autoDetec) {
      x = searchLimits(0);                                                            // Intentamos conocer los limites de la persiana 1
      iniRunFc1 = true;                                                               // Indicamos que tenemos los F.C inicializado
      if (x == 1) {                                                                   // Cerrada ?
        bitSet(statusFC, 1);
        setPosition1 = 0;
        position1 = 0;                                                                // Cerrada al 0%
      }
      else if (x == 2) {                                                              // Abierta?
        bitSet(statusFC, 2);
        setPosition1 = 100;
        position1 = 100;                                                              // Abierta al 100%
      }
    }
    if (activeBlind2 && autoDetec) {
      x = searchLimits(1);                                                            // Intentamos conocer los limites de la persiana 2
      iniRunFc2 = true;															                                  // Indicamos que tenemos los F.C inicializado
      if (x == 3) {                                                                   // Cerrada
        bitSet(statusFC, 3);
        setPosition2 = 0;
        position2 = 0;
      }
      else if (x == 4) {                                                              // Abierta?
        bitSet(statusFC, 4);
        setPosition2 = 100;
        position2 = 100;
      }
    }
  }

#if DebugSerial
  debugPrintln(String(F("CONFIG: ID: ")) + clientId);
  if (activeBlind1) {
    debugPrintln(String(F("CONFIG: Persiana 1 habilitada")));
    debugPrintln(String(F("CONFIG: Tiempo subida persiana 1: ")) + String(delayUp1));
    debugPrintln(String(F("CONFIG: Tiempo bajada persiana 1: ")) + String(delayDown1));
  }
  if (activeBlind2) {
    debugPrintln(String(F("CONFIG: Persiana 2 habilitada")));
    debugPrintln(String(F("CONFIG: Tiempo subida persiana 2: ")) + String(delayUp2));
    debugPrintln(String(F("CONFIG: Tiempo bajada persiana 2: ")) + String(delayDown2));
  }
  if (modeLimit == 1) {
    debugPrintln(String(F("CONFIG: Finales de carrera físicos")));
  }
  else if (modeLimit == 2) {
    debugPrintln(String(F("CONFIG: Limites por consumo motor")));
  }
  else if (modeLimit == 0) {
    debugPrintln(String(F("CONFIG: Limites NO usados")));
  }
  if (autoDetec) {
    debugPrintln(String(F("CONFIG: Auto-detección posición persianas al iniciar")));
  }
  else {
    debugPrintln(String(F("CONFIG: NO Auto-detección posición persianas al iniciar")));
  }
  if (statusFC == 0) debugPrintln(F("CONFIG: Finales carrera persiana 1 posición desconocida"));
  else if (bitRead (statusFC, 2) == 1) debugPrintln(F("CONFIG: Detectado final carrera persiana 1 arriba"));
  else if (bitRead (statusFC, 1) == 1) debugPrintln(F("CONFIG: Detectado final carrera persiana 1 abajo"));
  if (activeBlind2) {
    if (statusFC == 0) debugPrintln(F("CONFIG: Finales carrera persiana 2 posición desconocida"));
    else if (bitRead (statusFC, 4) == 1) debugPrintln(F("CONFIG: Detectado final carrera persiana 2 arriba"));
    else if (bitRead (statusFC, 3) == 1) debugPrintln(F("CONFIG: Detectado final carrera persiana 2 abajo"));
  }
 
  if (alexaRun) {
    debugPrintln(String(F("CONFIG: Soporte para Alexa habilitado")));
  }
  else {
    debugPrintln(String(F("CONFIG: SIN soporte para Alexa")));
  }

  if (modeHandbook) {
    debugPrintln(String(F("CONFIG: Control NO automático")));
  }
  else {
    debugPrintln(String(F("CONFIG: Control automático")));
  }
#endif
#if DHTsensor
  OkDht = ReadDHT();
#endif

#ifdef IniConfig
  wifiAP();
#else      
  // Creamos el nombre del cliente basado en la dirección MAC y los últimos 3 bytes                                                    
  clientId = nameLocate + clientId;
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientId +=  "-" + String(mac[3], 16) + String(mac[4], 16) + String(mac[5], 16);
  clientId.toUpperCase();                                                             // Pasamos a minúsculas
  WiFi.disconnect();                                                                  // Después de reiniciar aún se podría conserva la conexión antigua
  WiFi.persistent (false);                                                            // No se guardar SSID y contraseña (NO memoria flash)
  WiFi.setAutoReconnect(true);                                                        // Reconectar si se pierde la conexión
  wifi_station_set_hostname((char*) clientId.c_str());
  reConnect();                                                                        // Intentamos conectarnos a la red WIFI y luego conectarse al servidor MQTT
#if mDNS
  if(!mdns.begin((char*) clientId.c_str())){
#if DebugSerial
    debugPrintln(F("[WIFI] ERROR al inicializar mDNS!!")); 
  }
  else {
    debugPrintln(String(F("[WIFI]: HostName: ")) + WiFi.hostname().c_str());
#endif
  }
#endif
#endif

  if (!InitOTA(clientId)) {
    error == 0 ? error = 3: error = 100;                                              // Indicamos error OTA
#if DebugSerial
    debugPrintln(F("[OTA] ERROR!!"));   
  }
  else {
    debugPrintln(F("[OTA] Inicializado"));
#endif
  }

#if alexaSetup
  if (alexaRun) {
    // Por defecto, fauxmoESP crea su propio servidor web en el puerto definido
    // El puerto TCP debe ser 80 para dispositivos gen3 (tercera generación), el valor predeterminado es 1901
    alexa.createServer(true);                                                         // valor por defecto
    alexa.setPort(80);                                                                // Para dispositivos gen3
    alexa.enable(alexaRun);                                                           // Habilitamos después de estar conectados

    String Blind1 = "";
    String Blind2 = "";
    if (activeBlind1 + activeBlind2 == 2) {
      Blind1 = "Derecha ";
      Blind2 = "Izquierda ";
    }

    nameDeviceAlexaUp1 = "subir persiana " + Blind1 + nameLocate;
    static uint8_t leName = nameDeviceAlexaUp1.length() + 1;
    char setUp1[leName];
    char setdown1[leName];
    char setStop1[leName];

    char setUp2[leName];
    char setdown2[leName];
    char setStop2[leName];
    if (activeBlind1) {
      nameDeviceAlexaUp1.toCharArray(setUp1, leName);
      alexa.addDevice(setUp1);                                                        // Sube persiana 1
#if DebugSerial
      debugPrintln(String(F("CONFIG: Orden subir Alexa: ")) + nameDeviceAlexaUp1);
#endif
      nameDeviceAlexadown1 = "bajar persiana " + Blind1 + nameLocate;
      leName = nameDeviceAlexadown1.length() + 1;
      nameDeviceAlexadown1.toCharArray(setdown1, leName);
      alexa.addDevice(setdown1);                                                      // Baja persiana 1
#if DebugSerial
      debugPrintln(String(F("CONFIG: Orden bajar Alexa: ")) + nameDeviceAlexadown1);
#endif
      nameDeviceAlexaStop1 = "parar persiana " + Blind1 + nameLocate;
      leName = nameDeviceAlexaStop1.length() + 1;
      nameDeviceAlexaStop1.toCharArray(setStop1, leName);
      alexa.addDevice(setStop1);                                                      // Para persiana 1
#if DebugSerial
      debugPrintln(String(F("CONFIG: Orden parar Alexa: ")) + nameDeviceAlexaStop1);
#endif
    }
    if (activeBlind2) {
      nameDeviceAlexaUp2 = "subir persiana " + Blind2 + nameLocate;
      nameDeviceAlexaUp2.toCharArray(setUp2, leName);
      alexa.addDevice(setUp2);                                                        // "subir"
#if DebugSerial
      debugPrintln(String(F("CONFIG: Orden subir Alexa: ")) + nameDeviceAlexaUp2);
#endif
      nameDeviceAlexadown2 = "bajar Persiana " + Blind2 + nameLocate;
      leName = nameDeviceAlexadown2.length() + 1;
      nameDeviceAlexadown2.toCharArray(setdown2, leName);
      alexa.addDevice(setdown2);                                                      // "bajar"
#if DebugSerial
      debugPrintln(String(F("CONFIG: Orden bajar Alexa: ")) + nameDeviceAlexadown2);
#endif
      nameDeviceAlexaStop2 = "parar Persiana " + Blind2 + nameLocate;
      leName = nameDeviceAlexaStop2.length() + 1;
      nameDeviceAlexaStop2.toCharArray(setStop2, leName);
      alexa.addDevice(setStop2);                                                      // "parar"
#if DebugSerial
      debugPrintln(String(F("CONFIG: Orden parar Alexa: ")) + nameDeviceAlexaStop2);
#endif
    }
    // Callback de llamada cuando se recibe un comando de Alexa.
    alexa.onSetState([](unsigned char device_id, const char* device_name, bool state, unsigned char value) {
      if (strcmp(device_name, (char*) nameDeviceAlexaUp1.c_str()) == 0) {
        bitWrite(alexaFalg, 0, state);                                                // Bandera subir persiana 1
        bitWrite(alexaFalg, 7, 1);                                                    // Indicamos cambio desde Alexa
      }
      else if ((strcmp(device_name, (char*) nameDeviceAlexadown1.c_str()) == 0) ) {
        bitWrite(alexaFalg, 1, state);                                                // Bandera bajar persiana 1
        bitWrite(alexaFalg, 7, 1);
      }
      else if ((strcmp(device_name, (char*) nameDeviceAlexaStop1.c_str()) == 0) ) {
        bitWrite(alexaFalg, 2, state);                                                // Bandera parar persiana 1
        bitWrite(alexaFalg, 7, 1);
      }
      else if (strcmp(device_name, (char*) nameDeviceAlexaUp2.c_str()) == 0) {
        bitWrite(alexaFalg, 3, state);                                                // Bandera subir persiana 2
        bitWrite(alexaFalg, 7, 1);
      }
      else if ((strcmp(device_name, (char*) nameDeviceAlexadown2.c_str()) == 0) ) {
        bitWrite(alexaFalg, 4, state);                                                // Bandera bajar persiana 2
        bitWrite(alexaFalg, 7, 1);
      }
      else if ((strcmp(device_name, (char*) nameDeviceAlexaStop2.c_str()) == 0) ) {
        bitWrite(alexaFalg, 5, state);                                                // Bandera parar persiana 2
        bitWrite(alexaFalg, 7, 1);
      }
    });
    if (activeBlind1) {
      alexa.setState((char*) nameDeviceAlexaUp1.c_str(), 0, 0);                       // Indicamos a alexa el estado inicial
      alexa.setState((char*) nameDeviceAlexadown1.c_str(), 0, 0);                     // Indicamos a alexa el estado inicial
    }
    if (activeBlind2) {
      alexa.setState((char*) nameDeviceAlexaUp2.c_str(), 0, 0);                       // Indicamos a alexa el estado inicial
      alexa.setState((char*) nameDeviceAlexadown2.c_str(), 0, 0);                     // Indicamos a alexa el estado inicial
    }
  }
#endif
#if DebugSerial
  debugPrintln(String(F("Inicializado...\n\n")));
#endif
}

uint8_t webReturn;
uint8_t cicleIR = 0;
uint16_t cicleConnec = 0, cicleDisConnec = 0;
unsigned long previousMillis = 0, delayAP = 60000;
//************************************************************************************
// Principal
//************************************************************************************
void loop() {
  if (APmode) {                                                                       // SIN conexión WIFI, modo Punto de Acceso?
    webReturn = connectAP();                                                          // SI, RUN Punto de Acceso + web
    if (updateWeb) webUpdate();                                                       // Datos a actualizar desde web en modo AP
    delay(1);
    if (++cicleDisConnec == delayAP) {
      if (delayAP > 60000) espReset();                                                // Modo AP forzado por comando "apmodeCmd"
      cicleDisConnec = 0;
      if (listNetworks(ssid))
        reConnect();                                                                  // Cada minuto en modo AP se re-intenta conectar
    }
#ifdef IniConfig
    cicleDisConnec = 0;
#endif
  }
  else {
#if alexaSuport
    if (alexaRun) alexa.handle();
#endif
    if (!client.connected() || WiFi.status() != 3) {                                  // Reconectar si se perdió la conexión wifi o mqtt
      reConnect();                                                                    // Re-intenta conectar
    }
    if ((millis() - ultimaConsulta) > tiempoConsulta) {                               // Gestión temporizador periódico 4.5 minutos
      if (++cicleConnec == (39 + NameLocation)) {                                     // 4.5 * 40 = 180 minutos
        cicleConnec = 0;
        locate = 0;                                                                   // Reenviamos Auto descubrimiento Home Assistant
      }
#if DHTsensor
      OkDht = ReadDHT();                                                              // Leemos sensor DHT11
#endif
      publicMqtt();                                                                   // Publicación MQTT periódica
      ultimaConsulta = millis();
    }
    client.loop();                                                                    // Mantenemos activa la conexión MQTT
    ArduinoOTA.handle();
  }

#if DebugSerial
  if (runBlind == 0) {                                                                // Persianas en reposo?
    if (millis() % 5000 == 0) {                                                       // Info...
    debugPrintln(String(F("En marcha: ")) + uptime_formatter::getUptime());
    debugPrintln(String(F("Persiana: ")) + String(nameLocate)); // Nombre
    debugPrintln(String(F("ID: ")) + clientId);                 // ID
    if (iniRunFc1 == false)
      debugPrintln(F("Posición persiana 1 desconocida"));
    else
      debugPrintln(String(F("Posición persiana 1: ")) + String(position1));
    if (iniRunFc2 == false && activeBlind2)
      debugPrintln(F("Posición persiana 2 desconocida"));
    else
      debugPrintln(String(F("Posición persiana 2: ")) + String(position2));
    if (WiFi.status() == WL_CONNECTED)
    {                                                          // Conectado a WIFI?
      debugPrintln(String(F("Conectado a: ")) + String(ssid)); // SSID
      debugPrintln(String(F("WIFI: RSSI: ")) + WiFi.RSSI());   // Nivel señal wifi
      if (client.connected())
      { // Conectado a MQTT?
        debugPrintln(F("Conectado a MQTT"));
      }
      else
      {
        debugPrintln(F("NO Conectado a MQTT"));
      }
      }
      else {
        debugPrintln(F("!!DESCONECTADO!!"));
      } 
    }
  }
#endif

  if (bitRead (ReleFalg, 7)) RelayStatus();                                           // Comprobamos si tenemos activo algún relé
  if (modeLimit) LimitsActive (LimitStatus());                                        // Comprobamos si usamos finales de carrera

#if alexaSuport
  if (bitRead (alexaFalg, 7)) RelayStatus();                                          // Gestión actualización cambio estado relés por Alexa
#endif  
}

//************************************************************************************
// Gestionamos ordenes y configuración enviados desde la pagina web en modo punto de acceso
//************************************************************************************
void webUpdate() {
  uint8_t x;
  bool eppronWire = 0;
  cicleConnec = 0;
  if (updateWeb == 2) {                                                               // Configuración
    EEPROM.begin(100);
#if DebugSerial
    debugPrintln(F("Configuración WEB"));
#endif
    updateWeb = 0;
    if (bitRead(webFalg, 5)) {                                                        // Persiana 1 activa?
      activeBlind1 = 1;
      x = EEPROM.read(5);
      if (x != activeBlind1) {
        EEPROM.write (5, activeBlind1);
        eppronWire = 1;
      }
#if DebugSerial
      debugPrintln(F("Control persiana 1 habilitado"));
#endif
    }
    else {                                                                            // Persiana 1 NO activa
      activeBlind1 = 0;
      x = EEPROM.read(5);
      if (x != activeBlind1) {                                                        // SOLO escribimos si el valor de la EEPROM es distinto
        EEPROM.write (5, activeBlind1);
        eppronWire = 1;
      }
#if DebugSerial
      debugPrintln(F("Control persiana 1 deshabilitado"));
#endif
    }
    if (bitRead(webFalg, 6)) {                                                        // Persiana 2 activa?
      activeBlind2 = 1;
      x = EEPROM.read(6);
      if (x != activeBlind2) {                                                        // SOLO escribimos si el valor de la EEPROM es distinto
        EEPROM.write (6, activeBlind2);
        eppronWire = 1;
      }
#if DebugSerial
      debugPrintln(F("Control persiana 2 habilitado"));
#endif
    }
    else {
      activeBlind2 = 0;                                                               // Persiana 2 NO activa
      x = EEPROM.read(6);
      if (x != activeBlind2) {                                                        // SOLO escribimos si el valor de la EEPROM es distinto
        EEPROM.write (6, activeBlind2);
        eppronWire = 1;
      }
#if DebugSerial
      debugPrintln(F("Control persiana 2 deshabilitado"));
#endif
    }
    if (bitRead(webFalg, 7)) {                                                        // Tipo de F.C ->por consumo
      modeLimit = 2;
      x = EEPROM.read(7);
      if (x != modeLimit) {
        EEPROM.write (7, modeLimit);
        eppronWire = 1;
      }
#if DebugSerial
      debugPrintln(F("Finales de carrera por control consumo motor"));
#endif
    }
    if (bitRead(webFalg, 8)) {                                                        // Tipo de F.C -> físicos
      modeLimit = 1;
      x = EEPROM.read(7);
      if (x != modeLimit) {
        EEPROM.write (7, modeLimit);
        eppronWire = 1;
      }
#if DebugSerial
      debugPrintln(F("Finales de carrera físicos"));
#endif
    }
    if (bitRead(webFalg, 9)) {                                                        // Tipo de F.C ->NO usados
      modeLimit = 0;
      bitClear(webFalg, 9);
      x = EEPROM.read(7);
      if (x != modeLimit) {
        EEPROM.write (7, modeLimit);
        eppronWire = 1;
      }
#if DebugSerial
      debugPrintln(F("NO usamos finales carrera"));
#endif
    }
    if (autoDetec != bitRead(webFalg, 10)) {                                          // Auto detención posición persiana al inicio?
      autoDetec = bitRead(webFalg, 10);
      bitClear(webFalg, 10);
      x = EEPROM.read(8);
      if (x != autoDetec) {
        EEPROM.write (8, autoDetec);
        eppronWire = 1;
      }
#if DebugSerial
      debugPrintln(String(F("Auto-detección posición persianas ")) + autoDetec ? "habilitado" : "deshabilitado");
#endif
    }

    if (alexaRun != bitRead(webFalg, 11)) {                                           // Config. alexa?
      alexaRun = bitRead(webFalg, 11);
      x = EEPROM.read(9);
      if (x != alexaRun) {
        EEPROM.write (9, alexaRun);
        eppronWire = 1;
      }
#if DebugSerial
      debugPrintln(String(F("Elexa ")) + alexaRun ? "habilitado" : "deshabilitado");
#endif
    }

    if (bitRead(webFalg, 12)) {                                                       // Config. nombre?
      bitClear(webFalg, 12);
      txEeprom(10, nameLocate);
      eppronWire = 1;
#if DebugSerial
      debugPrintln(String(F("Cambio del nombre a: ")) + nameLocate);
#endif
    }
    x = EEPROM.read(1);
    if (x != delayUp1) {                                                              // Cambio en valor del tiempo de subida persiana 1?
      EEPROM.write (1, delayUp1);
      eppronWire = 1;
    }
    x = EEPROM.read(2);
    if (x != delayDown1) {                                                            // Cambio en valor del tiempo de bajada persiana 1?
      EEPROM.write (2, delayDown1);
      eppronWire = 1;
    }
    x = EEPROM.read(3);
    if (x != delayUp2) {                                                              // Cambio en valor del tiempo de subida persiana 2?
      EEPROM.write (3, delayUp2);
      eppronWire = 1;
    }
    x = EEPROM.read(4);
    if (x != delayDown2 ) {                                                           // Cambio en valor del tiempo de bajada persiana 2?
      EEPROM.write (4, delayDown2);
      eppronWire = 1;
    }
    if (eppronWire) {
      EEPROM.write (0, 128);
      EEPROM.commit();
      EEPROM.end();

#if DebugSerial
      debugPrintln(String(F("Tiempo subida persiana 1: ")) + String (delayUp1));
      debugPrintln(String(F("Tiempo bajada persiana 1: ")) + String (delayDown1));
      debugPrintln(String(F("Tiempo subida persiana 2: ")) + String (delayUp2));
      debugPrintln(String(F("Tiempo bajada persiana 2: ")) + String (delayDown2));
      debugPrintln(F("Nueva configuración escrita en EEPROM"));
#endif
    }
  }
  // Control -------------------------------------------------------------------------
  if (updateWeb == 1) {
#if DebugSerial
    debugPrintln(F("Control WEB persiana/s"));
#endif
    updateWeb = 0;
    if (bitRead(webFalg, 0)) {                                                        // Orden subir persiana 1?
      setPosition1 = 100;
      RelayOnOff (1, 0);
      bitClear(webFalg, 0);
#if DebugSerial
      debugPrintln(F("Subiendo persiana 1"));
#endif
    }
    else if (bitRead(webFalg, 1)) {                                                   // Orden bajar persiana 1?
      setPosition1 = 0;
      RelayOnOff(2, 0);
      bitClear(webFalg, 1);
#if DebugSerial
      debugPrintln(F("Bajando persiana 1"));
#endif
    }
    if (bitRead(webFalg, 2)) {                                                        // Orden subir persiana 2?
      setPosition2 = 100;
      RelayOnOff(3, 0);
      bitClear(webFalg, 2);
#if DebugSerial
      debugPrintln(F("Subiendo persiana 2"));
#endif
    }
    else if (bitRead(webFalg, 3)) {                                                   // Orden bajar persiana 2?
      setPosition2 = 0;
      RelayOnOff(4, 0);
      bitClear(webFalg, 3);
#if DebugSerial
      debugPrintln(F("Bajando persiana 2"));
#endif
    }
    if (bitRead(webFalg, 4)) {                                                        // Orden detectar posición?
      bitClear(webFalg, 4);
#if DebugSerial
      debugPrintln(F("Intentado detectar posición persianas"));
#endif
      if (activeBlind1) {                                                             // Intentamos conocer los limites de la persiana 1
        uint8_t x = searchLimits(0);
        if (x == 0) {
          statusFC = 0;
        }
        else {
          if (x == 1) {
            bitSet(statusFC, 1);
            setPosition1 = 100;
            position1 = 100;
#if DebugSerial
            debugPrintln(F("Persiana 1 subida"));
#endif
          }
          if (x == 2) {
            bitSet(statusFC, 2);
            setPosition1 = 0;
            position1 = 0;
#if DebugSerial
            debugPrintln(F("Persiana 1 bajada"));
#endif
          }
        }
      }
      if (activeBlind2) {
        uint8_t x = searchLimits(1);                                                  // Intentamos conocer los limites de la persiana 2
        if (x == 3) {
          bitSet(statusFC, 3);
          setPosition2 = 100;
          position2 = 100;
#if DebugSerial
          debugPrintln(F("Persiana 2 subida"));
#endif
        }
        if (x == 4) {
          bitSet(statusFC, 4);
          setPosition2 = 0;
          position2 = 0;
#if DebugSerial
          debugPrintln(F("Persiana 2 bajada"));
#endif
        }
      }
#if DebugSerial
      if (statusFC == 0) {
        debugPrintln(F("NO detectada posición persiana"));
      }
#endif
    }
  }
}
//////////////////////////////////////////////////////////////////////////////////////
// Reseteamos ESP
//////////////////////////////////////////////////////////////////////////////////////
void espReset() {
#if DebugSerial
  debugPrintln(F("RESET ESP8266"));
#endif
  if (client.connected()) client.disconnect();
  WiFi.disconnect();
  digitalWrite(Up1Pin, LOW);
  digitalWrite(Down1Pin, LOW);
  digitalWrite(Up2Pin, LOW);
  digitalWrite(Down2Pin, LOW);
  String position;
  if (activeBlind1){
    position = String  (position1 + 101);                                             //0 %->102, 100%->201
    writeFile(position, "/position1.txt");
  }
  if (activeBlind2) {
    position = String  (position2 + 101);                                             //0 %->102, 100%->201
    writeFile(position, "/position2.txt");
  }
  ESP.restart();
  delay(5000);
}
//////////////////////////////////////////////////////////////////////////////////////
// Enviamos configuración por MQTT
//////////////////////////////////////////////////////////////////////////////////////
void mqttConfig() {
  char payload[5];
  snprintf(payload, 4, "%d", delayUp1);
  mqttSend("/delayUp1", payload, false);
  snprintf(payload, 4, "%d", delayDown1);
  mqttSend("/delayDown1", payload, false);
  snprintf(payload, 4, "%d", delayUp2);
  mqttSend("/delayUp2", payload, false);
  snprintf(payload, 4, "%d", delayDown2);
  mqttSend("/delayDown2", payload, false);
  snprintf(payload, 4, "%d", activeBlind1);
  mqttSend("/activeBlind1", payload, false);
  snprintf(payload, 4, "%d", activeBlind2);
  mqttSend("/activeBlind2", payload, false);
  snprintf(payload, 4, "%d", modeLimit);
  mqttSend("/modeLimit", payload, false);
  snprintf(payload, 4, "%d", autoDetec);
  mqttSend("/autoDetec", payload, false);
  snprintf(payload, 4, "%d", alexaRun);
  mqttSend("/alexaRun", payload, false);
}
//////////////////////////////////////////////////////////////////////////////////////
// Indicamos que estamos pasando al modo punto de acceso
//////////////////////////////////////////////////////////////////////////////////////
void wifiAP() {
  modeAP();
#if DebugSerial
  debugPrintln(String(F("Configurado modo AP, nombre: ")) + String(nameAp) + " Pass: " + String(passAp));
  debugPrintln(String(F("IP: ")) + WiFi.softAPIP().toString());                       // Dirección para el AP
#endif
}
//////////////////////////////////////////////////////////////////////////////////////
// Obtenemos el periodo en marcha en segundos
//////////////////////////////////////////////////////////////////////////////////////
String getUptime() {
static unsigned long last_uptime = 0;
static unsigned char uptime_overflows = 0;
  if (millis() < last_uptime) ++uptime_overflows;
    last_uptime = millis();
    unsigned long uptime_seconds = uptime_overflows * (UPTIME_OVERFLOW / 1000) + (last_uptime / 1000);
#if DebugSerial
  debugPrintln(String(F("Periodo en marcha: ")) + String(uptime_seconds) + " Seg.");
#endif
    String GetT = String (uptime_seconds);
    return GetT;
}

//////////////////////////////////////////////////////////////////////////////////////
// Gestionamos la recepción de los topics
// topic:
// clientId /cover/nameLocate1-2/#
//  payload_stop
//  command  OPEN
//  command  CLOSE
//  command  STOP
//  position/command 0-100
// clientId /cover/nameLocate/switch/mode/set
// clientId /cover/nameLocate/cmd
//  restartCmd
//  updateCmd
//  configCmd
//  apmodeCmd
//////////////////////////////////////////////////////////////////////////////////////
void callback(char* topic, byte * payload, unsigned int length) {
#if DebugSerial == 2
  String buff = "";
  for (unsigned int i = 0; i < length; i++) {
    buff = buff + (char)payload[i];
  }
  debugPrintln(String(F("Topic recibido [")) + String(topic) + "] payload: " + buff + " Longitud: " + String (length));
#endif
  if (!strncmp((char *)payload, "restartCmd", length)) {                              // Comando reset?
#if DebugSerial == 2
    debugPrintln(F("Cmd MQTT Reiniciar"));
#endif
    espReset(); 
  }                   
  if (!strncmp((char *)payload, "updateCmd", length)) {                               // Comando actualizar MQTT
#if DebugSerial == 2
    debugPrintln(F("Cmd MQTT actualizar"));
#endif
    locate = 10;
    publicMqtt();
    return; 
  }
  if (!strncmp((char *)payload, "configCmd", length)){                                // Comando info. configuración
#if DebugSerial == 2
    debugPrintln(F("Cmd MQTT configuración"));
#endif
    mqttConfig();
    return;
  }
  if (!strncmp((char *)payload, "apmodeCmd", length))                                 { // Comando pasar a modo AP + web
#if DebugSerial == 2
    debugPrintln(F("Cmd Pasar a modo AP  (5 minutos)"));
#endif
    delayAP = 300000;                                                                 // 5 minutos en modo AP
    APmode = true;
    return;
  }
  String topicMode = clientId + String ("/cover/" + nameLocate) + String ("/switch/mode/set");
  if (!strcmp(topic, (char*) topicMode.c_str())) {                                    // Topic correcto
#if DebugSerial == 2
    debugPrintln(F("Orden MQTT manual-automatico"));
#endif
    if (!strncmp((char *)payload, "1", length)) {
      if (!modeHandbook) Anomalia (true);
    }
    else {
      if (modeHandbook) Anomalia (false);
    }
    return;
  }
  if (modeHandbook) {
#if DebugSerial == 2
    debugPrintln(F("SALIMOS MODO manual!!"));
#endif
    return;
  }
  String topicSTOP = clientId + String ("/cover/" + nameLocate) + "1/payload_stop";
  if (!strcmp(topic, (char*) topicSTOP.c_str())) {                                    // Topic correcto
  #if DebugSerial == 2
      debugPrintln(F("Orden MQTT payload STOP 1"));
#endif
    CommandSTOP1 = true;
    RelayOnOff (5, 0);
    bitClear(runBlind, 0);
    bitClear(runBlind, 1);
    bitClear(ReleFalg, 0);
    bitClear(ReleFalg, 1);
    return;
  }
  String topic1 = clientId + String ("/cover/" + nameLocate) + "1/command";
  if (!strcmp(topic, (char*) topic1.c_str())) {                                       // topic correcto
    if (!strncmp((char *)payload, "OPEN", length)) {
#if DebugSerial == 2
      debugPrintln(F("Orden MQTT abrir 1"));
#endif
      setPosition1 = 100;                                                             // Posición solicitada (ir a)
      CommandSTOP1 = false;
      bitClear(ReleFalg, 2);
      bitClear(ReleFalg, 3);
      RelayOnOff (1, 0);                                                              // Subir persiana 1
    }
    else if (!strncmp((char *)payload, "CLOSE", length)) {
#if DebugSerial == 2
      debugPrintln(F("Orden MQTT cerrar 1"));
#endif
      setPosition1 = 0;
      CommandSTOP1 = false;
      bitClear(ReleFalg, 2);
      bitClear(ReleFalg, 3);
      RelayOnOff (2, 0);
    }
    else if (!strncmp((char *)payload, "STOP", length)) {
#if DebugSerial == 2
      debugPrintln(F("Orden MQTT STOP 1"));
#endif
      CommandSTOP1 = true;
      RelayOnOff (5, 0);
      bitClear(runBlind, 0);
      bitClear(runBlind, 1);
      bitClear(ReleFalg, 0);
      bitClear(ReleFalg, 1);
    }
    return;
  }
  topic1 = clientId + String ("/cover/" + nameLocate) + "1/position/command";
  if (!strcmp(topic, (char*) topic1.c_str())) {
#if DebugSerial == 2
    debugPrintln(F("Comando MQTT 1"));
#endif
    if (length != 0 && length < 4) {
      payload[length] = '\0';
      uint16_t val = atoi((char *)payload);
#if DebugSerial
      debugPrintln(String(F("Ir a posición persiana 1: ")) + String(val));
#endif
      if (val < 101) {
        setPosition1 = val;                                                           // indicamos  hasta que porcentaje se debe abrir/cerrar
        if ((position1 - val) >= 0) {
          CommandSTOP1 = false;
          RelayOnOff (2, ((delayUp1 * 10) * (position1 - val)));                      // obtenemos el periodo a bajar
#if DebugSerial
          debugPrintln(String(F("Periodo bajar persiana 1: ")) + String((delayUp1 * 10) * (position1 - val)));
#endif
        }
        else {
          CommandSTOP1 = false;
          RelayOnOff (1, ((delayDown1 * 10) * ((position1 - val) * -1)));             // obtenemos el periodo a subir
#if DebugSerial
          debugPrintln(String(F("Periodo subir persiana 1: ")) + String((delayDown1 * 10) * ((position1 - val) * -1)));
#endif
        }
      }
    }
    return;
  }
  //******************Persiana 2******************************************************
  String  topic2STOP = clientId + String ("/cover/" + nameLocate) + "2/payload_stop";
  if (!strcmp(topic, (char*) topic2STOP.c_str())) {                                   // Topic correcto
#if DebugSerial == 2
    debugPrintln(F("Orden MQTT payload STOP 2"));
#endif
    setPosition2 = position2;
    CommandSTOP2 = true;
    RelayOnOff (6, 0);
    bitClear(runBlind, 2);
    bitClear(runBlind, 3);
    bitClear(ReleFalg, 2);
    bitClear(ReleFalg, 3);
    return;
  }
  String topic2 = clientId  + String ("/cover/" + nameLocate) + "2/command";
  if (!strcmp(topic, (char*) topic2.c_str())) {                                       // topic correcto
    if (modeHandbook) return;
    if (!strncmp((char *)payload, "OPEN", length)) {
#if DebugSerial == 2
      debugPrintln(F("Orden MQTT abrir 2"));
#endif
      setPosition2 = 100;
      CommandSTOP2 = false;
      bitClear(ReleFalg, 0);
      bitClear(ReleFalg, 1);
      RelayOnOff (3, 0);
    }
    else if (!strncmp((char *)payload, "CLOSE", length)) {
#if DebugSerial == 2
      debugPrintln(F("Orden MQTT cerrar 2"));
#endif
      setPosition2 = 0;
      CommandSTOP2 = false;
      bitClear(ReleFalg, 0);
      bitClear(ReleFalg, 1);
      RelayOnOff (4, 0);
    }
    else if (!strncmp((char *)payload, "STOP", length)) {
#if DebugSerial == 2
      debugPrintln(F("Orden MQTT STOP 2"));
#endif
      CommandSTOP2 = true;
      RelayOnOff (6, 0);
      bitClear(runBlind, 2);
      bitClear(runBlind, 3);
      bitClear(ReleFalg, 2);
      bitClear(ReleFalg, 3);
    }
    return;
  }
  topic2 = clientId + String ("/cover/" + nameLocate) + "2/position/command";
  if (!strcmp(topic, (char*) topic2.c_str())) {
#if DebugSerial == 2
    debugPrintln(F("Comando MQTT 2"));
#endif
    if (length != 0 && length < 4) {
      payload[length] = '\0';
      uint16_t val = atoi((char *)payload);
#if DebugSerial
      debugPrintln(String(F("Ir a posición persiana 2: ")) + String(val));
#endif
      if (val < 101) {
        setPosition2 = val;                                                           // indicamos  hasta que porcentaje se debe abrir/cerrar
        if ((position2 - val) >= 0) {
          CommandSTOP2 = false;
          RelayOnOff (4, ((delayUp2 * 10) * (position2 - val)));                      // obtenemos el periodo a bajar
#if DebugSerial
          debugPrintln(String(F("Periodo bajar persiana 2: ")) + String((delayUp2 * 10) * (position2 - val)));
#endif
        }
        else {
          CommandSTOP2 = false;
          RelayOnOff (3, ((delayDown2 * 10) * ((position2 - val) * -1)));             // obtenemos el periodo a subir
#if DebugSerial
          debugPrintln(String(F("Periodo subir persiana 2: ")) + String((delayDown2 * 10) * ((position2 - val) * -1)));
#endif
        }
      }
    }
    return;
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// Publicamos en el broker MQTT
//////////////////////////////////////////////////////////////////////////////////////
void mqttSend(String topic, char* topublish, bool forceRetain) {
  if (APmode) return;
  if (client.connected()) {
    topic = clientId + topic;
    client.publish((char*) topic.c_str(), topublish, forceRetain ? true: MQTT_RETAIN);
#if DebugSerial == 2
    debugPrintln(String(F("Publicado topic: ")) + topic + " valor: " + topublish);
#endif
  } else {
#if DebugSerial
    debugPrintln(String(F("Fallo en conexión con el broker, rc=")) + String(client.state()) + " intentando RE-conectar");
#endif
    reConnect();
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// Gestionamos los mensajes a enviar/publicar al broker MQTT
//////////////////////////////////////////////////////////////////////////////////////
void publicMqtt() {
  static uint8_t info = 0;
  char payload[5];
  if (locate == 10) {                                                                 // Forzar update ?
    locate = 0;
    info = 0;
  }
  int32_t LevelRssi = WiFi.RSSI();
  if (++info == 1) {
    mqttSend ("/app", (char*) "Radioelf", false);
    mqttSend ("/version", (char*)  sofVersion, false);
    mqttSend ("/board", (char*) "Cover_RADIOELF", false);
    mqttSend ("/host", (char*) clientId.c_str(), false);
    mqttSend ("/desc", (char*) "RELAY_COVER", false);
    mqttSend ("/ssid", (char*) ssid, false);
    mqttSend ("/ip", (char*) WiFi.localIP().toString().c_str(), false);
    mqttSend ("/mac", (char*) WiFi.macAddress().c_str(), false);
    snprintf(payload, 4, "%d", LevelRssi);
    mqttSend ("/rssi", payload, false);
    snprintf(payload, 5, "%d", ESP.getVcc());
    mqttSend ("/vcc", payload, false);
#if DebugSerial
    debugPrintln(String(F("Memoria libre de la pila DRAM: ")) + String(ESP.getFreeHeap()));
#endif
  }

  if (info == 4) info = 0;
  String statusTx = "offline";
  if (locate) statusTx = "online";
  mqttSend ("/status", (char*) statusTx.c_str(), false);
  
  if (LevelRssi <= -80) {                                                             // Si el nivel de RF es muy bajo (-50 a -100 dBm)
    snprintf(payload, 4, "%d", LevelRssi);
    mqttSend ("/rssi", payload, false);
  }
  // ERRORES: 1->leer FS, 2->escribir FS, 3->OTA, 4->anomalía, 5->DHT11, 100->varios errores
  if (error != 0){                                                                    
    snprintf(payload, 3, "%d", error);
    mqttSend ("/error", payload, false);
#if DebugSerial
    debugPrintln(String(F("Error: ")) + String(error));
    debugPrintln(String(F("1->leer FS, 2->escribir FS, 3->OTA, 4->anomalía, 5->DHT11, 100->varios errores")));
#endif
    error = 0;
  }

  modeHandbook ? statusTx = "1" : statusTx = "0";
  String messageMode = String(F("/cover/")) + String (nameLocate) + String(F("/switch/mode"));
  mqttSend (messageMode, (char*) statusTx.c_str(), false);

  String message_;
  uint16_t messageLeng_;

  message_ = String(F("/sensor/uptime_")) + String (nameLocate) + String(F("/state"));
  messageLeng_ = message_.length();
  char msg_t[messageLeng_];
  message_.toCharArray(msg_t, (messageLeng_ + 1));
  mqttSend (msg_t, (char*) getUptime().c_str(), false);

#if DHTsensor
  if (OkDht) {
    message_ = String(F("/sensor/temperatura_")) + String (nameLocate) + String(F("/state"));
    messageLeng_ = message_.length();
    char msg_t[messageLeng_];
    message_.toCharArray(msg_t, (messageLeng_ + 1));
    snprintf(payload, 3, "%d", tempInt);                                              // Convertimos temperatura en un array de caracteres
    mqttSend (msg_t, payload, false);
    message_ = String(F("/sensor/humedad_")) + String (nameLocate) + String(F("/state"));
    messageLeng_ = message_.length();
    char msg_h[message_.length()];
    message_.toCharArray(msg_h, (messageLeng_ + 1));
    snprintf(payload, 3, "%d", humedadInt);
    mqttSend (msg_h, payload, false);
  }
#endif

  if (activeBlind1) {
    message_ = String(F("/cover/")) + String (nameLocate) + String(F("1/position/command"));
    messageLeng_ = message_.length();
    char msg_upDown1[messageLeng_];
    message_.toCharArray(msg_upDown1, (messageLeng_ + 1));
    String statusBlind1 = "stop";
    if (digitalRead(Up1Pin)) {
      statusBlind1 = "open";
    }
    else if (digitalRead(Down1Pin)) {
      statusBlind1 = "closed";
    }
    mqttSend (msg_upDown1, (char*) statusBlind1.c_str(), false);
    // Indicamos persiana abierta o cerrada ON abierta, OFF cerrada
    mqttSend (("/switch/" + nameLocate + "1/open_cover"), position1 > 5 ? (char*) "on": (char*) "off", false);

    message_ = String(F("/cover/")) + String (nameLocate) + String(F("1/position/state"));
    messageLeng_ = message_.length();
    char msg_position1[messageLeng_];
    message_.toCharArray(msg_position1, (messageLeng_ + 1));
    if (modeHandbook) {
      mqttSend (msg_position1, (char*) "50", false);
    }
    else{
      snprintf(payload, 4, "%d", position1);
      mqttSend (msg_position1, payload, false);
    }
    // Indicamos persiana opening - closing -unknown (actualiza estado de la persiana en Home Assistant)
    if (info == 1) {
      statusBlind1 = "unknown";
      if (position1 <= 100 && position1 > 5) {
        statusBlind1 = "open";
      }
      else if (position1 >= 0 && position1 < 6){
        statusBlind1 = "closed"; 
      }
      mqttSend(("/cover/" + nameLocate + "1/state"), (char *)statusBlind1.c_str(), false);
    }
  }
  if (activeBlind2) {
    message_ = String(F("/cover/")) + String (nameLocate) + String(F("2/position/command"));
    messageLeng_ = message_.length();
    char msg_upDown2[messageLeng_];
    message_.toCharArray(msg_upDown2, (messageLeng_ + 1));
    String  statusBlind2 = "stop";
    if (digitalRead(Up2Pin)) {
      statusBlind2 = "open";
    }
    else if (digitalRead(Down2Pin)) {
      statusBlind2 = "closed"; 
    }
    mqttSend (msg_upDown2, (char*) statusBlind2.c_str(), false);
    // Indicamos persiana abierta o cerrada ON abierta, OFF cerrada
    mqttSend(("/switch/" + nameLocate + "2/open_cover"), position2 > 5 ? (char *)"on" : (char *)"off", false);

    message_ = String(F("/cover/")) + String (nameLocate) + String(F("2/position/state"));
    messageLeng_ = message_.length();
    char msg_position2[messageLeng_];
    message_.toCharArray(msg_position2, (messageLeng_ + 1));
    if (modeHandbook) {
      mqttSend (msg_position2, (char*) "50", false);
    }
    else{
      snprintf(payload, 4, "%d", position2);
      mqttSend (msg_position2, payload, false);
    }
    // Indicamos persiana opening - closing -unknown (actualiza estado de la persiana en Home Assistant)
    if (info == 1) {
      statusBlind2 = "unknown";
      if (position2 <= 100 && position2 > 5) {
        statusBlind2 = "open";
      }
      else if (position2 >= 0 && position2 < 6) {
        statusBlind2 = "closed"; 
      }
      mqttSend(("/cover/" + nameLocate + "2/state"), (char *)statusBlind2.c_str(), false);
    }
  }

  if (!locate) {
    //Auto descubrimiento para Home Assistant <discovery_prefix>/<component>/[<node_id>/]<object_id>/config
    uint8_t nCover = 0;
    uint8_t mac[6];
    WiFi.macAddress(mac);
    String Mac = String(mac[0], 16) + String(mac[1], 16) + String(mac[2], 16) + String(mac[3], 16) + String(mac[4], 16) + String(mac[5], 16);

    while (nCover < (activeBlind1 + activeBlind2)) {
      nCover ++;
      String message = "{";
      message += String(F("\"optimistic\":\"true\",\"position_topic\":\"")) + String(clientId) + String(F("/cover/")) + String (nameLocate + nCover) + String(F("/position/state""\",\""));
      message += String(F("set_position_topic\":\"")) + String(clientId) + String(F("/cover/")) + String (nameLocate + nCover) + String(F("/position/command""\",\""));
      message += String(F("name\":\"")) + String (nameLocate + nCover) + String(F("\",\""));
      message += String(F("state_topic\":\"")) + String(clientId) + String(F("/cover/")) + String (nameLocate + nCover)  + String(F("/state\",\""));
      message += String(F("command_topic\":\"")) + String(clientId) + String(F("/cover/")) + String (nameLocate + nCover) + String(F("/command\",\""));
      message += String(F("availability_topic\":\"")) + String(clientId) + String(F("/status\",\""));
      message += String(F("unique_id\":\"")) + String(F("ESPcover")) + String (nameLocate + nCover) + String(F("\",\""));
      message += String(F("device\":{\"identifiers\":\"")) + String (Mac) + String(F("\",\""));
      message += String(F("name\":\"Persiana "))+ String (nameLocate) + String (F("\",\"sw_version\":\"COVER "));
      message += String(sofVersion) + "-" + String(Compiler) + String(F("\",\"model\":\"COVER1\",\"manufacturer\":\"Radioelf\"}"));
      message += "}";

      String topic = ("homeassistant/cover/" + clientId + "/" + String (nameLocate + nCover) + "/config");
      uint16_t messageLeng = message.length();
      char msg[messageLeng];
      message.toCharArray(msg, (messageLeng + 1));
      client.publish((char*) topic.c_str(), msg, messageLeng);
    }
    String message = "{";
    message += String(F("\"name\":\"Modo_")) + String (nameLocate) + String(F("\",\""));
    message += String(F("icon\":\"mdi:toggle-switch\",\""));
    message += String(F("state_topic\":\"")) + String(clientId) + String(F("/cover/")) + String (nameLocate) + String(F("/switch/mode")) + String(F("\",\""));
    message += String(F("command_topic\":\"")) + String(clientId) + String(F("/cover/")) + String (nameLocate) + String(F("/switch/mode")) + String(F("/set\",\""));
    message += String(F("payload_on\":\"1\",\"payload_off\":\"0\",\""));
    message += String(F("availability_topic\":\"")) + String(clientId) + String(F("/status\",\""));
    message += String(F("unique_id\":\"")) + String(clientId) + String(F("_switch")) + String (nameLocate) + String(F("\",\""));
    message += String(F("device\":{\"identifiers\":\"")) + String (Mac) + String(F("\",\""));
    message += String(F("name\":\"Persiana "))+ String (nameLocate) + String (F("\",\"sw_version\":\"COVER "));
    message += String(sofVersion) + "-" + String(Compiler) + String(F("\",\"model\":\"COVER1\",\"manufacturer\":\"Radioelf\"}"));
    message += "}";

    String topic_s = ("homeassistant/switch/" + clientId + "_estado" + "/config");
    uint16_t messageLeng_s = message.length();
    char msg_s[messageLeng_s];
    message.toCharArray(msg_s, (messageLeng_s + 1));
    client.publish((char*) topic_s.c_str(), msg_s, messageLeng_s);

    message = "{";
    message += String(F("\"stat_t\":\"")) + String(clientId) + String(F("/status\",\""));
    message += String(F("name\":\"SYS: Connectivity")) + String(clientId) + String(F("\",\""));
    message += String(F("unique_id\":\"")) + String (clientId ) + String(F("connectivity\",\""));
    message += String(F("dev_cla\":\"connectivity\",\"pl_on\":\"online\",\"pl_off\":\"offline\",\""));
    message += String(F("pl_avail\":\"online\",\"pl_not_avail\":\"offline\",\""));
    message += String(F("device\":{\"identifiers\":\"")) + String (Mac) + String(F("\",\""));
    message += String(F("name\":\"Persiana "))+ String (nameLocate) + String (F("\",\"sw_version\":\"COVER "));
    message += String(sofVersion) + "-" + String(Compiler) + String(F("\",\"model\":\"COVER1\",\"manufacturer\":\"Radioelf\"}"));
    message += "}";

    String topic_Conect = ("homeassistant/binary_sensor/" + clientId + "connectivity/config");
    uint16_t messageLeng_Status = message.length();
    char msg_Status[messageLeng_Status];
    message.toCharArray(msg_Status, (messageLeng_Status + 1));

    message = "{";
    message += String(F("\"unit_of_measurement\":\"seg\",\"icon\":\"mdi:timeline-clock\",\""));
    message += String(F("name\":\"Uptime ")) + String (nameLocate) + String(F("\",\""));
    message += String(F("state_topic\":\"")) + String(clientId) + String(F("/sensor/uptime_")) + String (nameLocate)  + String(F("/state\",\""));
    message += String(F("availability_topic\":\"")) + String(clientId) + String(F("/status\",\""));
    message += String(F("unique_id\":\"")) + String(F("ESPuptime")) + String (nameLocate) + String(F("\",\""));
    message += String(F("device\":{\"identifiers\":\"")) + String (Mac) + String(F("\",\""));
    message += String(F("name\":\"Persiana "))+ String (nameLocate) + String (F("\",\"sw_version\":\"COVER "));
    message += String(sofVersion) + "-" + String(Compiler) + String(F("\",\"model\":\"COVER1\",\"manufacturer\":\"Radioelf\"}"));
    message += "}";

    String topic_time = ("homeassistant/sensor/" + clientId + "/uptime_" + String (nameLocate) + "/config");
    uint16_t messageLeng_time = message.length();
    char msg_time[messageLeng_time];
    message.toCharArray(msg_time, (messageLeng_time + 1));
    client.publish((char*) topic_time.c_str(), msg_time, messageLeng_time);

#if DHTsensor
    client.publish((char*) topic_Conect.c_str(), msg_Status, messageLeng_Status);
#else
    locate = client.publish((char*) topic_Conect.c_str(), msg_Status, messageLeng_Status);
#endif
#if DHTsensor
    message = "{";
    message += String(F("\"unit_of_measurement\":\"°C\",\"icon\":\"mdi:thermometer\",\""));
    message += String(F("name\":\"Temperatura ")) + String (nameLocate) + String(F("\",\""));
    message += String(F("state_topic\":\"")) + String(clientId) + String(F("/sensor/temperatura_")) + String (nameLocate)  + String(F("/state\",\""));
    message += String(F("availability_topic\":\"")) + String(clientId) + String(F("/status\",\""));
    message += String(F("unique_id\":\"")) + String(F("ESPtemperatura")) + String (nameLocate) + String(F("\",\""));
    message += String(F("device\":{\"identifiers\":\"")) + String (Mac) + String(F("\",\""));
    message += String(F("name\":\"Persiana "))+ String (nameLocate) + String (F("\",\"sw_version\":\"COVER "));
    message += String(sofVersion) + "-" + String(Compiler) + String(F("\",\"model\":\"COVER1\",\"manufacturer\":\"Radioelf\"}"));
    message += "}";

    String topic_t = ("homeassistant/sensor/" + clientId + "/temperatura_" + String (nameLocate) + "/config");
    uint16_t messageLeng_t = message.length();
    char msg_t[messageLeng_t];
    message.toCharArray(msg_t, (messageLeng_t + 1));
    client.publish((char*) topic_t.c_str(), msg_t, messageLeng_t);

    message = "{";
    message += String(F("\"unit_of_measurement\":\"%\",\"icon\":\"mdi:water-percent\",\""));
    message += String(F("name\":\"Humedad ")) + String (nameLocate) + String(F("\",\""));
    message += String(F("state_topic\":\"")) + String(clientId) + String(F("/sensor/humedad_")) + String (nameLocate)  + String(F("/state\",\""));
    message += String(F("availability_topic\":\"")) + String(clientId) + String(F("/status\",\""));
    message += String(F("unique_id\":\"")) + String(F("ESPhumedad")) + String (nameLocate) + String(F("\",\""));
    message += String(F("device\":{\"identifiers\":\"")) + String (Mac) + String(F("\",\""));
    message += String(F("name\":\"Persiana "))+ String (nameLocate) + String (F("\",\"sw_version\":\"COVER "));
    message += String(sofVersion) + "-" + String(Compiler) + String(F("\",\"model\":\"COVER1\",\"manufacturer\":\"Radioelf\"}"));
    message += "}";

    String topic_h = ("homeassistant/sensor/" + clientId + "/humedad_" + String (nameLocate) + "/config");
    messageLeng_t = message.length();
    char msg_h[messageLeng_t];
    message.toCharArray(msg_h, (messageLeng_t + 1));
    locate = client.publish((char*) topic_h.c_str(), msg_h, messageLeng_t);
#endif

    if (locate ) mqttSend ("/status", (char*) "online", true);
#if DebugSerial
    debugPrintln("Auto descubrimiento Home Assistant");
#endif
  }
  client.endPublish();
}

//////////////////////////////////////////////////////////////////////////////////////
// Gestión conexión/reconexión Wifi + MQTT
//////////////////////////////////////////////////////////////////////////////////////
void reConnect() {
  uint8_t conectOK = 0;
  static uint8_t NoConect = 0;
  static uint8_t NoMqtt = 0;
  WiFi.disconnect();
  if (WiFi.status() != WL_CONNECTED) {                                                // Re-intentamos conectarse a wifi si se pierde la conexión
    locate = false;                                                                   // Indicamos reenviar auto descubrimiento
    if (APmode) WiFi.softAPdisconnect();                                        
    APmode = false;  
#if DebugSerial
    debugPrintln(String(F("Conectando a: ")) + String (ssid));
#endif
    WiFi.mode(WIFI_STA);                                                              // Solo usamos el modo STA y desactivar el AP
    WiFi.config(staticIP, dnServer, gateway, subnet);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {                                           // Permanecemos mientras esperamos la conexión
      delay(500);
#if DebugSerial
      debugPrintln(F("."));
#endif
      if (++conectOK == 50) {
        ++NoConect;                                                                   // 500 * 50 =25000-> 25 segundos
        if (listNetworks(ssid)) {
          conectOK = 0;
#if DebugSerial
          debugPrintln(F("Encontrado Punto de Acceso correcto..."));
#endif
        }
        else {
#if DebugSerial
          debugPrintln(F("NO encontrado Punto de Acceso..."));
#endif
          wifiAP();
          if (++NoConect == 10) espReset();                                           // SIN conectar 25*10=250/60=4.1 minutos (ap NO OK)
          return;
        }
      }
      if (NoConect > 24) espReset();                                                  // SIN conectar 25*24=600/60=10 minutos (ap OK)
    }
  }
  // !! CONECTADOS !!
  // Creamos el nombre del cliente basado en la dirección MAC y los últimos 3 bytes
  //uint8_t mac[6];
  //WiFi.macAddress(mac);
  //clientId +=  "-" + String(mac[3], 16) + String(mac[4], 16) + String(mac[5], 16);
  //clientId.toUpperCase();
  okMqtt = false;
  NoConect = 0;
  conectOK = 25;                                                                      // Número de re intentos de conexión MQTT
  if (WiFi.status() == WL_CONNECTED) {
#if DebugSerial
    debugPrintln(String(F("WIFI: Conexión OK con IP: ")) + WiFi.localIP().toString());
    debugPrintln(String(F("WIFI: mascara de subred: ")) + WiFi.subnetMask().toString());
    debugPrintln(String(F("WIFI: gateway: ")) + WiFi.gatewayIP().toString());
    debugPrintln(String(F("WIFI: DNS: ")) + WiFi.dnsIP().toString());
    debugPrintln(String(F("WIFI: MAC ESP: ")) + WiFi.macAddress().c_str());
    debugPrintln(String(F("WIFI: HOST: ")) + WiFi.hostname().c_str() + ".local");

    debugPrintln(String(F("WIFI: BSSID: ")) + WiFi.BSSIDstr().c_str());
    debugPrintln(String(F("WIFI: CH: ")) + WiFi.channel());
    debugPrintln(String(F("WIFI: RSSI: ")) + WiFi.RSSI());
    // Conexión al broker MQTT
    // Cada mensaje MQTT puede ser enviado como un mensaje con retención (retained), en este caso cada
    // nuevo cliente que conecta a un topic recibirá el último mensaje retenido de ese tópico.
    // Cuando un cliente conecta con el Broker puede solicitar que la sesión sea persistente, en ese
    // caso el Broker almacena todas las suscripciones del cliente.
    // Un mensaje MQTT CONNECT contiene un valor keepAlive en segundos donde el cliente establece el
    // máximo tiempo de espera entre intercambio de mensajes
    // QOS
    // 0: El broker/cliente entregará el mensaje una vez, sin confirmación. (Baja/rápido)
    // 1: El broker/cliente entregará el mensaje al menos una vez, con la confirmación requerida. (Media)
    // 2: El broker/cliente entregará el mensaje exactamente una vez. (Alta/lento)
    debugPrintln(String(F("Config MQTT: ")) + "clientID: " + clientId.c_str() + + " Broker: " + String (MQTT_SERVER) + " username: "
                 + userMQTT + " password: " + passMQTT + " willTopic: " + String (willTopic) + " MQTT_QOS: "
                 + String (MQTT_QOS) + " MMQTT_RETAIN: " + String (MQTT_RETAIN) + " willMessage: " + String (willMessage));
    debugPrintln("Intentando de conectar a MQTT...");
#endif
    client.disconnect();
    client.setClient(espClient);
    client.setServer(MQTT_SERVER, MQTT_PORT);

    while (!client.connected()) {                                                     // Permanecemos mientras NO estemos conectados al servidor MQTT
      //                              clientID          username  password  willTopic  willQoS,  willRetain,  willMessage,  cleanSession =1 (default)
      okMqtt = client.connect((char*) clientId.c_str(), userMQTT, passMQTT, willTopic, MQTT_QOS, MQTT_RETAIN, willMessage, 0);
      if (!okMqtt) {
        if (conectOK-- == 0) break;
        delay(2500);
#if DebugSerial
        // Respuesta a client.state()
        //  -4: MQTT_CONNECTION_TIMEOUT- el servidor no respondió dentro del periodo esperado
        //  -3: MQTT_CONNECTION_LOST- la conexión de red se interrumpió
        //  -2: MQTT_CONNECT_FAILED- la conexión de red falló
        //  -1: MQTT_DISCONNECTED- el cliente está desconectado limpiamente
        //  0: MQTT_CONNECTED el cliente está conectado
        //  1: MQTT_CONNECT_BAD_PROTOCOL el servidor no admite la versión solicitada de MQTT
        //  2: MQTT_CONNECT_BAD_CLIENT_ID el servidor rechazó el identificador del cliente
        //  3: MQTT_CONNECT_UNAVAILABLE- el servidor no pudo aceptar la conexión
        //  4: MQTT_CONNECT_BAD_CREDENTIALS- el nombre de usuario/contraseña NO validos
        //  5: MQTT_CONNECT_UNAUTHORIZED- el cliente no estaba autorizado para conectarse
        debugPrintln(String(F("Fallo al conectar al broker, rc=")) + String(client.state()) + " intentando conectar en 5 segundos, " + String (conectOK));
#endif
      }
    }
    if (okMqtt) {
      if (Fs) {
        String stateMode = readFile("/state.txt");
        stateMode == "on" ? modeHandbook = true : modeHandbook = false;
      }
      publicMqtt();
      NoMqtt = 0;
#if DebugSerial
      debugPrintln("Conectado a MQTT");
      debugPrintln("Publicación de Topics enviada");
#endif
      if (activeBlind1) {
        const String topicSub1 = clientId  + "/cover/" + String (nameLocate) + "1/#";
        client.subscribe((char*)topicSub1.c_str(), 0);                                // Suscripción a topics topic persiana 1, QoS 0
#if DebugSerial
        debugPrintln(String(F("Subscrito a:")) + topicSub1);
#endif
      }
      if (activeBlind2) {
        const String topicSub2 = clientId  + "/cover/" + String (nameLocate) + "2/#";
        client.subscribe((char*)topicSub2.c_str(), 0);                                // Suscripción a topics topic persiana 2, QoS 0
#if DebugSerial
        debugPrintln(String(F("Subscrito a:")) + topicSub2);
#endif
      }
      const String topicSub3 = clientId  + "/cover/" + String (nameLocate) + "/switch/mode/#";
      client.subscribe((char*)topicSub3.c_str(), 0);                                  // Suscripción a topics topic interruptor modo, QoS 0
#if DebugSerial
      debugPrintln(String(F("Subscrito a:")) + topicSub3);
#endif
      const String topicSub4 = clientId  + "/cover/" + String (nameLocate) + "/cmd/#";//nombre_BLIND-ffffff/cover/nombre/cmd
      client.subscribe((char*)topicSub4.c_str(), 0);                                  // Suscripción a topics topic comandos, QoS 0
#if DebugSerial
      debugPrintln(String(F("Subscrito a:")) + topicSub4);
#endif
#if DebugSerial
      debugPrintln("Suscripción a topics enviada");
#endif                                 
    }
    else {
      wifiAP();
      if (++NoMqtt == 50) espReset();
      return;
    }
  }
  else {
#if DebugSerial
      debugPrintln("ERROR conexión !NO se tendría que ejecutar!");
#endif 
    espReset();                                                                       // NO se tendría que ejecutar
  }
}
//////////////////////////////////////////////////////////////////////////////////////
// Gestión OFF relés persianas
//////////////////////////////////////////////////////////////////////////////////////
bool OffRelay(uint8_t option) {
  if (modeHandbook) return false;                                                     // Modo manual, salimos
  if (option == 1 || option == 3) {
    digitalWrite(Up1Pin, LOW);
    digitalWrite(Down1Pin, LOW);
    periodFcI1 = millis() + OnOffI_FC;
  }
  if (option > 1) {
    digitalWrite(Up2Pin, LOW);
    digitalWrite(Down2Pin, LOW);
    periodFcI2 = millis() + OnOffI_FC;
  }
  if (modeLimit == 2) {                                                               // Si usamos FC por intensidad
    uint16_t x = 0;
    do {
      delayMicroseconds(100);
      x++;
      if (x == 5000) {                                                                // Aproximadamente 600 ciclos (OK)
        if (!modeHandbook) Anomalia(true);
#if DebugSerial
        debugPrintln(F("Anomalía tiempo máximo ON F.C"));
#endif
        return false;
      }
    } while (digitalRead(FC_1) || digitalRead(FC_2));                                 // Esperamos fin FCs
    if (x < 2) {
      if (!modeHandbook) Anomalia(true);
#if DebugSerial
      debugPrintln(F("Anomalía tiempo mínimo OFF F.C"));
#endif
      return false;
    }
    iniRunFc1 = true;
    iniRunFc2 = true;
  }
  return true;
}
//////////////////////////////////////////////////////////////////////////////////////
// Control relés
// Relay 1-> subir persiana 1
// Relay 2-> bajar persiana 1
// Relay 5-> stop persiana 1
// Relay 3-> subir persiana 2
// Relay 4-> bajar persiana 2
// Relay 6-> stop persiana 2
//////////////////////////////////////////////////////////////////////////////////////
void RelayOnOff(uint8_t Relay, uint16_t runPeriod) {
  if (modeHandbook) {
    Anomalia (true);
    return;                                                                           // Modo manual, salimos
  }
  char payload[4];
  if (Relay < 5) {                                                                    // NO STOP
#if DebugSerial
    if (runPeriod == 0) {
      if (Relay > 2) {
        debugPrintln(String(F("RUN control relés, periodo marcha persiana 2 de: ")) + String(delayUp2 * 1000) + " ms");
      }
      else {
        debugPrintln(String(F("RUN control relés, periodo marcha persiana 1 de: ")) + String(delayUp1 * 1000) + " ms");
      }
    }
#endif
    if ((iniRunFc1 == false || iniRunFc2 == false) && runPeriod) {
      if (Relay > 2) {
        iniRunFc2 = true;                                                             // Inicializado FC persiana 2
#if DebugSerial
        debugPrintln(F("Posición persiana 2 conocida"));
#endif
      }
      else {
        iniRunFc1 = true;                                                             // Inicializado FC persiana 1
#if DebugSerial
        debugPrintln(F("Posición persiana 1 conocida"));
#endif
      }
    }
    if (iniRunFc1 == false) {                                                         // Posición desconocida?
      runPeriod = 0;
      if (Relay == 1) position1 = 0;                                                  // Subir, si no es conocida se inicia en posición 0
      if (Relay == 2) position1 = 100;                                                // Bajar, si no es conocida se inicia en posición 100
#if AllowHandbook == 0
      if (modeLimit == 2){                                                            // Nos aseguramos de que no se encuentre pulsado bajar manualmente
        if (Relay == 1){                                                              // comprobamos si el se encuentra activo el pulsador manual de bajar
          digitalWrite(Up1Pin, HIGH);                                                 // ON relé subir
          delay (500);
          digitalWrite(Up1Pin, LOW);                                                  // OFF relé subir
          uint16_t OnFC1 = 500;                                                       // periodo MÁXIMO de transición a reposo del FC por intensidad
          do {
            delay (1);
            if (!OnFC1--) {
#if DebugSerial  
              debugPrintln("Atención manual bajar persiana 1 activado!!");
#endif
              Anomalia (true);
              return;
            }
          } while (digitalRead(FC_1));                                                // Esperamos si final carrera activo
#if DebugSerial
          debugPrintln(String(F("F.C activo durante ")) + String(500 - OnFC1) + " ms");
#endif
        }
        if (Relay == 2){
          digitalWrite(Down1Pin, HIGH);                                               // ON relé bajar
          delay (500);
          digitalWrite(Down1Pin, LOW);                                                // OFF relé bajar
          uint16_t OnFC1 = 500;                                                       // periodo MÁXIMO de transición a reposo del FC por intensidad
          do {
            delay (1);
            if (!OnFC1--) {
#if DebugSerial  
            debugPrintln("Atención manual subir persiana 1 activado!!");
#endif
            Anomalia (true);
            return;
            }
          } while (digitalRead(FC_1));
#if DebugSerial
          debugPrintln(String(F("F.C activo durante ")) + String(500 - OnFC1) + " ms");
#endif
        }
      }
#endif
#if DebugSerial  
      debugPrintln("Posición de persiana 1 NO conocida");
#endif
    }

    if (iniRunFc2 == false && activeBlind2) {                                         // Posición desconodida de pesina 2
      runPeriod = 0;
      if (Relay == 3) position2 = 0;                                                  // Subir, si no es conocida se inicia en posición 0
      if (Relay == 4) position2 = 100;                                                // Bajar, si no es conocida se inicia en posición 100
#if AllowHandbook == 0
      if (modeLimit == 2) {                                                           // Nos aseguramos de que no se encuentre pulsado bajar manualmente
        if (Relay == 3){
          digitalWrite(Up2Pin, HIGH);                                                 // ON relé subir persiana 2
          delay (500);
          digitalWrite(Up2Pin, LOW);                                                  // OFF relé subir persiana 2
          uint16_t  OnFC2 = 500;                                                      // periodo MÁXIMO de transición a reposo del FC por intensidad
          do {
            delay (1);
            if (!OnFC2--) {
#if DebugSerial  
              debugPrintln("Atención manual bajar persiana 2 activado!!");
#endif
              Anomalia (true);
              return;
            }
          } while (digitalRead(FC_2));
#if DebugSerial
          debugPrintln(String(F("F.C activo durante ")) + String(500 - OnFC2) + " ms");
#endif
        }
        if (Relay == 4){
          digitalWrite(Down2Pin, HIGH);                                               // ON relé bajar persiana 2
          delay (500);
          digitalWrite(Down2Pin, LOW);                                                // OFF relé bajar persiana 2
          uint16_t OnFC2 = 500;                                                       // periodo MÁXIMO de transición a reposo del FC por intensidad
          do {
            delay (1);
            if (!OnFC2--) {
#if DebugSerial  
            debugPrintln("Atención manual subir persiana 2 activado!!");
#endif
            Anomalia (true);
            return;
            }
          } while (digitalRead(FC_2));
#if DebugSerial
          debugPrintln(String(F("F.C activo durante ")) + String(500 - OnFC2) + " ms");
#endif
        }
      }
#endif
#if DebugSerial  
      debugPrintln("Posición de persiana 2 NO conocida");
#endif
    }
  }
  switch (Relay) {
//****************************SUBIR 1*************************************************
    case 1:                                                                           // Relé subir persiana 1
      if (digitalRead (Up1Pin) || position1 == setPosition1) {			                  // Subiendo o posición actual igual a la solicitada?
        mqttSend (("/cover/" + nameLocate + "1/operation"), (char*) "OPENING", false);
        snprintf(payload, 4, "%d", position1);
        mqttSend ("/cover/" + nameLocate + "1/position/state", payload, false);
        mqttSend (("/switch/" + nameLocate + "1/open_cover"), digitalRead (Up1Pin) ? (char*) "ON": (char*) "OFF", false);
        return;                                                                       // Si estamos subiendo salimos
      }
      digitalWrite(Down1Pin, LOW);                                                    // Nos aseguramos que el relé de bajar persiana 1 se encuentre parado
      bitClear(ReleFalg, 1);                                                          // Borramos bandera de indicación bajando persiana 1
      digitalWrite(Up1Pin, HIGH);                                                     // Activamos el relé de subir la persiana 1
#if DebugSerial
          debugPrintln("ON RELE subir persiana 1");
#endif
      if (runPeriod == 0) {
        tickerMs1.attach_ms(map(position1, 0, 100, (delayUp1 * 1000), 0), toggleseconds1); // Inicializamos timer subir persiana 1
        StopIsr1 = false;                                                             // indicamos arranque periodo ISR (ON relés)
      }
      else {
        tickerMs1.attach_ms(map(position1, 0, 100, runPeriod, 0), toggleseconds1);    // Inicializamos timer subir persiana 1
        StopIsr1 = false;
      }
      mqttSend (("/switch/" + nameLocate + "1/open_cover"), (char*) "Turning ON", false);// publicamos estado en movimiento
      mqttSend (("/switch/" + nameLocate + "1/open_cover"), (char*) "on", false);
      mqttSend (("/cover/" + nameLocate + "1/operation"), (char*) "opening", false);
      bitSet (ReleFalg, 7);                                                           // Indicamos relé activado
      bitSet (ReleFalg, 0);                                                           // Indicamos relé subir persiana 1 activo
      periodRun1 = millis();
      break;
//***********************BAJAR 1************************************************
    case 2:                                                                           // Relé bajar persiana 1
      if (digitalRead (Down1Pin) || position1 == setPosition1) {                      // Relé bajar en marcha? o posición OK?
        mqttSend (("/cover/" + nameLocate + "1/operation"), (char*) "closing", false);
        snprintf(payload, 4, "%d", position1);
        mqttSend ("/cover/" + nameLocate + "1/position/state", payload, false);
        mqttSend (("/switch/" + nameLocate + "1/open_cover"), digitalRead (Down1Pin) ? (char*) "on": (char*) "off", false);
        return;
      }
      digitalWrite(Up1Pin, LOW);
      bitClear(ReleFalg, 0);                                                          // Borramos bandera de indicación subiendo persiana 1
      digitalWrite(Down1Pin, HIGH);                                                   // Activamos el relé de bajar la persiana 1
#if DebugSerial
      debugPrintln("ON RELE bajar persiana 1");
#endif
      if (runPeriod == 0) {
        tickerMs1.attach_ms(map(position1, 0, 100, 0, (delayDown1 * 1000)), toggleseconds1); // inicializamos timer bajar persiana 1
        StopIsr1 = false;
      }
      else {
        tickerMs1.attach_ms(map(position1, 0, 100, 0, runPeriod), toggleseconds1);    // inicializamos timer bajar persiana 1
        StopIsr1 = false;
      }
      mqttSend (("/switch/" + nameLocate + "1/close_cover"), (char*) "Turning ON", false);   // publicamos estado
      mqttSend (("/switch/" + nameLocate + "1/close_cover"), (char*) "on", false);
      mqttSend (("/cover/" + nameLocate + "1/operation"), (char*) "closing", false);  // cerrando
      bitSet (ReleFalg, 7);                                                           // indicamos relé activado
      bitSet (ReleFalg, 1);                                                           // Indicamos relé bajar persiana 1 activo
      periodRun1 = millis();
      break;
//***********************SUBIR 2******************************************************
    case 3:
      if (digitalRead(Up2Pin) || position2 == setPosition2) {
        mqttSend(("/cover/" + nameLocate + "2/operation"), (char *)"OPENING", false);
        snprintf(payload, 4, "%d", position2);
        mqttSend("/cover/" + nameLocate + "2/position/state", payload, false);
        mqttSend(("/switch/" + nameLocate + "2/open_cover"), digitalRead(Up2Pin) ? (char *) "ON": (char *) "OFF", false);
        return;                                                                       // Si estamos subiendo salimos
      }
      digitalWrite(Down2Pin, LOW);                                                    // Nos aseguramos que el relé de bajar persiana 2 se encuentre parado
      bitClear(ReleFalg, 3);                                                          // Borramos bandera de indicación bajando persiana 2
      digitalWrite(Up2Pin, HIGH);                                                     // Activamos el relé de subir la persiana 2
#if DebugSerial
      debugPrintln("ON RELE subir persiana 2");
#endif
      if (runPeriod == 0) {
        tickerMs2.attach_ms(map(position2, 0, 100, (delayUp2 * 1000), 0), toggleseconds2); // Inicializamos timer subir persiana 2
        StopIsr2 = false;
      }
      else {
        tickerMs2.attach_ms(map(position2, 0, 100, runPeriod, 0), toggleseconds2); // Inicializamos timer subir persiana 2
        StopIsr2 = false;
      }
      mqttSend (("/switch/" + nameLocate + "2/open_cover"), (char*) "Turning ON", false);    // publicamos estado
      mqttSend (("/switch/" + nameLocate + "2/open_cover"), (char*) "on", false);
      mqttSend (("/cover/" + nameLocate + "2/operation"), (char*) "opening", false);
      bitSet (ReleFalg, 7);                                                           // Indicamos relé activado
      bitSet (ReleFalg, 2);                                                           // Indicamos relé subir persiana 2 activo
      periodRun2 = millis();
      break;
//***********************BAJAR 2******************************************************
    case 4:
      if (digitalRead(Down2Pin) || position2 == setPosition2) {
        mqttSend(("/cover/" + nameLocate + "2/operation"), (char *)"closing", false);
        snprintf(payload, 4, "%d", position1);
        mqttSend("/cover/" + nameLocate + "2/position/state", payload, false);
        mqttSend(("/switch/" + nameLocate + "2/open_cover"), digitalRead(Down2Pin) ? (char *)"on" : (char *)"off", false);
        return;
      }
      digitalWrite(Up2Pin, LOW);                                                      // Nos aseguramos que el relé de subir persiana 2 se encuentre parado
      bitClear(ReleFalg, 2);                                                          // Borramos bandera de indicación subiendo persiana 2
      digitalWrite(Down2Pin, HIGH);                                                   // Activamos el relé de bajar la persiana 2
#if DebugSerial
      debugPrintln("ON RELE bajar persiana 2");
#endif
      if (runPeriod == 0) {
        tickerMs2.attach_ms(map(position2, 0, 100, 0, (delayDown2 * 1000)), toggleseconds2); // inicializamos timer bajar persiana 2
        StopIsr2 = false;
      }
      else
      {
        tickerMs2.attach_ms(map(position2, 0, 100, 0, runPeriod), toggleseconds2);    // inicializamos timer bajar persiana 2
        StopIsr2 = false;
      }
      mqttSend (("/switch/" + nameLocate + "2/close_cover"), (char*) "Turning ON", false);   // publicamos estado
      mqttSend (("/switch/" + nameLocate + "2/close_cover"), (char*) "on", false);
      mqttSend (("/cover/" + nameLocate + "2/operation"), (char*) "closing", false);
      bitSet (ReleFalg, 7);                                                           // indicamos relé activado
      bitSet (ReleFalg, 3);                                                           // Indicamos relé bajar persiana 2 activo
      periodRun2 = millis();
      break;
//***********************STOP 1*******************************************************
    case 5:                                                                           // STOP persiana 1
      tickerMs1.detach();
      bitClear (ReleFalg, 7);
      if (digitalRead (Up1Pin)) {                                                     // Subiendo persiana 1?
        digitalWrite(Up1Pin, LOW);                                                    // Paramos relé subir persiana 1
        periodFcI1 = millis() + OnOffI_FC;
	bitClear(ReleFalg, 1);                                                        // Borramos bandera indicación bandera subir persiana 1
        mqttSend (("/switch/" + nameLocate + "1/open_cover"), (char*) "Turning OFF", false);
        mqttSend (("/switch/" + nameLocate + "1/open_cover"), (char*) "off", false);
      }
      if (digitalRead (Down1Pin)) {                                                   // Bajando persiana 1?
        digitalWrite(Down1Pin, LOW);
        periodFcI1 = millis() + OnOffI_FC;
        bitClear(ReleFalg, 0);
        mqttSend (("/switch/" + nameLocate + "1/close_cover"), (char*) "Turning OFF", false);
        mqttSend (("/switch/" + nameLocate + "1/close_cover"), (char*) "off", false);
      }
      mqttSend (("/cover/" + nameLocate + "1/operation"), (char*) "stop", false);
      snprintf(payload, 4, "%d", position1);
      mqttSend ("/cover/" + nameLocate + "1/position/state", payload, false);
      return;
//***********************STOP 2*******************************************************
    case 6:                                                                           // STOP persiana 2
      tickerMs2.detach();
      bitClear (ReleFalg, 7);
      if (digitalRead (Up2Pin)) {                                                     // Subiendo persiana 2?
        digitalWrite(Up2Pin, LOW);                                                    // Paramos relé subir persiana 2
        periodFcI2 = millis() + OnOffI_FC;
        bitClear(ReleFalg, 2);                                                        // Borramos bandera indicación bandera subir persiana 2
        mqttSend (("/switch/" + nameLocate + "2/open_cover"), (char*) "Turning OFF", false);
        mqttSend (("/switch/" + nameLocate + "2/open_cover"), (char*) "off", false);
      }
      if (digitalRead (Down2Pin)) {                                                   // Bajando persiana 2?
        digitalWrite(Down2Pin, LOW);
        periodFcI2 = millis() + OnOffI_FC;
        bitClear(ReleFalg, 3);
        mqttSend (("/switch/" + nameLocate + "2/close_cover"), (char*) "Turning OFF", false);
        mqttSend (("/switch/" + nameLocate + "2/close_cover"), (char*) "off", false);
      }
      mqttSend (("/cover/" + nameLocate + "2/operation"), (char*) "stop", false);
      snprintf(payload, 4, "%d", position2);
      mqttSend ("/cover/" + nameLocate + "2/position/state", payload, false);
      return;
    default:                                                                          // Relés OFF
      tickerMs1.detach();
      tickerMs2.detach();
      OffRelay(3);                                                                    // TODOS los relés a OFF
      bitClear(ReleFalg, 0);
      bitClear(ReleFalg, 1);
      bitClear(ReleFalg, 2);
      bitClear(ReleFalg, 3);
      break;
  }
  RelayStatus();                                                                      // Actualizamos nuevo estado
}

//////////////////////////////////////////////////////////////////////////////////////
// Comprobamos estado de los relés y si hay cambios publicamos el estado
//////////////////////////////////////////////////////////////////////////////////////
void RelayStatus() {
  if (modeHandbook) return;                                                           // Modo manual, salimos
  static uint8_t memPosition1 = 0;
  static uint8_t memPosition2 = 0;
  static int8_t iniPosition1 = -1;
  static int8_t iniPosition2 = -1;
  char payload[4];
  int8_t Position_1, Position_2;
  if (!bitRead (ReleFalg, 7)) return;
  switch (ReleFalg) {
    case 129:                                                                         // 10000001, Bit orden relé y subir persiana 1 activo
      if (iniPosition1 == -1) {                                                       // Sin posición inicial?
        iniPosition1 = position1;
        memPosition1 = position1;
        snprintf(payload, 4, "%d", memPosition1);
        mqttSend ("/cover/" + nameLocate + "1/position/state", payload, false);
        mqttSend (("/cover/" + nameLocate + "1/operation"), (char*) "opening", false);
        mqttRelay ();
        break;
      }
      Position_1 = iniPosition1 + ((millis() - periodRun1) / (delayUp1 * 10));        // Obtenemos el porcentaje de la posición actual
      if (Position_1 > 100)  Position_1 = 100;
      if (Position_1 > memPosition1) {                                                // tenemos cambio en el porcentaje?
        memPosition1 = uint8_t (Position_1);
        if (memPosition1 >= setPosition1) {                                           // FIN, en posición solicitada?
          bitClear(ReleFalg, 7);
          bitClear(ReleFalg, 0);
          memPosition1 = setPosition1;
          snprintf(payload, 4, "%d", memPosition1);
          mqttSend ("/cover/" + nameLocate + "1/position/state", payload, false);
          mqttSend (("/cover/" + nameLocate + "1/operation"), (char*) "open", false);
          iniPosition1 = -1;
          mqttRelay ();
        }
        else if (memPosition1 % 2 == 0) {                                             // Posición par? enviamos de nuevo el estado por MQTT
          snprintf(payload, 4, "%d", memPosition1);
          mqttSend ("/cover/" + nameLocate + "1/position/state", payload, false);
          mqttSend (("/cover/" + nameLocate + "1/operation"), (char*) "opening", false);
        }
        position1 = memPosition1;
      }
    break;
    case 130:                                                                         // 10000010, Bit orden relé y bajar persiana 1 activo
      if (iniPosition1 == -1) {                                                       // Primer ciclo?
        iniPosition1 = position1;
        memPosition1 = position1;
        snprintf(payload, 4, "%d", position1);
        mqttSend ("/cover/" + nameLocate + "1/position/state", payload, false);
        mqttSend (("/cover/" + nameLocate + "1/operation"), (char*) "closing", false);
        mqttRelay ();
        break;
      }
      Position_1 = iniPosition1 - ((millis() - periodRun1) / (delayDown1 * 10));      // Obtenemos el porcentaje
      if (Position_1 < 0) Position_1 = 0;
      if (Position_1 < memPosition1) {                                                // cambio en el porcentaje?
        memPosition1 = uint8_t (Position_1);
        if (memPosition1 <= setPosition1) {                                           // FIN, en posición solicitada bajando
          bitClear(ReleFalg, 7);
          bitClear(ReleFalg, 1);
          memPosition1 = setPosition1;
          snprintf(payload, 4, "%d", memPosition1);
          mqttSend ("/cover/" + nameLocate + "1/position/state", payload, false);
          mqttSend(("/cover/" + nameLocate + "1/operation"), (char *)"closed", false); 
          iniPosition1 = -1;
          mqttRelay ();
        }
        else if (memPosition1 % 2 == 0) {                                             // Posición par?
          snprintf(payload, 4, "%d", memPosition1);
          mqttSend ("/cover/" + nameLocate + "1/position/state", payload, false);
          mqttSend (("/cover/" + nameLocate + "1/operation"), (char*) "closing", false);
        }
        position1 = memPosition1;
      }
      break;
    // Pesiana 2 
    case 132:                                                                         // 10000100, Bit orden relé y subir persiana 2 activo
      if (iniPosition2 == -1) {
        iniPosition2 = position2;
        memPosition2 = position2;
        snprintf(payload, 4, "%d", memPosition2);
        mqttSend("/cover/" + nameLocate + "2/position/state", payload, false);
        mqttSend(("/cover/" + nameLocate + "2/operation"), (char *)"opening", false);
        mqttRelay ();
        break;
      }
      Position_2 = iniPosition2 + ((millis() - periodRun2) / (delayUp2 * 10));        // Obtenemos el porcentaje
      if (Position_2 > 100) Position_2 = 100;
      if (Position_2 > memPosition2) {                                                // cambio en el porcentaje?
        memPosition2 = uint8_t (Position_2);
        if (memPosition2 >= setPosition2) {
          bitClear(ReleFalg, 7);
          bitClear(ReleFalg, 2);
          memPosition2 = setPosition2;
          snprintf(payload, 4, "%d", memPosition2);
          mqttSend("/cover/" + nameLocate + "2/position/state", payload, false);
          mqttSend(("/cover/" + nameLocate + "2/operation"), (char *)"open", false);
          iniPosition2 = -1;
          mqttRelay ();
        }
        else if (memPosition2 % 2 == 0) {
          snprintf(payload, 4, "%d", memPosition2);
          mqttSend("/cover/" + nameLocate + "2/position/state", payload, false);
          mqttSend(("/cover/" + nameLocate + "2/operation"), (char *)"opening", false);
        }
        position2 = memPosition2;
      }
    break;                                                                          
    case 136:                                                                         // 10001000, Bit orden relé y bajar persiana 2 activo
      if (iniPosition2 == -1) {
        iniPosition2 = position2;
        memPosition2 = position2;
        snprintf(payload, 4, "%d", position2);
        mqttSend("/cover/" + nameLocate + "2/position/state", payload, false);
        mqttSend(("/cover/" + nameLocate + "2/operation"), (char *)"closing", false);
        mqttRelay ();
        break;
      }
      Position_2 = iniPosition2 - ((millis() - periodRun2) / (delayDown2 * 10));      // Obtenemos el porcentaje
      if (Position_2 < 0) Position_2 = 0;
      if (Position_2 < memPosition2) {                                                // Cambio en el porcentaje?
        memPosition2 = uint8_t (Position_2);
        if (memPosition2 <= setPosition2) {
          bitClear(ReleFalg, 7);
          bitClear(ReleFalg, 3);
          memPosition2 = setPosition2;
          snprintf(payload, 4, "%d", memPosition2);
          mqttSend("/cover/" + nameLocate + "2/position/state", payload, false);
          mqttSend("/cover/" + nameLocate + "2/operation", (char *)"closed", false);
          iniPosition2 = -1;
          mqttRelay ();
        }
        else if (memPosition2 % 2 == 0) {
          snprintf(payload, 4, "%d", memPosition2);
          mqttSend("/cover/" + nameLocate + "2/position/state", payload, false);
          mqttSend("/cover/" + nameLocate + "2/operation", (char *)"closing", false);
        }
        position2 = memPosition2;
      }
    break;
  } 
#if alexaSuport
  if (bitRead (alexaFalg, 7)) {
    bitClear(alexaFalg, 7);                                                           // Borramos bandera indicación cambio estado relés por Alexa
#if DebugSerial
    debugPrintln(String (F("Orden de Alexa ")) + String (alexaFalg, BIN));
#endif
    if (bitRead (alexaFalg, 0)) {
      bitClear(alexaFalg, 0);
      setPosition1 = 100;
      RelayOnOff (1, 0);                                                              // Subir persiana 1
#if DebugSerial
      debugPrintln("[Alexa] Subiendo persiana 1");
#endif
    }
    else if (bitRead (alexaFalg, 1)) {
      bitClear(alexaFalg, 1);
      setPosition1 = 0;
      RelayOnOff (2, 0);                                                              // Bajar persiana 1
#if DebugSerial
      debugPrintln("[Alexa] Bajando persiana 1");
#endif
    }
    else if (bitRead (alexaFalg, 2)) {
      bitClear(alexaFalg, 3);
      RelayOnOff (5, 0);                                                              // STOP persiana 1
#if DebugSerial
      debugPrintln("[Alexa] STOP persiana 1");
#endif
    }
    if (bitRead (alexaFalg, 3)) {
      bitClear(alexaFalg, 3);
      setPosition2 = 100;
      RelayOnOff (3, 0);                                                              // Subir persiana 2
#if DebugSerial
      debugPrintln("[Alexa] Subiendo persiana 2");
#endif
    }
    else if (bitRead (alexaFalg, 4)) {
      bitClear(alexaFalg, 4);
      setPosition2 = 0;
      RelayOnOff (4, 0);                                                              // Bajar persiana 2
#if DebugSerial
      debugPrintln("[Alexa] Bajando persiana 2");
#endif
    }
    else if (bitRead (alexaFalg, 5)) {
      bitClear(alexaFalg, 5);
      RelayOnOff (6, 0);                                                              // STOP persiana 2
#if DebugSerial
      debugPrintln("[Alexa] STOP persiana 2");
#endif
    }
  }
#endif
}
//////////////////////////////////////////////////////////////////////////////////////
// Enviamos los cambios de estado de los relés
//////////////////////////////////////////////////////////////////////////////////////
void mqttRelay () {
  String topicRelay;
  uint8_t id = 1;
  uint8_t change = ReleFalg;
  bool Status;
  do {
    switch (id) {
      case 1:
        Status = digitalRead(Up1Pin);
        bitWrite(ReleFalg, 0, Status);
        topicRelay = "/relay/0";
        break;
      case 2:
        Status = digitalRead(Down1Pin);
        bitWrite(ReleFalg, 1, Status);
        topicRelay = "/relay/1";
        break;
      case 3:
        Status = digitalRead(Up2Pin);
        bitWrite(ReleFalg, 2, Status);
        topicRelay = "/relay/2";
        break;
      case 4:
        Status = digitalRead(Down2Pin);
        bitWrite(ReleFalg, 3, Status);
        topicRelay = "/relay/3";
        break;
    }
    if (change != ReleFalg) {                                                         // Tenemos cambios en el estado de los relés?
      char payload[3];
      snprintf(payload, 3, "%d", Status );
      mqttSend((char*) topicRelay.c_str(), payload, false);                           // Publicamos nuevo estado de relés
#if alexaSuport
      if (id == 1) alexa.setState((char*) nameDeviceAlexaUp1.c_str(), Status ? 255 : false, 0);   // Indicamos a alexa el nuevo estado
      if (id == 2) alexa.setState((char*) nameDeviceAlexadown1.c_str(), Status ? 255 : false, 0); // Indicamos a alexa el nuevo estado
      if (id == 3) alexa.setState((char*) nameDeviceAlexaUp2.c_str(), Status ?  255 : false, 0);  // Indicamos a alexa el nuevo estado
      if (id == 4) alexa.setState((char*) nameDeviceAlexadown2.c_str(), Status ?  255 : false, 0);// Indicamos a alexa el nuevo estado
#endif
      change = ReleFalg;
#if DebugSerial
      debugPrintln(String (F("RELAY: ")) + String (id) + " a pasado a " + payload);
#endif
    }
  } while (id++ < (4));
}

//////////////////////////////////////////////////////////////////////////////////////
// Obtenemos el estado de los finales de carrera
// 1-> final carrera de persiana 1 arriba activado
// 2-> final carrera de persiana 1 abajo activado
// 3-> final carrera de persiana 2 arriba activado
// 4-> final carrera de persiana 2 abajo activado
// 10-> detector de intensidad activo para motor 1 subiendo en automático
// 11-> detector de intensidad activo para motor 1 bajando en automático
// 30-> detector de intensidad NO activo para motor 1 manual
// 20-> detector de intensidad activo para motor 2 subiendo automático
// 21-> detector de intensidad activo para motor 2 bajando automático
// 40-> detector de intensidad NO activo para motor 2 manual
// 50-> detector de intensidad activo para motor 1 con apagado por ISR
// 60-> detector de intensidad activo para motor 2 con apagado por ISR
// 70-> Comando STOP desde mqtt
// 0--> desconocido
//////////////////////////////////////////////////////////////////////////////////////
uint8_t LimitStatus () {
  if (modeLimit == 2) {                                                               // Finales de carrera por intensidad?
    if (millis() < periodFcI1 || millis() < periodFcI2) return 0;
    periodFcI1 = 0;
    periodFcI2 = 0;
    if (digitalRead(FC_1)) {                                                          // Solo tenemos lectura si tenemos consumo en el motor
      if (digitalRead(Up1Pin)) return 10;                                             // Automático subiendo motor 1
      if (digitalRead(Down1Pin)) return 11;                                           // Automático bajando motor 1
      if (activeBlind2) {
        if (digitalRead(Up2Pin)) return 20;                                             // Automático subiendo motor 2
        if (digitalRead(Down2Pin)) return 21;                                           // Automático bajando motor 2
      }
      if (digitalRead(FC_1)) {                                                        // Final carrera por intensidad activado?
        if (StopIsr1) {
          OffRelay(0);
          StopIsr1 = false;
          return 50;
        }
        if (CommandSTOP1) {
          CommandSTOP1 = false;
          return 70;
        }
        if (iniRunFc1){
          iniRunFc1 = false;                                                          // Después de un control manual se tiene que inicializar FC
          return 30;                                                                  // Manual motor 1
        }
      }
    }
    if (FC_1 != FC_2) {
      if (digitalRead(FC_2) && activeBlind2) {
        if (digitalRead(Up2Pin)) return 20;                                           // Automático subiendo motor 2
        if (digitalRead(Down2Pin)) return 21;                                         // Automático bajando motor 2
        if (digitalRead(FC_2)) {                                                      // Final carrera por intensidad activado?
          if (StopIsr2) {
            OffRelay(0);
            StopIsr2 = false;
            return 60;
          }
          if (CommandSTOP2) {
            CommandSTOP2 = false;
            return 70;
          }
          if (iniRunFc2) {
            iniRunFc2 = false;                                                        // Después de un control manual se tiene que inicializar FC
            return 40;                                                                // Manual motor 2
          }
        }
      }
    }
  }
  else if (modeLimit == 1) {                                                          // Finales de carrera físicos de subir o de bajar persiana
    if (digitalRead(FC_1) && activeBlind1) {                                          // F.C persiana 1? (SOLO señal activa si Up1Pin o Down1Pin a 1)
      if (digitalRead(Up1Pin)) return 1;
      if (digitalRead(Down1Pin)) return 2;
    }
    if (digitalRead(FC_2) && activeBlind2) {                                          // F.C persiana 2? (SOLO señal activa si Up2Pin o Down2Pin a 1)
      if (digitalRead(Up2Pin)) return 3;
      if (digitalRead(Down2Pin)) return 4;
    }
  }
  if (bitRead (runBlind, 4)) {                                                        // Anteriormente teníamos persiana 1 en movimiento manual?
    bitClear (runBlind, 4);
    uint16_t periodeRun1 = millis() - fcRun1;
    uint8_t porcentajeRun1 = (periodeRun1 / 10) / delayUp1;
    bool unknown1 = true;
    if (porcentajeRun1 < 0) porcentajeRun1 = 0;
    if (porcentajeRun1 > 100) porcentajeRun1 = 100;
    if (periodeRun1 > 250) {                                                          // Filtramos periodo mínimo
#if DebugSerial
      debugPrintln(String(F("-STOP manual- Moviendo persiana 1 durante ")) + String (periodeRun1)  + " ms");
#endif
      if (position1 == 100) {                                                         // Si se encontraba totalmente abierta
        position1 = position1 - porcentajeRun1;                                       // Obtenemos porcentaje cerrado
        unknown1 = false;
#if DebugSerial
        debugPrintln(String(F("Cerrado del 100 al ")) + String (position1)  + " %");
#endif
      }
      else if (position1 == 0) {                                                      // Si se encontraba totalmente cerrada
        position1 = porcentajeRun1;                                                   // Obtenemos porcentaje abierto
        unknown1 = false;
#if DebugSerial
        debugPrintln(String(F("Abierto del 0 al ")) + String (position1)  + " %");
#endif
      }
      else if (position1 < 50 && porcentajeRun1 > position1) {                        // Menos del 50 % y bajado
        position1 = position1 + porcentajeRun1;
        if (position1 > 100) position1 = 100;
        unknown1 = false;
#if DebugSerial
        debugPrintln(String(F("Abierto al ")) + String (position1)  + " %");
#endif
      }
      else  if (position1 > 50 && porcentajeRun1 > (100 - position1)) {               // Mas del 50% y subido
        uint8_t x = position1 - porcentajeRun1;
        x < 0 ? position1 = 0 : position1 = x;
        unknown1 = false;
#if DebugSerial
        debugPrintln(String(F("Cerrado al ")) + String (position1)  + " %");
#endif
      }
    }
    if (unknown1) {
      position1 = 50;                                                                 // Posición desconocida
      iniRunFc1 = false;
#if DebugSerial
      debugPrintln("Posición persiana 1 desconocida");
#endif
    }
    publicMqtt();
  }
  if (bitRead (runBlind, 5)) {                                                        // Anteriormente teníamos persiana 2 en movimiento manual?
    bitClear (runBlind, 5);
    uint16_t periodeRun2 = millis() - fcRun2;
    uint8_t porcentajeRun2 = (periodeRun2 / 10) / delayUp2;
    bool unknown2 = true;
    if (porcentajeRun2 < 0) porcentajeRun2 = 0;
    if (porcentajeRun2 > 100) porcentajeRun2 = 100;
    if (periodeRun2 > 250) {                                                          // Filtramos periodo mínimo
#if DebugSerial
      debugPrintln(String(F("-STOP manual- Moviendo persiana 2 durante ")) + String (periodeRun2 )  + " ms");
#endif
      if (position2 == 100) {                                                         // Si se encontraba totalmente abierta
        position2 = position2 - porcentajeRun2;
        unknown2 = false;
#if DebugSerial
        debugPrintln(String(F("Cerrado del 100 al ")) + String (position2)  + " %");
#endif
      }
      else if (position2 == 0) {                                                      // Si se encontraba totalmente cerrada
        position2 = porcentajeRun2;
        unknown2 = false;
#if DebugSerial
        debugPrintln(String(F("Abierto del 0 al ")) + String (position2)  + " %");
#endif
      }
      else if (position2 < 50 && porcentajeRun2 > position2){                         // Si se encontraba en una posición intermedia o menos del 50 % y bajado
        position2 = position2 + porcentajeRun2;
        if (position2 > 100) position2 = 100;
        unknown2 = false;
#if DebugSerial
          debugPrintln(String(F("Abierto al ")) + String (position2)  + " %");
#endif
      }
      else if (position2 > 50 && porcentajeRun2 > (100 - position2)) {                   // Mas del 50% y subido
          uint8_t x = position2 - porcentajeRun2;
          x < 0 ? position2 = 0 : position2 = x;
          unknown2 = false;
#if DebugSerial
          debugPrintln(String(F("Cerrado al ")) + String (position2)  + " %");
#endif
      }
    }
    if (unknown2) {
      position2 = 50;                                                                 // Posición desconocida
      iniRunFc2 = false;
#if DebugSerial
      debugPrintln("Posición persiana 2 desconocida");
#endif
    }
    publicMqtt();
  }
  return 0;                                                                           // Desconocido/no usamos
}

//////////////////////////////////////////////////////////////////////////////////////
// Gestionamos la activación de los limites
// 1-> final carrera de persiana 1 arriba activado
// 2-> final carrera de persiana 1 abajo activado
// 3-> final carrera de persiana 2 arriba activado
// 4-> final carrera de persiana 2 abajo activado
// 10-> detector de intensidad activo para motor 1 subiendo en automático
// 11-> detector de intensidad activo para motor 1 bajando en automático
// 30-> detector de intensidad NO activo para motor 1 manual
// 20-> detector de intensidad activo para motor 2 subiendo automático
// 21-> detector de intensidad activo para motor 2 bajando automático
// 40-> detector de intensidad NO activo para motor 2 manual
// 50-> detector de intensidad activo para motor 1 con apagado por ISR
// 60-> detector de intensidad activo para motor 2 con apagado por ISR
// 70-> Comando STOP desde mqtt
// 0--> desconocido
//////////////////////////////////////////////////////////////////////////////////////
void LimitsActive(uint8_t status) {
  switch (status) {
    case 1:                                                                           // Activado final carrera de persiana 1 arriba (físicos)
      position1 = 100;
      RelayOnOff(5, 0);
      break;
    case 2:                                                                           // Activado final carrera de persiana 1 abajo (físicos)
      position1 = 0;
      RelayOnOff(5, 0);
      break;
    case 3:                                                                           // Activado final carrera de persiana 2 arriba (físicos)
      position2 = 100;
      RelayOnOff(6, 0);
      break;
    case 4:                                                                           // Activado final carrera de persiana 2 abajo (físicos)
      position2 = 0;
      RelayOnOff(6, 0);
      break;
    case 10:
      if (!bitRead (runBlind, 0)) {
        bitSet (runBlind, 0);                                                         // Indicamos persiana 1 en marcha, subiendo
        fcRun1 = millis();
#if DebugSerial
        if (fcRun1 == millis()) debugPrintln("Persiana 1 subiendo");
#endif
      }
      break;
    case 11:
      if (!bitRead (runBlind, 1)) {
        bitSet (runBlind, 1);                                                         // Indicamos persiana 1 en marcha, bajando
        fcRun1 = millis();
#if DebugSerial
        if (fcRun1 == millis()) debugPrintln("Persiana 1 bajando");
#endif
      }
      break;
    case 20:
      if (!bitRead (runBlind, 2)) {
        bitSet (runBlind, 2);                                                         // Indicamos persiana 2 en marcha, subiendo
        fcRun2 = millis();
#if DebugSerial
        if (fcRun2 == millis())  debugPrintln("Persiana 2 subiendo");
#endif
      }
      break;
    case 21:
      if (!bitRead (runBlind, 3)) {
        bitSet (runBlind, 3);                                                         // Indicamos persiana 2 en marcha, bajando
        fcRun2 = millis();
#if DebugSerial
        if (fcRun2 == millis()) debugPrintln("Persiana 2 bajando");
#endif
      }
      break;
    case 30:
      if (!bitRead (runBlind, 4)) {                                                   // Indicamos persiana 1 en marcha, manual
        bitSet (runBlind, 4);
        fcRun1 = millis();                                                            // Permitimos manual y automatico?
         if (!modeHandbook) Anomalia(true);
#if DebugSerial
        debugPrintln("Persiana 1 control manual");
#endif
      }
      break;
    case 40:
      if (!bitRead (runBlind, 5)) {                                                   // Indicamos persiana 2 en marcha, manual
        bitSet (runBlind, 5);
        fcRun2 = millis();                                                            // Permitimos manual y automático?
        if (!modeHandbook) Anomalia (true);
#if DebugSerial
        debugPrintln("Persiana 2 control manual");
#endif
      }
      break;
    case 50:
      if (!modeHandbook) Anomalia(true);
#if DebugSerial
      debugPrintln("Detector de intensidad activo para motor 1 -ISR OFF-");
#endif
      break;
    case 60:
      if (!modeHandbook) Anomalia(true);
#if DebugSerial
      debugPrintln("Detector de intensidad activo para motor 2 -ISR OFF-");
#endif
      break;
    case 70:

#if DebugSerial
      debugPrintln("Detector de intensidad activo, STOP persiana desde MQTT");
#endif
    break;
  }
}
//////////////////////////////////////////////////////////////////////////////////////
// Intentamos obtener la posición de la/s persiana/s, si no es posible forzamos cerrar
//////////////////////////////////////////////////////////////////////////////////////
uint8_t searchLimits(bool blind) {
  if (modeLimit == 0) return 0;                                                       // Solo es valido si tenemos FC físicos o por sensor corriente
  uint8_t pinUp = Up1Pin;
  uint8_t pinDown = Down1Pin;
  uint8_t FC = FC_1;
  uint16_t limitDelay = delayDown1 * 1000;
  if (blind == 1) {                                                                   // Segunda persiana?
    pinUp = Up2Pin;
    pinDown = Down2Pin;
    FC = FC_2;
    limitDelay = delayDown2 * 1000;
  }
#if DebugSerial
  debugPrintln(String(F("Iniciamos detención posición persiana ")) + String (blind + 1));
#endif
  if (modeLimit == 1) {                                                               // Finales de carrera físicos
    digitalWrite(pinUp, HIGH);                                                        // ON relé subir
#if DebugSerial
    debugPrintln(String(F("Detección: ON RELE subir persiana ")) + String (blind + 1));
#endif
    delay (10);
    if (digitalRead(FC)) {                                                            // F.C activo?
      if (!OffRelay(blind + 1)) return 0;                                             // OFF relé subir
#if DebugSerial
      debugPrintln(String(F("Persiana ")) + String (blind + 1) + " abierta");
#endif
      return blind ? 4 : 2;
    }
    if (!OffRelay(blind + 1)) return 0;
    digitalWrite(pinDown, HIGH);
#if DebugSerial
    debugPrintln(String(F("Detección: ON RELE bajar persiana ")) + String(blind + 1));
#endif
    delay (10);
    if (digitalRead(FC)) {
      if (!OffRelay(blind + 1)) return 0;
#if DebugSerial
      debugPrintln(String(F("Persiana ")) + String (blind + 1) + " cerrada");
#endif
      return blind ? 3 : 1;
    }
    do {                                                                              // Bajamos persiana...
      delay (1);
      limitDelay--;
    } while (digitalRead(FC) && limitDelay != 0);
    if (!OffRelay(blind + 1)) return 0;
#if DebugSerial
    debugPrintln(F("Relé bajar OFF"));
    debugPrintln(String(F("Persiana ")) + String (blind + 1) + " cerrada");
#endif
    return blind ? 3 : 1;
  }
  else {
    if (digitalRead(FC)) {                                                            // Si tenemos consumo en motor, nos aseguramos que se encuentre en reposo
#if DebugSerial
      debugPrintln(String(F("Persiana ")) + String (blind + 1) + " en funcionamiento ->paramos");
#endif
      if (!OffRelay(blind + 1)) return 0;
    }
    if (digitalRead(FC)) {                                                            // Nos aseguramos que nos encontramos en reposo
    if (!modeHandbook) Anomalia(true);
#if DebugSerial
      debugPrintln(String(F("Error: Persiana ")) + String (blind + 1) + " en funcionamiento manualmente");
#endif
      return 0;
    }
    // Iniciamos detección posición
    digitalWrite(pinUp, HIGH);                                                        // ON relé subir
#if DebugSerial
    debugPrintln(F("Relé subir ON"));
#endif
    delay (60);
    if (digitalRead(FC)) {                                                            // Tenemos consumo subiendo?
#if DebugSerial
      debugPrintln(String(F("Persiana ")) + String (blind + 1) + " NO abierta totalmente");
#endif
      if (!OffRelay(blind + 1)) return 0;                                             // OFF relé subir
      digitalWrite(pinDown, HIGH);                                                    // ON relé bajar, comprobamos bajando
#if DebugSerial
      debugPrintln(F("Relé bajar ON"));
#endif
      delay (120);
      if (digitalRead(FC)) {
#if DebugSerial
        debugPrintln(String(F("Persiana ")) + String (blind + 1) + " cerrándose..");
#endif
        do {
          delay (1);
          limitDelay--;
        } while (digitalRead(FC) && limitDelay != 0);
        if (!OffRelay(blind + 1)) return 0;
#if DebugSerial
        debugPrintln(F("Relé bajar OFF"));
        debugPrintln(String(F("Persiana ")) + String (blind + 1) + " cerrada");
#endif
        return blind ? 3 : 1;
      }
      else {
        if (!OffRelay(blind + 1)) return 0;                                           // OFF relé subir
#if DebugSerial
        debugPrintln(F("Relé bajar OFF"));
        debugPrintln(String(F("Persiana ")) + String (blind + 1) + " cerrada");
#endif
        return blind ? 3 : 1;
      }
    }
    else {
      if (!OffRelay(blind + 1)) return 0;                                             // OFF relé subir
#if DebugSerial
      debugPrintln(F("Relé subir OFF"));
      debugPrintln(String(F("Persiana ")) + String (blind + 1) + " abierta");
#endif
      return blind ? 4 : 2;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// Anomalía en el funcionamiento o orden automático/manual
//////////////////////////////////////////////////////////////////////////////////////
void Anomalia (bool OnOff) {
  if (OnOff) {
#if AllowHandbook ==  0
    modeHandbook = true;                                                              // mode->true modo manual habilitado
    writeFile("on", "/state.txt");                                                    // Escribimos modo NO automático
#endif
    if (activeBlind1) {
      digitalWrite(Up1Pin, LOW);
      digitalWrite(Down1Pin, LOW);
    }
    if (activeBlind2) {
      digitalWrite(Up2Pin, LOW);
      digitalWrite(Down2Pin, LOW);
    }
    mqttSend ("/cover/" + nameLocate + "/switch/mode", (char*) "1", false);
    iniRunFc1 = false;                                                                // Indicamos finales de carrera NO iniciados
    iniRunFc2 = false;
    position1 = 50; 
    position2 = 50; 
    mqttSend("/cover/" + nameLocate + "1/position/state", (char *)"50", false);
    if (activeBlind2) mqttSend("/cover/" + nameLocate + "2/position/state", (char *)"50", false);
    mqttSend ("/status", (char*) "anomaly", false);
    error == 0 ? error = 4: error = 100;                                              // Indicamos error por anomalía
#if DebugSerial
    debugPrintln(F("Bloqueo modo automático"));
#endif
  }
  else {
    modeHandbook = false;
#if AllowHandbook == 0
    writeFile("off", "/state.txt");                                                   // Escribimos modo automático
    position1 = 50;
    position2 = 50;
#endif
    char payload[4];
    snprintf(payload, 4, "%d", position1);
    mqttSend("/cover/" + nameLocate + "1/position/state", payload, false);
    if (activeBlind2) {
      snprintf(payload, 4, "%d", position2);
      mqttSend("/cover/" + nameLocate + "2/position/state", payload, false);
    }
    mqttSend ("/cover/" + nameLocate + "/switch/mode", (char*) "0", false);
    mqttSend ("/status", (char*) "online", false);
#if DebugSerial
    debugPrintln(F("Pasamos modo automático"));
#endif
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// Leemos la temperatura y la humedad del sensor DHT11
// err = 0x00  OK
// err = 0x10 Error al esperar la señal baja de inicio
// err = 0x11 Error al esperar la señal alta de inicio
// err = 0x12 Error al esperar la señal de inicio de datos bajo
// err = 0x13 Error al esperar la señal de lectura de datos
// err = 0x14 Error al esperar la señal de datos EOF
// err = 0x15 Error al validar la suma de verificación
// err = 0x16 Error cuando la temperatura y la humedad es cero
// err = 0x17 Error cuando el pin no está inicializado
// err = 0x18 Error cuando el modo de pin no es válido
// err > 0x18 (dos bytes) los 8bits altos son de duración del tiempo y los 8bits bajos
//            son el código de error (eje: 0x5010 periodo de 50us error 0x10)
//////////////////////////////////////////////////////////////////////////////////////
#if DHTsensor
bool ReadDHT() {
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&tempInt, &humedadInt, NULL)) != SimpleDHTErrSuccess) {
#if DebugSerial
    debugPrintln(String(F("[DHT] error: 0x")) + String (err, HEX) + " al leer el sensor");
#endif
    error == 0 ? error = 5: error = 100;                                              // Indicamos el error de la lectura del sensor
    return false;
  }
  // if (tempInt > 15) tempInt = tempInt - 1;                                         // Corrección según calibración
#if DebugSerial
  debugPrintln(String(F("[DHT] Temperatura ")) + String (tempInt));
  debugPrintln(String(F("[DHT] Humedad ")) + String (humedadInt));
#endif
  return true;
}
#endif
//////////////////////////////////////////////////////////////////////////////////////
// Función para grabar un String en la EEPROM (longitud máxima de 50)
//////////////////////////////////////////////////////////////////////////////////////
void txEeprom(int16_t addr, String str) {
  int longitud = str.length();
  char inchar[50];
  str.toCharArray(inchar, longitud + 1);
  for (int i = 0; i < longitud; i++) {
    if (EEPROM.read(addr + i) != inchar[i]) EEPROM.write(addr + i, inchar[i]);
  }
  for (int i = longitud; i < 50; i++) {  												                      // Completamos el resto  con el valor 255
    if (EEPROM.read(addr + i) != 255) EEPROM.write(addr + i, 255);
  }
  EEPROM.commit();
}

//////////////////////////////////////////////////////////////////////////////////////
// Función para leer la EEPROM y obtener un String (longitud máxima de 50)
//////////////////////////////////////////////////////////////////////////////////////
String rxEeprom(int16_t addr) {
  byte rx;
  String str_RX;
  for (int i = addr; i < addr + 50; i++) {
    rx = EEPROM.read(i);
    if (rx != 255) {
      str_RX += (char)rx;
    }
  }
#if DebugSerial
  debugPrintln(String(F("ID en EEPROM: ")) + String(str_RX));
#endif
  return str_RX;
}

#if DebugSerial
//////////////////////////////////////////////////////////////////////////////////////
// Enviamos trama de debugger
//////////////////////////////////////////////////////////////////////////////////////
void debugPrintln(String debugText) {
  yield();
  noInterrupts();
  String debugTimeText = "[+" + String(float(millis()) / 1000, 3) + "s] " + debugText;
  Serial.println(debugTimeText);
  interrupts();
  yield();
  Serial.flush();                                                                     // Esperar hasta que el uart emita la interrupción TXComplete
}
#endif
