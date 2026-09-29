# Practica 3 - Marco Teorico

Conceptos teoricos detras de la practica, para la seccion "Marco Teorico" del reporte. Cada uno explicado en general y despues conectado con como lo use exactamente en mi proyecto.

---

## Wi-Fi

Wi-Fi es un estandar de comunicacion inalambrica (familia de protocolos IEEE 802.11) que permite conectar dispositivos a una red local sin cables, usando radiofrecuencia — tipicamente en las bandas de 2.4GHz y 5GHz. Un dispositivo (como el ESP32) se asocia a un punto de acceso (un router, o en mi caso el Hotspot Personal de un iPhone) y a partir de ahi puede enviar y recibir paquetes IP como si estuviera conectado por cable.

**En mi practica:** el ESP32 usa su radio WiFi integrada para unirse a una red (`WiFi.begin(ssid, password)`), y una vez conectado obtiene una direccion IP local. Un detalle importante: el ESP32 **solo soporta la banda de 2.4GHz**, no 5GHz — esto me genero un problema real durante las pruebas, porque el Hotspot Personal del iPhone necesitaba tener activada la opcion "Maximizar compatibilidad" para que el ESP32 pudiera verlo y conectarse.

## MQTT (Message Queuing Telemetry Transport)

MQTT es un protocolo de mensajeria ligero, pensado especificamente para dispositivos con poca memoria/procesamiento y redes con ancho de banda limitado o inestable — por eso es el protocolo mas usado en proyectos de IoT. Corre sobre TCP/IP, y a diferencia de HTTP (donde un cliente pide algo a un servidor y espera respuesta), MQTT funciona con un modelo distinto: **publish/subscribe**.

**En mi practica:** reemplace la comunicacion BLE de la practica anterior por MQTT, que me permite controlar y monitorear el ESP32 desde cualquier dispositivo en la red (no solo uno conectado por Bluetooth a corta distancia), y ademas deja que varios clientes (mi celular, mi compu, Node-RED) esten viendo/controlando el mismo dispositivo al mismo tiempo.

## Publish / Subscribe (Pub/Sub)

Es el patron de comunicacion en el que se basa MQTT. En vez de que dos dispositivos hablen directamente entre si, existen dos roles:

- **Publisher (publicador):** manda un mensaje a un canal con nombre (topico), sin saber ni importarle quien lo va a leer.
- **Subscriber (suscriptor):** se registra en un topico que le interesa, y recibe automaticamente cualquier mensaje que se publique ahi, sin tener que estar preguntando ("polling").

Esto desacopla a los dispositivos entre si: el publicador no necesita saber la direccion IP ni la existencia del suscriptor, y pueden aparecer o desaparecer clientes sin romper nada.

**En mi practica:** el ESP32 es publicador en varios topicos (`light/status`, `button/event`, `pot/value`, `device/status`) y suscriptor en uno (`light/control`). Node-RED y MQTTX son publicadores del topico de control, y suscriptores de todo lo demas.

## Broker

El broker es el servidor central que recibe todos los mensajes publicados y los reenvia a quien este suscrito al topico correspondiente. Es la pieza que hace posible el modelo publish/subscribe: sin el, publicadores y suscriptores no tendrian forma de encontrarse. En mi caso el broker es **Eclipse Mosquitto**, un broker MQTT open source, corriendo local en mi compu (no en la nube).

**En mi practica:** instale Mosquitto con Homebrew y lo configure para escuchar en toda mi red local (no solo `localhost`), asi el ESP32 (que es otro dispositivo fisico en la red) lo puede alcanzar. Todos mis clientes (ESP32, MQTTX, Node-RED) se conectan al mismo broker en `<ip-de-mi-compu>:1883`.

## Topics (Topicos)

