/*
  Pagina web para el modo Punto de Acceso


    Radioelf - Junio 2020
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
#ifndef HTTPAP_H
#define HTTPAP_H

// Nombre del punto de acceso si falla la conexión a la red WiFi
#if NameLocation == 1
#define nameAp "Taller_persiana"                                                      // Configuración taller
#elif NameLocation == 2
#define nameAp "Comedor_persiana"                                                     // Configuración comedor
#elif NameLocation == 3
#define nameAp "Cocina_persiana"                                                      // Configuración cocina
#elif NameLocation == 4
#define nameAp "Aseo_persiana"                                                        // Configuración baño
#elif NameLocation == 5
#define nameAp "Cuartillo_persiana"                                                   // Configuración cuartillo
#elif NameLocation == 6
#define nameAp "Habitacion_persiana"                                                  // Configuración habitación
#elif NameLocation == 7
#define nameAp "Pasillo_persiana"                                                     // Configuración pasillo
#else
#define nameAp "Persiana"
#endif
                                                     
#define passAp "xxxxxx"

bool APmode = false;
WiFiServer serverAP(80);                                                              // Definimos el puerto de comunicaciones

//************************************************************************************
// Activamos y configuramos el modo AP
//************************************************************************************
void modeAP() {
  serverAP.begin();                                                                   // inicializamos el servidor
  WiFi.mode(WIFI_AP);
  // Canal RF 1, ISSD ON, 1 conexión
  while (!WiFi.softAP(nameAp, passAp, 1, 0, 1)) {
    delay(100);
  }
  APmode = true;                                                                      // Este ESP será un Punto de Acceso
}
//************************************************************************************
// Busca si en la listar las redes disponibles, se encuentra la indicada.
// Parámetros scanNetworks:
// async ->true escaneo asíncrono, false escaneo en segundo plano y finalización función
// show_hidden->true todas las redes, false-> NO redes ocultas
// channel -> el canal indicado (0 para todos los canales)
// ssid -> escanea solo el ssid indicado (NULL para escanear todos)
//************************************************************************************
bool listNetworks(const char* ssidOK) {
  uint8_t numSsid = WiFi.scanNetworks(false, false, 0, (uint8_t *) ssidOK);           // Buscar red , NO ocultas
  WiFi.scanDelete();
  if (numSsid <= 0) return false;
  return true;
}
//************************************************************************************
// Gestión de conexión cliente y pagina web
//************************************************************************************
uint16_t connectAP() {
  // Comprueba si el cliente ha conectado
  WiFiClient clientAP = serverAP.available();
  if (!clientAP) {
    updateWeb = 0;
    return webFalg;                                                                   // Sin cliente..
  }
  while (!clientAP.available()) {                                                     // Espera hasta que el cliente envía alguna petición
    delay(1);
  }
  // Lee la petición
  String peticion = clientAP.readStringUntil('\r');
  clientAP.flush();                                                                   // Limpiamos buffer

  // Comprueba la petición
  if (peticion.indexOf("/CONTROL") != -1) {
    updateWeb = 1;
    if (peticion.indexOf("/CONTROL=1") != -1) {
      bitSet(webFalg, 0);
    }
    else if (peticion.indexOf("/CONTROL=2") != -1) {
      bitSet(webFalg, 1);
    }
    else if (peticion.indexOf("/CONTROL=3") != -1) {
      bitSet(webFalg, 2);
    }
    else if (peticion.indexOf("/CONTROL=4") != -1) {
      bitSet(webFalg, 3);
    }
    else if (peticion.indexOf("/CONTROL=5") != -1) {
      bitSet(webFalg, 4);
    }
    else if (peticion.indexOf("/CONTROL=6") != -1) {
      WiFi.softAPdisconnect(true);
      digitalWrite(Up1Pin, 0);
      digitalWrite(Down1Pin, 0);
      digitalWrite(Up2Pin, 0);
      digitalWrite(Down2Pin, 0);
      ESP.restart();
    }
  }
  else if (peticion.indexOf("config?") != -1) {
    updateWeb = 2;
    if (peticion.indexOf("enable1=1") != -1) {                                        // Persiana 1 activa?
      bitSet(webFalg, 5);
    }
    if (peticion.indexOf("enable1=0") != -1) {                                        // Persiana 1 NO activa?
      bitClear(webFalg, 5);
    }
    int indice = peticion.indexOf("t_up1=");
    int longitud  = peticion.indexOf("&", indice);
    String valor = peticion.substring (indice + 6, longitud);
    if (indice != -1) delayUp1 = valor.toInt();                                       // Pasamos el valor del tiempo de subida para persiana 1
    indice = peticion.indexOf("t_dowd1=");
    longitud  = peticion.indexOf( "&", indice);
    valor = peticion.substring (indice + 8, longitud);
    if (indice != -1) delayDown1 = valor.toInt();                                     // Pasamos el valor del tiempo de bajada para persiana 1
    if (peticion.indexOf("enable2=1") != -1) {                                        // Persiana 2 activa?
      bitSet(webFalg, 6);
    }
    if (peticion.indexOf("enable2=0") != -1) {                                        // Persiana 2 NO activa?
      bitClear(webFalg, 6);
    }
    indice = peticion.indexOf("t_up2=");
    longitud  = peticion.indexOf("&", indice);
    valor = peticion.substring (indice + 6, longitud);
    if (indice != -1) delayUp2 = valor.toInt();                                       // Pasamos el valor del tiempo de subida para persiana 2
    indice = peticion.indexOf("t_dowd2=");
    longitud  = peticion.indexOf("&", indice);
    valor = peticion.substring (indice + 8, longitud);
    if (indice != -1) delayDown2 = valor.toInt();                                     // Pasamos el valor del tiempo de bajada para persiana 2
    if (peticion.indexOf("fc=i") != -1) {                                             // Finales de carrear por intensidad?
      bitSet(webFalg, 7);
      FC_2 = FC_1;                                                                    // Limites por consumo solo se usa FC_1
    }
    if (peticion.indexOf("fc=f") != -1) {                                             // Finales de carrera físicos?
      bitClear(webFalg, 8);
      FC_2 = FC2;
    }
    if (peticion.indexOf("fc=n") != -1) {                                             // Sin finales de carrera?
      bitSet(webFalg, 9);
      FC_2 = FC2;
    }
    if (peticion.indexOf("auto=1") != -1) {                                           // Habilitamos soporte alexa
      bitSet(webFalg, 10);
    }
    if (peticion.indexOf("auto=0") != -1) {                                           // Deshabilitamos auto detención limites
      bitClear(webFalg, 10);
    }
    if (peticion.indexOf("alexa=1") != -1) {                                          // Habilitamos auto detección limites
      bitSet(webFalg, 11);
    }
    if (peticion.indexOf("alexa=0") != -1) {                                          // Deshabilitamos soporte alexa
      bitClear(webFalg, 11);
    }
    indice = peticion.indexOf("nameCover=");
    longitud  = peticion.indexOf("&", indice);
    if (indice != -1) {
      if (nameLocate != peticion.substring (indice + 10, longitud)) {
        nameLocate = peticion.substring (indice + 10, longitud);
        nameLocate.toLowerCase();                                                     // Pasamos a minúsculas
        bitSet(webFalg, 12);
      }
    }
  }

  // Envía la página HTML de respuesta al cliente
  clientAP.println("HTTP/1.1 200 OK");
  clientAP.println("Content-Type: text/html");
  clientAP.println("");                                                               // Línea de separación (NECESARIA)
  clientAP.println("<!DOCTYPE HTML>");
  clientAP.println("<meta charset='UTF-8'>");
  clientAP.println("<html>");
  clientAP.println("<head>");
  clientAP.println("<meta http-equiv='refresh' content='60'>");                       // Recarga la pagina cada minuto
  clientAP.println("</head>");
  clientAP.println("<h1>Configuración Persianas</h1>");
  clientAP.println("<form action='config' method='get'>");

  clientAP.println("<label for='name'>Nombre del host (de 4 a 50 characteres):</label>");
  clientAP.print("<input type='text' id='name' name='nameCover' value=");
  clientAP.print (nameLocate);
  clientAP.println(" required minlength='4' maxlength='50' size='10'>");
  //***********persiana 1**************
  clientAP.println("<p>Persiana 1:");
  clientAP.print("<input type ='radio' name ='enable1' value ='1'");
  if (activeBlind1) clientAP.print(" checked");
  clientAP.println("> Activa");
  clientAP.print("<input type ='radio' name ='enable1' value ='0'");
  if (!activeBlind1) clientAP.print(" checked");
  clientAP.println("> Desactivada");

  clientAP.print("<p>Tiempo subir: <input type ='number' name ='t_up1' value =");
  String value = String (delayUp1);
  clientAP.print (value);
  clientAP.println(" size ='1' maxlength ='2' min ='1' max = '65'></p>");

  clientAP.print("<p>Tiempo bajar: <input type ='number' name ='t_dowd1' value =");
  value = String (delayDown1);
  clientAP.print (value);
  clientAP.println(" size ='1' maxlength ='2' min ='1' max ='65'></p>");
  clientAP.println("</p>");
  //***********persiana 2**************
  clientAP.println("<p>Persiana 2:");
  clientAP.println("<input type ='radio' name ='enable2' value ='1'");
  if (activeBlind2) clientAP.print(" checked");
  clientAP.println("> Activa");
  clientAP.print("<input type ='radio' name ='enable2' value ='0'");
  if (!activeBlind2) clientAP.print(" checked");
  clientAP.println("> Desactivada");

  clientAP.print("<p>Tiempo subir: <input type ='number' name ='t_up2' value =");
  value = String (delayUp2);
  clientAP.print (value);
  if (activeBlind2) clientAP.println(" size ='1' maxlength ='2' min ='1' max ='65'> </p>");

  clientAP.print ("<p>Tiempo bajar: <input type ='number' name ='t_dowd2' value =");
  value = String (delayDown2);
  clientAP.print (value);
  if (activeBlind2) clientAP.println(" size ='1' maxlength ='2' min ='1' max ='65'> </p>");
  clientAP.println("</p>");

  clientAP.println("<p>Tipo Finales carrera:");
  clientAP.print("<input type ='radio' name ='fc' value ='f'");
  if (modeLimit == 1) clientAP.print(" checked");
  clientAP.println("> Fisicos");
  clientAP.print("<input type ='radio' title='!Atención! Final de Carrera único para persiana 1 y 2!!' name ='fc' value ='i'");
  if (modeLimit == 2) clientAP.print(" checked");
  clientAP.println("> Consumo (intensidad)");
  clientAP.print("<input type ='radio' name ='fc' value ='n'");
  if (modeLimit == 0) clientAP.print(" checked");
  clientAP.println("> NO usados");

  clientAP.println("<p> Auto-detección posición persiana al inicio:");
  clientAP.print("<input type ='radio' name ='auto' value ='1'");
  if (autoDetec == 1) clientAP.print(" checked");
  clientAP.println("> Habilitado");
  clientAP.print("<input type ='radio' name ='auto' value ='0'");
  if (autoDetec == 0) clientAP.print(" checked");
  clientAP.println("> Deshabilitado");
#if alexaSuport 
  clientAP.println("<p> Soporte Amazon Alexa:");
  clientAP.print("<input type ='radio' name ='alexa' value ='1'");
  if (alexaRun == 1) clientAP.print(" checked");
  clientAP.println("> Habilitado");
  clientAP.print("<input type ='radio' name ='alexa' value ='0'");
  if (alexaRun == 0) clientAP.print(" checked");
  clientAP.println("> Deshabilitado");
#endif
  clientAP.println("</p>");
  clientAP.println("<p>");
  clientAP.println("<input type ='submit' value ='Enviar'>");
  clientAP.println("<input type ='reset' value ='Borrar'>");
  clientAP.println("<input type='button' value='Actualizar cambios' onclick='history.go(-1)'>");
  clientAP.println("</p>");
  clientAP.println("<p>");
  clientAP.println("<button type='button' title='!Atención! se reiniciará el dispositivo!!' onClick=location.href='/CONTROL=6'>RESET hardware</button>");
  clientAP.println("</p>");
  clientAP.println("</form>");
  //<!-- Trama respuesta valores por defecto: config?nameCover=taller&enable1=1&t_up1=10&t_dowd1=10&enable2=0&t_up2=10&t_dowd2=10&fc=i&auto=1&alexa=0-->
  if (statusFC != 0) {
    if (activeBlind1) {
      clientAP.print("<h1>Persiana 1</h1>");
      if (bitRead (statusFC, 1)) {                                                    // Botón subir persiana 1
        clientAP.print("<button type= 'button' onClick=location.href='/CONTROL=1' style='margin:auto; background-color:red; color:#F6D8CE; padding:10px; border:10px solid black; width:300px; height:150px;'><h2>SUBIR</h2></button><br><br></h1>");
      }
      if (bitRead (statusFC, 2)) {                                                    // Botón bajar persiana 1
        clientAP.print("<button type= 'button' onClick=location.href='/CONTROL=2' style='margin:auto; background-color:green; color:#A9F5A9; padding:10px; border:10px solid black; width:300px; height:150px;'><h2>BAJAR</h2></button>");
      }
    }
    if (activeBlind2) {
      clientAP.print("<h1>Persiana 2</h1>");
      if (bitRead (statusFC, 3)) {                                                    // Botón subir persiana 2
        clientAP.print("<button type= 'button' onClick=location.href='/CONTROL=3' style='margin:auto; background-color:red; color:#F6D8CE; padding:10px; border:10px solid black; width:300px; height:150px;'><h2>SUBIR</h2></button><br><br></h1>");
      }
      if (bitRead (statusFC, 4)) {                                                    // Botón bajar persiana 2
        clientAP.println("<button type='button' onClick=location.href='/CONTROL=4' style='margin:auto; background-color:green; color:#A9F5A9; padding:10px; border:10px solid black; width:300px; height:150px;'><h2>BAJAR</h2></button>");
      }
    }
  }
  else if (autoDetec) {
    clientAP.println("<h1>Intentar detectar posición persianas</h1>");                // Botón buscar posición persiana
    clientAP.println("<button type='button' onClick=location.href='/CONTROL=5' style='margin:auto; background-color:black; color:#F6D8CE; padding:10px; border:10px solid black; width:300px; height:150px;'><h2>DETECTAR</h2></button><br></h1>");
  }
  
  clientAP.println("</p>");
  clientAP.println("</html>");
  delay(1);
  //Desconexión de los clientes
  //WiFi.softAPdisconnect();
  return webFalg;
}

#endif
