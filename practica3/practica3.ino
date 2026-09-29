/*
 * ============================================================
 * PRACTICA 3 - Internet de las Cosas
 * Control de un LED RGB por WiFi + MQTT (broker Eclipse Mosquitto)
 * ============================================================
 *
 * Adaptacion de la practica 2 (BLE): se reemplaza la comunicacion BLE por
 * WiFi + MQTT, y el LED simple por un LED RGB. La logica de antirrebote del
 * boton y el parpadeo no bloqueante con millis() se reutiliza casi igual,
 * solo que ahora en vez de notificar por BLE se publica en topicos MQTT.
 *
 * Estructura de topicos (con <equipo> = MQTT_EQUIPO mas abajo):
 *   iot/<equipo>/light/control  -> (sub) comandos que llegan hacia el ESP32
 *   iot/<equipo>/light/status   -> (pub, retain) estado actual de la luz
 *   iot/<equipo>/device/status  -> (pub, retain) ONLINE / OFFLINE (LWT)
 *   iot/<equipo>/button/event   -> (pub) eventos del boton
 *   iot/<equipo>/pot/value      -> (pub) lectura del potenciometro cada 1s
 */

#include <WiFi.h>         // Conexion WiFi del ESP32
#include <PubSubClient.h> // Cliente MQTT (publish/subscribe) sobre esa conexion WiFi

// ============================================================
// CONFIGURACION
// ============================================================
// WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER_IP, MQTT_CLIENT_ID y MQTT_EQUIPO
// viven en secrets.h (NO se sube al repo). Copia secrets.h.example -> secrets.h
// y pon ahi tus propios datos antes de compilar.
#include "secrets.h"

const int MQTT_BROKER_PORT = 1883;

// ============================================================
// PINES
// ============================================================
// LED RGB: cada canal va a una resistencia de proteccion antes del pin del LED
const int RED_PIN = 26;
const int GREEN_PIN = 25;
const int BLUE_PIN = 33;
const int BUTTON_PIN = 14;
const int POT_PIN = 34; // ADC1: se usa a proposito, los pines ADC2 no funcionan bien con WiFi activo

// ============================================================
// CLIENTES DE RED
// ============================================================
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Topicos ya armados con el nombre del equipo (se llenan en construirTopicos())
char topicLightControl[48];
char topicLightStatus[48];
char topicDeviceStatus[48];
char topicButtonEvent[48];
char topicPotValue[48];

// ============================================================
// ESTADO DE LA ILUMINACION (equivalente al Estado A de la practica 2)
// ============================================================
enum Color
{
  ROJO,
  VERDE,
  AZUL
};
Color colorActual = ROJO;

bool sistemaHabilitado = true;  // true/false -> comandos ON / OFF
bool frecuenciaForzada = false; // true = frecuencia fija por BLINK:x, false = AUTO (usa el pot)
float hzForzadoValor = 1.0;

bool ledEncendido = false;
unsigned long ultimoCambioLED = 0;

String modoAnterior = "ON";
unsigned long ultimoCambioModo = 0;

// ============================================================
// ANTIRREBOTE DEL BOTON (igual que en la practica 2)
// ============================================================
int ultimaLecturaCruda = LOW;
int estadoBotonEstable = LOW;
unsigned long ultimoCambioBoton = 0;
const unsigned long DEBOUNCE_MS = 50;

// ============================================================
// EVENTOS DEL BOTON (contador + doble clic)
// ============================================================
unsigned long contadorPresiones = 0;
unsigned long ultimaPresionMs = 0;
const unsigned long DOBLE_CLICK_MS = 400; // ventana para considerar dos clics como "doble clic"

// ============================================================
// TELEMETRIA DEL POTENCIOMETRO
// ============================================================
unsigned long ultimaLecturaPot = 0;
const unsigned long POT_INTERVALO_MS = 1000; // cada 1 segundo, como pide la practica

// Arma los 5 topicos MQTT insertando el nombre del equipo, para no repetir
// el string "iot/equipo1/..." a mano en cada publish/subscribe
void construirTopicos()
{
  snprintf(topicLightControl, sizeof(topicLightControl), "iot/%s/light/control", MQTT_EQUIPO);
  snprintf(topicLightStatus, sizeof(topicLightStatus), "iot/%s/light/status", MQTT_EQUIPO);
  snprintf(topicDeviceStatus, sizeof(topicDeviceStatus), "iot/%s/device/status", MQTT_EQUIPO);
  snprintf(topicButtonEvent, sizeof(topicButtonEvent), "iot/%s/button/event", MQTT_EQUIPO);
  snprintf(topicPotValue, sizeof(topicPotValue), "iot/%s/pot/value", MQTT_EQUIPO);
}

