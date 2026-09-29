# Practica 3 - Explicacion completa para exponer en clase

---

## 1. Que hace el sistema, en una frase

Tengo un ESP32 que controla un LED RGB (prender/apagar, elegir color, hacerlo parpadear a una frecuencia) y que se puede controlar y monitorear a distancia desde cualquier dispositivo conectado a la misma red, usando el protocolo **MQTT** en vez de Bluetooth (que era como lo hacia en la practica anterior).

## 2. La arquitectura general (como se conecta todo)

No hay una conexion directa entre "el que manda el comando" y el ESP32. Todo pasa por un intermediario central llamado **broker**, que en mi caso es **Eclipse Mosquitto** corriendo local en mi compu.

```
[Celular / Compu / Node-RED]  <--MQTT-->  [Broker Mosquitto]  <--MQTT-->  [ESP32]
```

El ESP32 se conecta a esta red por WiFi, y de ahi abre una conexion MQTT hacia el broker. Todo lo que "habla" (publica mensajes) o "escucha" (se suscribe a mensajes) lo hace a traves de esos canales llamados **topicos**.

## 3. Por que la estructura de topicos que use

Cada topico empieza con `iot/<mi_equipo>/` para que, si varios equipos comparten el mismo broker en clase, los mensajes no se mezclen entre si. Despues de eso, separo por "area funcional":

- `light/control` -> por aqui **entran** los comandos hacia el ESP32 (es el unico topico al que el ESP32 esta suscrito)
- `light/status` -> por aqui el ESP32 **avisa** su estado cada vez que algo cambia
- `device/status` -> si el ESP32 esta ONLINE u OFFLINE
- `button/event` -> eventos del boton fisico
- `pot/value` -> lectura del potenciometro, cada segundo

La separacion entre `control` y `status` es a proposito: uno es para **mandar** ordenes, el otro para **recibir** confirmaciones/telemetria. Asi cualquier cliente puede suscribirse solo a lo que le interesa sin tener que filtrar mensajes mezclados.

## 4. El circuito

- **LED RGB**: tres pines de salida digital, uno por color (rojo, verde, azul), cada uno con su resistencia de proteccion. Solo prendo el pin del color que este activo en ese momento.
- **Boton**: un pin de entrada con pull-down interno (en reposo lee LOW, al presionar sube a HIGH).
- **Potenciometro**: un pin de entrada analogica (ADC), a proposito uno de los que siguen funcionando bien con el WiFi prendido (en el ESP32 hay pines ADC que se desactivan cuando usas WiFi).

## 5. Como se controla el LED remotamente

Cuando le llega un mensaje al topico `light/control`, el ESP32 lo limpia (le quita espacios, lo pasa a mayusculas) y decide que hacer segun el texto:

| Comando                                    | Que hace                                                                                                                           |
| ------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------- |
| `ON`                                       | Prende el sistema (el LED empieza a parpadear)                                                                                     |
| `OFF`                                      | Apaga todo                                                                                                                         |
| `COLOR:RED` / `COLOR:GREEN` / `COLOR:BLUE` | Cambia el color activo del LED                                                                                                     |
| `BLINK:x`                                  | Fuerza que parpadee a una frecuencia fija de `x` Hz                                                                                |
| `AUTO`                                     | Regresa el control al circuito local: deja de usar una frecuencia fija y usa el potenciometro para decidir que tan rapido parpadea |
| `STATUS`                                   | No cambia nada, solo pide que reporte su estado actual                                                                             |

Un detalle importante: cuando digo que el modo esta en "ON", en realidad el LED **sigue parpadeando** — solo que a la velocidad que marca el potenciometro en vez de una frecuencia forzada. "ON" significa "encendido en modo automatico", y "BLINK" significa "encendido a una frecuencia que yo forcé por comando". Esta logica la herede identica de la practica anterior (ahi tambien existia ese mismo concepto de automatico vs forzado, solo que ahora se controla por MQTT en vez de por Serial/BLE).

Cada vez que un comando cambia algo de verdad, el ESP32 publica automaticamente el nuevo estado en `light/status` — no hace falta pedirlo, se entera solo cualquiera que este suscrito.

## 6. Consulta de estado (STATUS) y telemetria

El mensaje de `light/status` es un JSON con toda la info que pide la practica: modo actual, color, frecuencia (solo cuando aplica, es decir cuando esta en BLINK), segundos desde el ultimo cambio de modo, y la lectura del potenciometro. Por ejemplo:

