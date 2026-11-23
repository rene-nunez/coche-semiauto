# Coche semiautónomo

Prototipo de vehículo **semiautónomo** con ESP32, controlado desde el navegador mediante MQTT. Puedes conducirlo con un joystick virtual o activar modos de asistencia (seguidor de línea, evasión de obstáculos y navegación GPS). Requiere supervisión humana en todo momento.

![hardware](./assets/hardware.jpeg)

## Cómo funciona

El navegador se conecta al broker por MQTT sobre WebSockets y el ESP32 por MQTT sobre TCP. El broker reenvía comandos y estados entre ambos.

```text
Navegador (Control Web) -- WebSocket :9001 --> Broker MQTT -- TCP :1883 --> Coche semiautónomo (ESP32)
```

> Si usas otros puertos o TLS, actualiza la configuración del broker, del firmware y del Control Web. Los puertos mostrados son los de `mosquitto/mosquitto.conf`.

## Modos

| Modo | Descripción |
|:---|:---|
| **Manual** | Joystick virtual, direccionales, preventivas y claxon. |
| **Seguidor** | Sigue una línea con el arreglo de sensores infrarrojos. |
| **Obstáculos** | Detecta y esquiva objetos con el sensor HC-SR04. |
| **Navegación** | Avanza hacia unas coordenadas GPS marcadas en el mapa. |

El coche se detiene si se pierde la conexión MQTT o si el joystick deja de enviar comandos. Aun así, no sustituye la vigilancia: los modos de asistencia son ayudas, no conducción totalmente autónoma.

## Materiales

| Categoría | Componente | Cantidad |
|:---|:---|:---:|
| **Control** | ESP32 | 1 |
|  | Shield para ESP32 | 1 |
|  | Expansor PCF8574 | 1 |
|  | Buzzer pasivo | 1 |
| **Chasis** | Chasis 2WD | 1 |
|  | Motorreductor 1:48, 3,6–6 V | 2 |
|  | Ruedas | 2 |
|  | Rueda loca | 1 |
|  | Discos de encoder | 2 |
|  | Tornillería M3 | — |
| **Motores** | Driver DRV8833 | 1 |
| **Sensores** | GPS Neo-6M (GY-NEO6MV2) | 1 |
|  | Ultrasónico HC-SR04 | 1 |
|  | Arreglo TCRT5000 | 1 |
|  | Encoders FC-03 | 2 |
| **Alimentación** | Baterías 18650 | 2 |
|  | Portabaterías y cargador para 18650 | 1 c/u |
|  | Regulador LM2596 | 1 |
|  | Capacitor electrolítico de 1000 µF | 1 |
| **Indicadores** | LED rojo | 2 |
|  | LED ámbar | 2 |
|  | Resistencias de 100 Ω | 4 |
| **Conexiones** | Protoboard | 1 |
|  | Cables Dupont | — |

> [!CAUTION]
> Verifica polaridad, voltaje y consumo antes de conectar. Usa un portabaterías y cargador adecuados para las 18650 y no mezcles baterías con distinta carga o estado.

## Puesta en marcha

### 1. Broker MQTT

Necesitas un broker con WebSockets. Se recomienda **Eclipse Mosquitto**: en `mosquitto/mosquitto.conf` tienes una configuración lista para MQTT (1883) y WebSockets (9001). También sirve un broker público (EMQX, HiveMQ, test.mosquitto.org); cada uno usa sus propios puertos y rutas.

ESP32, broker y navegador deben estar en la misma red, salvo que el broker sea accesible desde Internet. Evita redes con aislamiento de punto de acceso.

### 2. Firmware

Necesitas [PlatformIO CLI](https://platformio.org/install/cli). Edita [src/config.cpp](./src/config.cpp) con tu red y broker:

```cpp
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqttHost = "BROKER_HOST";
const int mqttPort = 1883;
```

Luego compila y carga:

```bash
pio run
pio run --target upload
pio device monitor
```

Las librerías se instalan solas: PubSubClient `^2.8.0`, TinyGPSPlus `^1.1.0`, PCF8574 `^0.4.4` y ArduinoJson `^7.2.2`.

### 3. Control Web

```bash
cd web
npm ci
cp .env.example .env   # configura host, puerto y ruta WebSocket del broker
npm run dev
```

Abre `http://localhost:3000`. Para generar los archivos de producción usa `npm run build` (salida en `web/dist`). Consulta [web/README.md](./web/README.md) para más detalles.

### 4. Uso

1. Enciende el ESP32 y el broker, y abre el Control Web.
2. Espera a que el indicador MQTT muestre **Conectado**.
3. Selecciona un modo y usa los controles disponibles.

## MQTT

El Control Web publica comandos y escucha estados; el ESP32 hace lo contrario.

| Publicador | Suscriptor | Tópico | Ejemplo |
|:---|:---|:---|:---|
| Web | ESP32 | `coche-semiauto/accion/modo` | `{"modo":"manual"}` |
| Web | ESP32 | `coche-semiauto/modo/manual` | `{"x":0.5,"y":0.3}` |
| Web | ESP32 | `coche-semiauto/accion/luces` | `{"tipo":"izq"}` |
| Web | ESP32 | `coche-semiauto/accion/claxon` | `{"estado":1}` |
| Web | ESP32 | `coche-semiauto/modo/seguidor` | `{"accion":"activar"}` |
| Web | ESP32 | `coche-semiauto/modo/obstaculos` | `{"accion":"activar"}` |
| Web | ESP32 | `coche-semiauto/modo/navegacion` | `{"accion":"iniciar","lat":19.24,"lon":-103.69}` |
| ESP32 | Web | `coche-semiauto/estado/ubicacion` | `{"lat":19.24,"lon":-103.69,"error":40,"sat":4,"destino":false}` |

| Campo | Valores o rango |
|:---|:---|
| `modo` | `manual`, `seguidor`, `obstaculos`, `navegacion` |
| `tipo` | `izq`, `der`, `prev`, `off` |
| `accion` | `activar`, `desactivar`, `iniciar`, `detener`, `reanudar` |
| `estado` | `0` (no sonar), `1` (sonar) |
| `x`, `y` | `-1.0` a `1.0` (joystick) |
| `lat`, `lon` | Coordenadas decimales |
| `error` | Error de rumbo en grados |
| `sat` | Número de satélites |
| `destino` | `true` al alcanzar el destino; la web reinicia la navegación |

## Licencia

MIT. Consulta [LICENSE](./LICENSE).