// Convierte el enum Color a texto, para mandarlo en los mensajes MQTT
const char *colorATexto(Color c)
{
  switch (c)
  {
  case ROJO:
    return "RED";
  case VERDE:
    return "GREEN";
  case AZUL:
    return "BLUE";
  }
  return "RED";
}

// Igual que obtenerModoActual() de la practica 2: resume en texto si el
// sistema esta apagado, parpadeando a frecuencia forzada, o en automatico
String obtenerModoActual()
{
  if (!sistemaHabilitado)
    return "OFF";
  if (frecuenciaForzada)
    return "BLINK";
  return "ON"; // encendido, parpadeando segun el potenciometro (control local)
}

// Prende o apaga solo el pin del color seleccionado, y deja los otros dos en LOW
void aplicarColor(bool encendido)
{
  digitalWrite(RED_PIN, (encendido && colorActual == ROJO) ? HIGH : LOW);
  digitalWrite(GREEN_PIN, (encendido && colorActual == VERDE) ? HIGH : LOW);
  digitalWrite(BLUE_PIN, (encendido && colorActual == AZUL) ? HIGH : LOW);
}

/*
 * CONTROL DE LA ILUMINACION (equivalente a manejarParpadeoEstadoA de la practica 2)
 * Si el sistema esta apagado, mantiene el LED apagado.
 * Si esta prendido, parpadea al color actual: a la frecuencia forzada por
 * BLINK:x si aplica, o si no, a la velocidad que marque el potenciometro
 * (esto es justamente el "control local" al que vuelve el comando AUTO).
 */
void manejarIluminacion()
{
  if (!sistemaHabilitado)
  {
    aplicarColor(false);
    ledEncendido = false;
    return;
  }

  int retardoMs;
  if (frecuenciaForzada)
  {
    retardoMs = (int)(1000.0 / hzForzadoValor / 2.0);
  }
  else
  {
    int valorADC = analogRead(POT_PIN);
    retardoMs = map(valorADC, 0, 4095, 1000, 50);
  }

  if (millis() - ultimoCambioLED >= retardoMs)
  {
    ultimoCambioLED = millis();
    ledEncendido = !ledEncendido;
    aplicarColor(ledEncendido);
  }
}

// Publica el estado actual de la luz en light/status, con retain para que
// cualquiera que se suscriba despues siga viendo el ultimo estado conocido
void publicarEstadoLuz()
{
  String modo = obtenerModoActual();
  int valorADC = analogRead(POT_PIN);
  unsigned long tiempoDesdeCambio = (millis() - ultimoCambioModo) / 1000; // segundos

  char buffer[160];
  if (modo == "BLINK")
  {
    snprintf(buffer, sizeof(buffer),
             "{\"mode\":\"%s\",\"color\":\"%s\",\"freq\":%.1f,\"time\":%lu,\"pot\":%d}",
             modo.c_str(), colorATexto(colorActual), hzForzadoValor, tiempoDesdeCambio, valorADC);
  }
  else
  {
    snprintf(buffer, sizeof(buffer),
             "{\"mode\":\"%s\",\"color\":\"%s\",\"time\":%lu,\"pot\":%d}",
             modo.c_str(), colorATexto(colorActual), tiempoDesdeCambio, valorADC);
  }

  mqttClient.publish(topicLightStatus, buffer, true); // true = retain
  Serial.print("light/status -> ");
  Serial.println(buffer);
}

// Si el modo (ON/OFF/BLINK) cambio respecto al anterior, actualiza el
// contador de tiempo y publica el nuevo estado. Se llama despues de procesar
// cualquier comando que pueda haber cambiado algo (ON, OFF, COLOR, BLINK, AUTO)
void actualizarModoYPublicarEstado()
{
  String modoNuevo = obtenerModoActual();
  if (modoNuevo != modoAnterior)
  {
    modoAnterior = modoNuevo;
    ultimoCambioModo = millis();
  }
  publicarEstadoLuz();
}

/*
 * Se dispara cada vez que llega un mensaje MQTT en un topico suscrito
 * (en este caso, solo estamos suscritos a light/control). Es el equivalente
 * al onWrite() de la clase EventCallback en la version BLE.
 */
