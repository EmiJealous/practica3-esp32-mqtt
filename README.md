# Practica 3 - IoT: ESP32 + WiFi + MQTT + LED RGB

Universidad de Guadalajara — Ingeniería Informática — Internet de las Cosas

Control remoto de un LED RGB desde un ESP32, usando WiFi y MQTT (broker local Eclipse Mosquitto) en vez de Bluetooth (BLE), que era como se hacía en la práctica anterior. Incluye además un dashboard extra hecho en Node-RED para controlar y monitorear todo desde una interfaz visual.

## Estructura del repo

```
practica3/
  practica3.ino       -> sketch principal del ESP32
  secrets.h.example    -> plantilla de credenciales (copiar a secrets.h)
docs/
  Practica 3 - Explicacion completa para exponer en clase.md
  Practica 3 - Marco Teorico.md
  Practica 3 - Node-RED - Indice.md
  Practica 3 - Node-RED - Que es y para que sirve.md
  Practica 3 - Node-RED - Instalacion paso a paso.md
  Practica 3 - Node-RED - Dashboard MQTT (Flujo).md
  practica3_node-red-flow.json   -> flujo de Node-RED, importable directo
  funcionamiento_practica3.txt
```

## Como poner en marcha tu copia (cada integrante del equipo)

### 1. Credenciales (WiFi / MQTT)

```bash
cp practica3/secrets.h.example practica3/secrets.h
```

Edita `practica3/secrets.h` con tus propios datos:

```cpp
const char *WIFI_SSID = "TU_RED_WIFI";
const char *WIFI_PASSWORD = "TU_PASSWORD_WIFI";
const char *MQTT_BROKER_IP = "192.168.1.100"; // IP de la compu con Mosquitto en tu red
const char *MQTT_CLIENT_ID = "ESP32-TuNombre"; // unico, no repetir entre compañeros
const char *MQTT_EQUIPO = "equipo1";
```

⚠️ `secrets.h` está en `.gitignore` — nunca se sube al repo. Cada quien tiene el suyo local.

### 2. Broker MQTT (Eclipse Mosquitto)

Instalar y correr Mosquitto local (Mac, con Homebrew):

```bash
brew install mosquitto
brew services start mosquitto
```

Configuración usada en `/opt/homebrew/etc/mosquitto/mosquitto.conf` (para que el ESP32 lo alcance por WiFi, no solo `localhost`):

```
listener 1883 0.0.0.0
allow_anonymous true
```

### 3. Circuito

| Componente | Pin ESP32 |
|---|---|
| LED RGB - Rojo | GPIO 26 |
| LED RGB - Verde | GPIO 25 |
| LED RGB - Azul | GPIO 33 |
| Botón | GPIO 14 |
| Potenciómetro | GPIO 34 |

LED RGB cátodo común, cada canal con su resistencia de protección (~220-330Ω) antes del pin. Botón con `INPUT_PULLDOWN` (no necesita resistencia externa). Potenciómetro con las patas externas a GND/3.3V y el wiper al GPIO.

### 4. Compilar y subir

Con Arduino IDE (core `esp32` instalado) o `arduino-cli`:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 practica3
arduino-cli upload -p /dev/tu.puerto --fqbn "esp32:esp32:esp32:UploadSpeed=115200" practica3
```

### 5. (Extra) Dashboard en Node-RED

Ver [docs/Practica 3 - Node-RED - Indice.md](docs/Practica%203%20-%20Node-RED%20-%20Indice.md) para instalación completa. Resumen: instalar Node-RED + `@flowfuse/node-red-dashboard`, correr `node-red`, e importar `docs/practica3_node-red-flow.json` desde el editor (Menu ☰ → Import).

## Documentación completa

Toda la explicación conceptual (arquitectura, estructura de tópicos, comandos, marco teórico MQTT/WiFi/Broker/QoS/Retain/LWT, adaptación desde la práctica anterior) está en la carpeta [`docs/`](docs/), pensada para armar el reporte en PDF y para exponer en clase.
