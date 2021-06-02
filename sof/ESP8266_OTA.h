/*
 * 
 * 
  Radioelf - Junio 2021
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
*/

#ifndef ESP8266_OTA_H
#define ESP8266_OTAP_H

#include <WiFiUdp.h>
#include <ArduinoOTA.h>

bool InitOTA(String hostname) {
  ArduinoOTA.setPort(8266);                                                           // Puerto por defecto 8266
  ArduinoOTA.setHostname(hostname.c_str());                                           // Nombre para el módulo ESP8266 al descubrirlo por el Ide de Arduino
  ArduinoOTA.setPassword("xxxxxx");                                                 

  ArduinoOTA.onStart([]() {
    digitalWrite(Up1Pin, 0);
    digitalWrite(Down1Pin, 0);
    digitalWrite(Up2Pin, 0);
    digitalWrite(Down2Pin, 0);
  });

  // ArduinoOTA.onEnd([]() {});

  // ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) { });

  ArduinoOTA.onError([](ota_error_t error) {
    return 0;
  });

  ArduinoOTA.begin();
  return 1;
}

#endif