Un topico es basicamente el "nombre del canal" al que se publica o suscribe un mensaje — es una cadena de texto jerarquica separada por `/`, por ejemplo `iot/equipo1/light/status`. No hay que "crear" un topico de antemano: existe en el momento en que alguien publica o se suscribe a el. Se pueden usar comodines al suscribirse, como `#` (todo lo que este debajo de ese nivel) o `+` (un solo nivel), por ejemplo `iot/equipo1/#` trae todos los topicos de mi equipo de una sola vez.

**En mi practica**, use esta estructura, agrupando por "area funcional" del sistema:

| Topico | Direccion | Proposito |
|---|---|---|
| `iot/equipo1/light/control` | Entra al ESP32 | Comandos remotos (ON, OFF, COLOR:x, BLINK:x, AUTO, STATUS) |
| `iot/equipo1/light/status` | Sale del ESP32 | Estado actual de la luz (modo, color, frecuencia, tiempo, pot) |
| `iot/equipo1/device/status` | Sale del ESP32 | ONLINE / OFFLINE |
| `iot/equipo1/button/event` | Sale del ESP32 | Eventos del boton fisico |
| `iot/equipo1/pot/value` | Sale del ESP32 | Lectura del potenciometro cada 1 segundo |

## QoS (Quality of Service)

MQTT define tres niveles de garantia de entrega para cada mensaje:

- **QoS 0** ("at most once"): se manda una vez, sin confirmacion. Puede perderse si hay un corte de red justo en ese momento, pero es el mas rapido y ligero.
- **QoS 1** ("at least once"): el receptor confirma que le llego (con un `PUBACK`); si no llega la confirmacion, se reenvia. Puede llegar duplicado, pero no se pierde.
- **QoS 2** ("exactly once"): el nivel mas seguro, con un intercambio de 4 mensajes para garantizar que llegue exactamente una vez. Es el mas lento y con mas overhead.

**En mi practica** use **QoS 0** en todos los topicos. Lo justifico porque son datos que se refrescan solos muy seguido (el potenciometro cada segundo, el estado cada vez que cambia) — si se llegara a perder un mensaje puntual, el siguiente lo reemplaza casi de inmediato, asi que no vale la pena el costo extra de QoS 1 o 2 para este caso de uso.

## Retain (Mensajes retenidos)

Cuando se publica un mensaje con la bandera `retain` activada, el broker lo guarda como "el ultimo valor conocido" de ese topico. Cualquier cliente que se suscriba **despues**, aunque el mensaje ya se haya mandado hace rato, lo recibe inmediatamente al conectarse — no tiene que esperar a que se publique de nuevo.

**En mi practica** uso retain en `light/status` y `device/status`. Esto es util para dos casos reales que me paso durante las pruebas: si abro MQTTX o el dashboard de Node-RED despues de que el ESP32 ya cambio de estado, igual veo el estado actual de inmediato; y si alguien se conecta al broker por primera vez, sabe al instante si el dispositivo esta ONLINE u OFFLINE sin tener que esperar al proximo evento.

## LWT (Last Will and Testament)

Es un mensaje que el **cliente configura de antemano al conectarse** al broker (topico, payload, QoS y si es retenido), y que el **broker publica automaticamente en su nombre** si detecta que la conexion se corto de forma anormal (no con un `DISCONNECT` limpio, sino porque se fue la señal, se quedo sin bateria, se desconecto el cable, etc.). Es la forma que tiene MQTT de detectar dispositivos "caidos" sin necesitar que el dispositivo avise activamente que se esta apagando.

**En mi practica**, configuro el LWT al conectar el ESP32 al broker: `topic=device/status`, `payload=OFFLINE`, `retain=true`. Si el ESP32 se desconecta de golpe, el broker publica `OFFLINE` solo. Y cuando la conexion se hace exitosamente, yo mismo publico `ONLINE` a mano justo despues de conectar — asi el topico `device/status` siempre refleja el estado real de conexion del dispositivo.

---

Ver tambien: [[Practica 3 - Explicacion completa para exponer en clase]] · [[Practica 3 - Node-RED - Indice]]
