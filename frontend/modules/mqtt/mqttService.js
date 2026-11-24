import mqtt from "mqtt";
import {topics} from "./topics.js";
import {actualizarEstado} from "./mqttStatus.js";

const MQTT_PROTOCOL = import.meta.env.VITE_MQTT_PROTOCOL;
const MQTT_HOST = import.meta.env.VITE_MQTT_HOST;
const MQTT_PORT = Number(import.meta.env.VITE_MQTT_PORT);
const MQTT_PATH = import.meta.env.VITE_MQTT_PATH || "";

if (!MQTT_HOST || !MQTT_PORT) throw new Error("Faltan MQTT_HOST o MQTT_PORT");

const BROKER_URL = (() => {
    const protocolo = MQTT_PROTOCOL || (window.location.protocol === "https:" ? "wss" : "ws");
    const path = MQTT_PATH.trim();
    const suffix = path && !path.startsWith("/") ? `/${path}` : path;
    return `${protocolo}://${MQTT_HOST}:${MQTT_PORT}${suffix}`;
})();

const isDev = import.meta.env.DEV;

// Reenviar estos comandos tras una reconexión evita que el ESP32 quede en un
// modo distinto al que muestra la interfaz.
const TOPICOS_REPETIBLES = new Set([
    topics.accion.modo,
    topics.modo.seguidor,
    topics.modo.obstaculos,
    topics.modo.navegacion
]);

const pendientes = new Map();

const mqttService = {
    cliente: null,
    callback: null,

    conectar() {
        if (this.cliente) return;

        this.cliente = mqtt.connect(BROKER_URL, {
            clientId: "control-" + (globalThis.crypto?.randomUUID?.() ?? Math.random().toString(36).slice(2)),
            clean: true,
            reconnectPeriod: 2000,
            connectTimeout: 5000
        });

        this.configurarEventos();
    },

    configurarEventos() {
        this.cliente.on("connect", () => {
            actualizarEstado("Conectado", "connected");
            if (isDev) {
                console.info("[MQTT] conectado", {
                    url: BROKER_URL,
                    clientId: this.cliente.options.clientId
                });
            }

            this.cliente.subscribe(topics.estado.ubicacion, (err) => {
                if (err) console.error(`[MQTT] error al suscribirse (${topics.estado.ubicacion})`, err);
            });

            for (const [topico, mensaje] of pendientes) {
                this.publicar(topico, mensaje);
            }
        });

        this.cliente.on("reconnect", () => {
            actualizarEstado("Reconectando", "reconnecting");
            if (isDev) console.warn("[MQTT] reconectando...");
        });

        this.cliente.on("offline", () => {
            actualizarEstado("Desconectado", "disconnected");
            if (isDev) console.warn("[MQTT] desconectado");
        });

        this.cliente.on("error", (err) => {
            actualizarEstado("Error", "error");
            console.error(`[MQTT] error: ${err.message}`);

            if (isDev && err.stack) console.error(err.stack);
        });

        this.cliente.on("message", (topic, message) => {
            let payload;

            try {
                payload = JSON.parse(message.toString());
            } catch (err) {
                console.error(`[MQTT] mensaje inválido (${topic})`, err.message);
                if (isDev) console.error("Payload:", message.toString());
                return;
            }

            if (isDev) console.info(`[MQTT] mensaje (${topic})`, payload);

            try {
                this.callback?.(topic, payload);
            } catch (err) {
                console.error(`[MQTT] error en el manejador (${topic})`, err);
            }
        });
    },

    recibir(handler) {
        this.callback = handler;
    },

    publicar(topic, mensaje) {
        if (!mensaje || typeof mensaje !== "object" || Array.isArray(mensaje)) {
            console.error(`[MQTT] payload inválido (${topic})`);
            return;
        }

        if (TOPICOS_REPETIBLES.has(topic)) {
            pendientes.set(topic, mensaje);
        }

        if (!this.cliente?.connected) {
            console.warn(`[MQTT] publicación sin conexión (${topic})`);
            return;
        }

        const payload = JSON.stringify(mensaje);

        if (isDev) console.info(`[MQTT] publicando en ${topic}`, mensaje);

        this.cliente.publish(topic, payload, {qos: 0}, (err) => {
            if (err) {
                console.error(`[MQTT] error publish (${topic})`);
                if (isDev) console.error(payload);
            }
        });
    }
};

export default mqttService;