```json
{ "mode": "BLINK", "color": "GREEN", "freq": 2.5, "time": 4, "pot": 3901 }
```

Aparte de eso, cada segundo publico el valor del potenciometro por separado en `pot/value`, ya normalizado de 0 a 100% (la lectura cruda del ADC va de 0 a 4095).

## 7. El boton fisico: eventos, contador y doble clic

A diferencia de la practica anterior (donde el boton cambiaba el estado del LED directamente), aqui el boton **solo genera eventos** — no controla la luz. Cada vez que lo presiono:

1. Filtro el rebote del boton (antirrebote/debounce): espero 50ms despues de cualquier cambio para confirmar que es un clic real y no un falso contacto del metal.
2. Sumo uno a un contador interno y publico `BUTTON_PRESSED: <numero>` en `button/event`.
3. Si el clic anterior fue hace menos de 400ms, ademas publico `BUTTON_DOUBLE_CLICK` — asi detecto doble clic sin necesitar hardware extra, solo comparando tiempos con `millis()`.

## 8. Estado de conexion (ONLINE / OFFLINE y el LWT)

Configure algo que se llama **Last Will and Testament (LWT)**: es un mensaje que el propio broker manda en mi nombre si el ESP32 se desconecta de forma inesperada (se le va la señal, se apaga de golpe, etc.) — en mi caso publica `OFFLINE` en `device/status`. Y cuando la conexion se hace bien, yo mismo publico `ONLINE` apenas conecto. Esto sirve para que cualquiera que este viendo el sistema sepa si el ESP32 sigue vivo, sin tener que esperar a un timeout larguisimo.

## 9. Que adapte exactamente respecto a la practica anterior (BLE)

La logica de fondo casi no cambio — lo que cambie fue el "medio" de comunicacion:

- El filtro antirrebote del boton: identico.
- El parpadeo no bloqueante con `millis()` (sin usar `delay()`): identico.
- El concepto de modo ON/OFF/BLINK controlado remotamente: identico, solo que antes llegaba por BLE (`onWrite` de una caracteristica) y ahora llega por MQTT (`light/control`).
- Lo que sí es nuevo: el LED paso de ser uno solo a un RGB con seleccion de color, el boton ya no controla el LED directamente sino que solo reporta eventos, y todo el tema de reconexion de WiFi/MQTT y el LWT no existia en la version BLE (Bluetooth no lo necesita de la misma forma).

## 10. El extra: dashboard en Node-RED

Ademas de lo que pide la practica, arme un dashboard visual con **Node-RED** para no tener que escribir comandos a mano cada vez. Node-RED es un programa que se conecta al mismo broker Mosquitto (es un cliente MQTT mas, como el ESP32 o MQTTX) y me deja armar una interfaz con botones, sliders y graficas sin programar HTML a mano.

- Botones para `ON`, `OFF`, `AUTO`, `STATUS`, y cada color
- Un slider que arma el comando `BLINK:x` con la frecuencia que yo elija
- Una grafica en vivo del potenciometro
- Texto en vivo del estado de la luz (ya formateado legible, no el JSON crudo), los eventos del boton, y si el ESP32 esta online

Documentacion completa de esto en [[Practica 3 - Node-RED - Indice]].

## 11. Cosas que me pueden preguntar (y como respondo)

- **"¿Por que Mosquitto y no otro broker?"** — Es open source, ligero, facil de correr local, y es justo el que pide el PDF de la practica.
- **"¿Que pasa si se cae el WiFi?"** — El `loop()` revisa en cada vuelta si sigue conectado, y si no, intenta reconectar solo (tanto WiFi como MQTT), sin necesitar reiniciar el ESP32 a mano.
- **"¿Por que separaste `control` de `status`?"** — Para que la direccion del mensaje sea clara con solo ver el nombre del topico: uno es de entrada, el otro de salida.
- **"¿Que pasa con el retain en `light/status`?"** — Si alguien se conecta despues de que cambie el estado, igual ve el ultimo estado conocido de inmediato, sin tener que esperar a que yo mande otro comando.

---

Ver tambien: [[Practica 3 - Node-RED - Indice]] · [[Practica 3 - Marco Teorico]] (definiciones formales de Wi-Fi, MQTT, Pub/Sub, Broker, Topics, QoS, Retain y LWT para el reporte)