void alRecibirMensajeMQTT(char *topic, byte *payload, unsigned int length)
{
  String comando;
  for (unsigned int i = 0; i < length; i++)
  {
    comando += (char)payload[i];
  }
  comando.trim();
  comando.toUpperCase();

  if (comando.length() == 0)
    return;

  Serial.print("Comando MQTT: ");
  Serial.println(comando);

  if (comando == "STATUS")
  {
    publicarEstadoLuz(); // solo consulta el estado, no cambia nada
    return;
  }

  if (comando == "ON")
  {
    sistemaHabilitado = true;
  }
  else if (comando == "OFF")
  {
    sistemaHabilitado = false;
  }
  else if (comando == "AUTO")
  {
    frecuenciaForzada = false; // vuelve al control local (frecuencia por potenciometro)
  }
  else if (comando == "COLOR:RED")
  {
    colorActual = ROJO;
  }
  else if (comando == "COLOR:GREEN")
  {
    colorActual = VERDE;
  }
  else if (comando == "COLOR:BLUE")
  {
    colorActual = AZUL;
  }
  else if (comando.startsWith("BLINK:"))
  {
    float hz = comando.substring(6).toFloat();
    if (hz > 0)
    {
      frecuenciaForzada = true;
      hzForzadoValor = hz;
    }
  }
  else
  {
    Serial.println("Comando no reconocido, se ignora.");
    return; // comando invalido: no publicamos status porque nada cambio
  }

  actualizarModoYPublicarEstado();
}

/*
 * LECTURA DEL BOTON (Antirrebote) + EVENTOS MQTT
 * Ademas de filtrar el rebote como en la practica 2, ahora cada clic
 * confirmado se publica en button/event junto con un contador acumulado,
 * y si dos clics caen dentro de la ventana DOBLE_CLICK_MS se publica
 * ademas un evento de doble clic.
 */
void manejarBoton()
{
  int lecturaCruda = digitalRead(BUTTON_PIN);

  if (lecturaCruda != ultimaLecturaCruda)
  {
    ultimoCambioBoton = millis();
  }

  if ((millis() - ultimoCambioBoton) > DEBOUNCE_MS)
  {
    if (lecturaCruda != estadoBotonEstable)
    {
      estadoBotonEstable = lecturaCruda;

      if (estadoBotonEstable == HIGH)
      {
        contadorPresiones++;

        char buffer[30];
        snprintf(buffer, sizeof(buffer), "BUTTON_PRESSED: %lu", contadorPresiones);
        mqttClient.publish(topicButtonEvent, buffer);

        unsigned long ahora = millis();
        if (ahora - ultimaPresionMs <= DOBLE_CLICK_MS)
        {
          mqttClient.publish(topicButtonEvent, "BUTTON_DOUBLE_CLICK");
        }
        ultimaPresionMs = ahora;
      }
    }
  }
  ultimaLecturaCruda = lecturaCruda;
}

// Publica la lectura del potenciometro normalizada de 0 a 100%, una vez por segundo
void publicarPot()
{
  if (millis() - ultimaLecturaPot < POT_INTERVALO_MS)
    return;
  ultimaLecturaPot = millis();

  int valorADC = analogRead(POT_PIN);
  float porcentaje = (valorADC / 4095.0) * 100.0;

  char buffer[10];
  snprintf(buffer, sizeof(buffer), "%.1f", porcentaje);
  mqttClient.publish(topicPotValue, buffer);
}

// Conecta el ESP32 a la red WiFi y se queda esperando hasta lograrlo
void conectarWiFi()
{
  Serial.printf("Conectando a WiFi '%s'", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println(" conectado.");
  Serial.print("IP asignada: ");
  Serial.println(WiFi.localIP());
}

/*
 * Conecta (o reconecta) al broker MQTT. Configura el Last Will and Testament
 * (LWT): si el ESP32 se cae sin avisar, el broker publica "OFFLINE" en
 * device/status por nosotros. Si la conexion es exitosa, publicamos
 * "ONLINE" a mano y nos suscribimos a light/control para recibir comandos.
 */
void reconectarMQTT()
{
  while (!mqttClient.connected())
  {
    Serial.print("Conectando a MQTT...");

    bool conectado = mqttClient.connect(
        MQTT_CLIENT_ID,
        topicDeviceStatus, 0, true, "OFFLINE" // willTopic, willQos, willRetain, willMessage
    );

    if (conectado)
    {
      Serial.println(" conectado.");
      mqttClient.publish(topicDeviceStatus, "ONLINE", true); // retain
      mqttClient.subscribe(topicLightControl);
    }
    else
    {
      Serial.printf(" fallo (rc=%d), reintentando en 2s...\n", mqttClient.state());
      delay(2000);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);

  construirTopicos();
  conectarWiFi();

  mqttClient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
  mqttClient.setCallback(alRecibirMensajeMQTT);
  reconectarMQTT();

  modoAnterior = obtenerModoActual();
  ultimoCambioModo = millis();
  publicarEstadoLuz(); // estado inicial, para que quede retenido desde el arranque

  Serial.println("Sistema listo.");
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    conectarWiFi();
  }
  if (!mqttClient.connected())
  {
    reconectarMQTT();
  }
  mqttClient.loop(); // procesa mensajes entrantes y mantiene viva la conexion MQTT

  manejarBoton();
  manejarIluminacion();
  publicarPot();
}
