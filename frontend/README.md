# Control

Control del navegador por MQTT para el coche RC semiautónomo utilizando el joystick virtual y modos de asistencia, conectados al broker por MQTT sobre WebSockets. Los modos, tópicos y montaje del hardware están en el [README](../README.md) principal.

## Requisitos

- Node.js 20.19+ o 22.12+ y npm (requisito de Vite 8).
- Broker MQTT con WebSockets. Se recomienda Mosquitto con la configuración incluida en el repositorio; también funcionan brokers públicos.

## Instalación

```bash
npm ci
cp .env.example .env
```

Edita .env con el protocolo (WebSocket o WebSocket Secure), host, puerto y ruta WebSocket del broker. Los ejemplos para brokers locales y públicos están en [.env.example](./.env.example). No se admite autenticación usuario/contraseña.

## Instrucciones

```bash
npm run dev     # Servidor de desarrollo en http://localhost:3000
npm run build   # Genera los archivos en dist/
npm run preview # Previsualiza la compilación
```

El servidor de desarrollo escucha en todas las interfaces (0.0.0.0:3000) para poder abrirlo desde el móvil en la misma red.

## Mapas

El modo navegación usa tiles de Stadia Maps y Esri World Imagery con Leaflet. En desarrollo funcionan sin configuración; en producción hay que registrar el dominio autorizado en el proveedor ([documentación de Stadia](https://docs.stadiamaps.com/authentication/)). Como alternativa sin restricciones de dominio puedes usar [OpenStreetMap](https://operations.osmfoundation.org/policies/tiles/), respetando sus políticas de uso.
