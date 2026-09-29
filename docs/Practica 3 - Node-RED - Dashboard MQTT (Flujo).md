# Dashboard MQTT en Node-RED - Flujo de la Practica 3

Flujo armado en la pestaña **"Practica 3 - Dashboard MQTT"** del editor de Node-RED. El JSON exportado esta guardado junto a esta nota: `practica3_node-red-flow.json` (se puede volver a importar desde el editor con *Menu (☰) → Import → seleccionar archivo*).

## Configuracion base

| Nodo de config | Valor |
|---|---|
| MQTT Broker | `localhost:1883` (el mismo Mosquitto local del ESP32) |
| Dashboard (ui-base) | sirve en `/dashboard` |
| Pagina (ui-page) | "Practica 3" |
| Grupos (ui-group) | "Control del LED RGB" y "Telemetria" |

## Grupo "Control del LED RGB"

Cada boton simplemente publica un texto fijo al topico `iot/equipo1/light/control` — exactamente los mismos comandos que el ESP32 ya sabe interpretar (`onWrite` / `alRecibirMensajeMQTT` en el sketch).

| Widget | Publica (payload) | Efecto en el ESP32 |
|---|---|---|
| Boton **ON** | `ON` | Prende el sistema |
| Boton **OFF** | `OFF` | Apaga el sistema |
| Boton **AUTO** | `AUTO` | Vuelve al parpadeo controlado por el potenciometro |
| Boton **STATUS** | `STATUS` | Pide que publique el estado actual (no cambia nada) |
| Boton **Rojo** | `COLOR:RED` | Cambia el color activo del LED RGB |
| Boton **Verde** | `COLOR:GREEN` | Cambia el color activo del LED RGB |
| Boton **Azul** | `COLOR:BLUE` | Cambia el color activo del LED RGB |
| Slider **Frecuencia BLINK (Hz)** (0.5 a 10, pasos de 0.5) | pasa por un nodo *function* que arma el texto `BLINK:<valor>` | Fuerza el parpadeo a esa frecuencia |

Todos los botones (y el slider, despues del nodo *function*) se conectan al **mismo** nodo `mqtt out` (`-> light/control`), que ya trae fijo el topico `iot/equipo1/light/control` en su configuracion — asi no hay que repetirlo en cada widget.

### Nodo *function* del slider

```js
msg.payload = "BLINK:" + msg.payload;
return msg;
```

El nodo `ui-slider` manda un numero en `msg.payload` (por ejemplo `2.5`); este nodo lo convierte al formato de comando que espera el ESP32 (`"BLINK:2.5"`).

## Grupo "Telemetria"

Cuatro nodos `mqtt in`, cada uno suscrito a un topico distinto, alimentando distintos widgets:

| Topico suscrito | Widget(s) conectados | Que muestra |
|---|---|---|
| `iot/equipo1/pot/value` | Gauge "Potenciometro" + Grafica de linea | Valor del potenciometro (0-100%), en vivo y su historial |
| `iot/equipo1/light/status` | Texto (via nodo *function* de formato) | Version legible del JSON de estado que publica `publicarEstadoLuz()` (modo, color, frecuencia si aplica, tiempo, pot) |
| `iot/equipo1/button/event` | Texto | `BUTTON_PRESSED: N` y `BUTTON_DOUBLE_CLICK` segun se presione el boton fisico |
| `iot/equipo1/device/status` | Texto | `ONLINE` / `OFFLINE` (este ultimo via el LWT configurado en el ESP32) |

El nodo de `pot/value` tiene **dos** cables de salida (fan-out): el mismo mensaje llega al gauge y a la grafica al mismo tiempo.

### Nodo *function* "Formatear STATUS legible"

El ESP32 publica `light/status` como JSON crudo, por ejemplo:

```json
{"mode":"OFF","color":"RED","time":118,"pot":3901}
```

Eso cumple lo que pide el PDF (modo, color, frecuencia si aplica, tiempo, pot), pero mostrado tal cual en un widget de texto se ve como el objeto crudo, no como texto legible. Este nodo se intercala entre el `mqtt in` de `light/status` y el widget de texto para armar una linea clara:

```js
let data = msg.payload;
if (typeof data === "string") {
    try { data = JSON.parse(data); } catch (e) { return msg; }
}

let texto = "Modo: " + data.mode + "  |  Color: " + data.color;

if (data.mode === "BLINK" && data.freq !== undefined) {
    texto += "  |  Frecuencia: " + data.freq + " Hz";
}

texto += "  |  Tiempo desde el ultimo cambio: " + data.time + "s";
texto += "  |  Potenciometro: " + data.pot;

msg.payload = texto;
return msg;
```

Resultado en el dashboard, por ejemplo:

```
Modo: OFF  |  Color: RED  |  Tiempo desde el ultimo cambio: 118s  |  Potenciometro: 3901
```

Y cuando el modo es BLINK, agrega tambien la frecuencia:

```
Modo: BLINK  |  Color: GREEN  |  Frecuencia: 2.5 Hz  |  Tiempo desde el ultimo cambio: 4s  |  Potenciometro: 3901
```

## Diagrama simplificado

```
[Boton ON]------\
[Boton OFF]------\
[Boton AUTO]------>-- [mqtt out: light/control] --> Mosquitto --> ESP32
[Boton STATUS]---/
[Boton Rojo/Verde/Azul]-/
[Slider Hz] -> [function: "BLINK:"+x] -/

Mosquitto <-- ESP32 publica -->
   iot/equipo1/pot/value      -> [mqtt in] -> [Gauge] + [Grafica]
   iot/equipo1/light/status   -> [mqtt in] -> [Texto]
   iot/equipo1/button/event   -> [mqtt in] -> [Texto]
   iot/equipo1/device/status  -> [mqtt in] -> [Texto]
```

## Como se desplego

En vez de armar el flujo arrastrando nodo por nodo en el editor, se construyo el JSON completo del flujo y se subio directo por la **Admin API** de Node-RED:

```bash
curl -X POST http://127.0.0.1:1880/flows \
  -H "Content-Type: application/json" \
  -H "Node-RED-Deployment-Type: full" \
  --data @practica3_node-red-flow.json
```

Respuesta `204` = desplegado sin errores. Esto es equivalente a armarlo a mano en el editor y darle click a "Deploy" — el archivo JSON es exactamente lo que Node-RED guarda internamente cuando haces eso.

Ver tambien: [[Practica 3 - Node-RED - Que es y para que sirve]] · [[Practica 3 - Node-RED - Instalacion paso a paso]]
