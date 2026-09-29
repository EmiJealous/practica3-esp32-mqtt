# Que es Node-RED y para que sirve

## Que es

Node-RED es una herramienta de **programacion visual basada en flujos** (flow-based programming), construida sobre Node.js. En vez de escribir codigo linea por linea, arrastras "nodos" a un lienzo en el navegador y los conectas entre si con cables — cada nodo hace una cosa pequeña (recibir un mensaje MQTT, transformar un dato, mostrar un boton, guardar en un archivo, llamar una API, etc.) y el flujo completo es la logica de tu aplicacion.

Fue creado originalmente por IBM (ahora es parte de la OpenJS Foundation) pensando en proyectos de IoT, pero hoy se usa para automatizaciones en general (domotica, integraciones entre APIs, bots, dashboards).

## Para que sirve en este contexto (IoT / MQTT)

Node-RED trae nodos nativos para MQTT (`mqtt in` / `mqtt out`) que se conectan a cualquier broker (en nuestro caso, el Mosquitto local que ya teniamos corriendo para el ESP32). Eso permite:

- **Suscribirse** a topicos y reaccionar a los mensajes sin escribir un cliente MQTT a mano
- **Publicar** comandos hacia el ESP32 con solo conectar un nodo
- Agregar el modulo **Dashboard** (`@flowfuse/node-red-dashboard`), que da nodos de interfaz (botones, sliders, graficas, gauges, texto) que arman automaticamente una pagina web funcional — sin tener que programar HTML/CSS/JS a mano

## Por que lo agregamos a la Practica 3

El PDF de la practica no lo pide, pero hasta ahora la unica forma de mandar comandos al ESP32 era escribiendo texto a mano en MQTTX (`ON`, `COLOR:RED`, `BLINK:2`, etc.), y ver los datos como texto plano en una lista de mensajes. Con Node-RED armamos un **dashboard real**: botones para cada comando, un slider para la frecuencia de parpadeo, y graficas en vivo del potenciometro — mucho mas presentable para la demo en clase, y demuestra un nivel extra de integracion sobre el broker MQTT.

## Como se relaciona con lo demas

```
ESP32 (practica3.ino)  <--WiFi/MQTT-->  Mosquitto (broker local)  <--MQTT-->  Node-RED (dashboard)
```

Node-RED no reemplaza nada de lo que ya teniamos — es **un cliente MQTT mas** conectado al mismo broker, igual que MQTTX o `mosquitto_pub`/`mosquitto_sub`. La diferencia es que en vez de escribir comandos a mano, tiene una interfaz grafica ya armada.

Ver tambien: [[Practica 3 - Node-RED - Instalacion paso a paso]] · [[Practica 3 - Node-RED - Dashboard MQTT (Flujo)]]
