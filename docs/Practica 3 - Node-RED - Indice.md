# Practica 3 - Node-RED (Extra)

Documentacion del extra que agregamos a la Practica 3 (ESP32 + WiFi + MQTT + LED RGB): un dashboard visual hecho en Node-RED para controlar y monitorear el ESP32 sin tener que escribir comandos a mano en MQTTX.

> Para exponer la practica completa en clase (no solo el extra de Node-RED), ver [[Practica 3 - Explicacion completa para exponer en clase]] — tiene todo junto: arquitectura, topicos, comandos, boton, LWT, adaptacion desde la practica anterior, y este dashboard.

## Notas de esta carpeta

1. [[Practica 3 - Node-RED - Que es y para que sirve]]
2. [[Practica 3 - Node-RED - Instalacion paso a paso]]
3. [[Practica 3 - Node-RED - Dashboard MQTT (Flujo)]]

## Resumen rapido

- **Node-RED** corre en `http://127.0.0.1:1880/` (editor de flujos)
- **Dashboard** (la interfaz visual con botones/graficas) corre en `http://127.0.0.1:1880/dashboard`
- Se conecta al mismo broker **Mosquitto** que ya estaba corriendo local para el ESP32 (`localhost:1883`)
- El flujo completo controla el ESP32 mandando los mismos comandos de texto que ya probamos a mano por MQTTX (`ON`, `OFF`, `COLOR:RED`, etc.) al topico `iot/equipo1/light/control`, y muestra en vivo `pot/value`, `light/status`, `button/event` y `device/status`

## Como levantar todo (checklist para la presentacion)

1. `brew services start mosquitto` (si no esta corriendo ya, revisar con `brew services list`)
2. En una terminal: `node-red` (deja la ventana abierta, o correrlo en background)
3. Abrir `http://127.0.0.1:1880/dashboard` en el navegador
4. Encender el ESP32 (conectado al hotspot / WiFi configurado en el sketch)
5. Confirmar que en el dashboard aparece `device/status: ONLINE`
