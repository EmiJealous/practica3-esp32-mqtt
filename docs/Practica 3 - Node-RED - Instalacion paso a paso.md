# Instalacion de Node-RED (paso a paso)

Hecho en macOS, con Node.js ya instalado (`node -v` -> v25.8.0, `npm -v` -> 11.11.0). Node-RED no declara soporte oficial para versiones de Node tan nuevas, pero funciono sin problema.

## 1. Instalar el paquete de Node-RED (global)

```bash
npm install -g --unsafe-perm node-red
```

Esto agrega el comando `node-red` al PATH. `--unsafe-perm` evita problemas de permisos al compilar dependencias nativas durante la instalacion global.

## 2. Instalar el modulo de Dashboard (⚠️ NO va global)

Aqui esta el detalle importante: los "nodos contribuidos" (como el de Dashboard) **no se instalan con `npm install -g`**. Node-RED los busca dentro de su carpeta de usuario, que por defecto es `~/.node-red`. Si los instalas global, Node-RED nunca los va a encontrar.

Ademas, el paquete `node-red-dashboard` "clasico" ya esta **deprecado** (el propio `npm install` lo avisa) — la recomendacion oficial es usar `@flowfuse/node-red-dashboard` (conocido como "Dashboard 2.0"), que es la version mantenida activamente.

```bash
mkdir -p ~/.node-red
cd ~/.node-red
npm init -y            # crea un package.json base (puede fallar el nombre por el punto inicial, no importa)
npm install @flowfuse/node-red-dashboard
```

Si `npm init -y` falla por el nombre de la carpeta (`Invalid name: ".node-red"`), se edita `~/.node-red/package.json` a mano:

```json
{
  "name": "node-red-project",
  "description": "Node-RED user directory",
  "version": "0.0.1",
  "private": true,
  "dependencies": {
    "@flowfuse/node-red-dashboard": "^1.32.0"
  }
}
```

## 3. Arrancar Node-RED

```bash
node-red
```

Deja la terminal abierta (o corrlo en background con `nohup node-red > node-red.log 2>&1 &` si vas a seguir usando la misma terminal). La primera vez imprime un aviso sobre las credenciales encriptadas — es normal, no afecta esta practica.

Cuando arranca bien, el log dice:

```
Server now running at http://127.0.0.1:1880/
Starting flows
Started flows
```

## 4. Abrir el editor y el dashboard

- **Editor de flujos** (donde se arma/edita la logica): `http://127.0.0.1:1880/`
- **Dashboard** (la interfaz final con botones y graficas): `http://127.0.0.1:1880/dashboard`

## 5. Conectar al broker MQTT

Dentro del editor, cualquier nodo `mqtt in` o `mqtt out` necesita un nodo de configuracion "MQTT Broker". Se configura una sola vez:

- **Server**: `localhost`
- **Port**: `1883`
- Sin usuario/contraseña (igual que configuramos Mosquitto para el ESP32)

El flujo especifico que armamos para controlar el LED del ESP32 esta documentado en [[Practica 3 - Node-RED - Dashboard MQTT (Flujo)]].

## Notas / troubleshooting

- Si Node-RED se cierra al cerrar la terminal, es porque no quedo corriendo en background. Para dejarlo persistente entre reinicios de la Mac habria que armar un servicio (como hicimos con Mosquitto via `brew services`), pero para esta practica basta con levantarlo a mano antes de la demo.
- El warning `Theme '' specified in page 'page1' does not exist` que aparece en el log es solo cosmetico (usa el tema por defecto), no afecta el funcionamiento.
