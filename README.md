# Coche Semiautónomo

Prototipo de coche semiautónomo con ESP32 y control desde el navegador por MQTT. Se maneja con un joystick virtual o mediante modos de asistencia para seguimiento de línea, navegación por GPS y evasión de obstáculos, bajo supervisión humana.

![hardware](./assets/hardware.jpeg)

## Características

- **Manual:** joystick virtual, direccionales, frenos y claxon.
- **Seguidor:** sigue una línea mediante un arreglo de sensores ópticos infrarrojos reflectivos TCRT5000.
- **Obstáculos:** detecta y esquiva objetos con el sensor ultrasónico HC-SR04.
- **Navegación:** avanza hacia coordenadas marcadas en el mapa.
- **Seguridad:** se detiene si se pierde MQTT o WiFi, o si el joystick deja de enviar comandos.

Los modos de asistencia son ayudas y no sustituyen la vigilancia humana.

## Hardware

- **Control:** ESP32, shield, expansor PCF8574 y buzzer pasivo.
- **Chasis:** 2WD, 2 motorreductores 1:48 (3,6–6 V), 2 ruedas, rueda loca, discos de encoder y tornillería M3.
- **Motores:** driver DRV8833.
- **Sensores:** GPS Neo-6M (GY-NEO6MV2), ultrasónico HC-SR04, arreglo TCRT5000 y 2 encoders FC-03.
- **Alimentación:** 2 baterías 18650, portabaterías y cargador, regulador LM2596 y capacitor de 1000 µF.
- **Indicadores:** 2 LED rojos, 2 LED ámbar y 4 resistencias de 100 Ω.
- **Conexiones:** protoboard y cables Dupont.

> [!CAUTION]
> Verifica polaridad, voltaje y consumo antes de conectar. Usa un portabaterías y cargador adecuados para las 18650 y no mezcles baterías con distinta carga o estado.

## Conexiones

El navegador usa MQTT sobre WebSockets; el ESP32 usa MQTT sobre TCP. El broker reenvía comandos y estados entre ambos.

```text
Navegador (frontend) -- WebSocket :9001 --> Broker MQTT -- TCP :1883 --> ESP32 (firmware)
```

Si usas otros puertos o TLS, actualiza mosquitto/mosquitto.conf, firmware/src/config.cpp y frontend/.env.

## Instrucciones

Necesitas un broker con WebSockets, [PlatformIO CLI](https://platformio.org/install/cli) y Node.js 20.19+ o 22.12+.

### 1. Broker

Se recomienda [Eclipse Mosquitto](https://mosquitto.org/): el archivo de configuración local [mosquitto.conf](mosquitto/mosquitto.conf) trae MQTT en 1883 y WebSockets en 9001. También funcionan brokers públicos como EMQX, HiveMQ, y test.mosquitto.org, con sus propios puertos y rutas de acceso.

ESP32, broker y navegador deben estar en la misma red, salvo que el broker sea accesible desde Internet. Evita redes con aislamiento de punto de acceso.

### 2. Firmware

Edita [firmware/src/config.cpp](./firmware/src/config.cpp) con tu red y broker:

```cpp
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

const char* mqttHost = "BROKER_HOST";
const int mqttPort = 1883;
```

Compila y carga desde el directorio firmware:

```bash
cd firmware
pio run
pio run --target upload
pio device monitor
```

Librerías utilizadas:

- PubSubClient
- TinyGPSPlus
- PCF8574
- ArduinoJson

### 3. Frontend

```bash
cd frontend
npm ci
cp .env.example .env
```

Edita .env con el protocolo (WebSocket o WebSocket Secure), host, puerto y ruta WebSocket del broker. Los ejemplos están en [.env.example](./frontend/.env.example). Después inicia el servidor:

```bash
npm run dev
```

Abre [http://localhost:3000](http://localhost:3000). `npm run build` genera frontend/dist. Más detalles en [frontend/README.md](./frontend/README.md).

### 4. Uso

1. Enciende el ESP32 y el broker, y abre el frontend.
2. Espera a que el indicador MQTT muestre el estado "Conectado".
3. Selecciona un modo y usa los controles disponibles.

## MQTT

El frontend publica comandos y escucha la ubicación; el firmware hace lo contrario.

| Publicador | Suscriptor | Tópico | Ejemplo |
|:---|:---|:---|:---|
| Control | ESP32 | `coche-semiauto/accion/modo` | `{"modo":"manual"}` |
| Control | ESP32 | `coche-semiauto/modo/manual` | `{"x":0.5,"y":0.3}` |
| Control | ESP32 | `coche-semiauto/accion/luces` | `{"tipo":"izq"}` |
| Control | ESP32 | `coche-semiauto/accion/claxon` | `{"estado":1}` |
| Control | ESP32 | `coche-semiauto/modo/seguidor` | `{"accion":"activar"}` |
| Control | ESP32 | `coche-semiauto/modo/obstaculos` | `{"accion":"activar"}` |
| Control | ESP32 | `coche-semiauto/modo/navegacion` | `{"accion":"iniciar","lat":19.24,"lon":-103.69}` |
| ESP32 | Control | `coche-semiauto/estado/ubicacion` | `{"lat":19.24,"lon":-103.69,"error":40,"sat":4,"destino":false}` |

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
| `destino` | `true` al alcanzar el destino; el frontend reinicia la navegación |

## Licencia

Este proyecto se distribuye bajo los términos de la Licencia MIT. Consulta el archivo [LICENSE](./LICENSE) para más información.
