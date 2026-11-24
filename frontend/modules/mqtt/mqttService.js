import mqtt from "mqtt";
import {topics} from "./topics.js";
import {updateStatus} from "./mqttStatus.js";

const MQTT_PROTOCOL = import.meta.env.VITE_MQTT_PROTOCOL;
const MQTT_HOST = import.meta.env.VITE_MQTT_HOST;
const MQTT_PORT = Number(import.meta.env.VITE_MQTT_PORT);
const MQTT_PATH = import.meta.env.VITE_MQTT_PATH || "";

if (!MQTT_HOST || !MQTT_PORT) throw new Error("Faltan MQTT_HOST o MQTT_PORT");

const BROKER_URL = (() => {
    const protocol = MQTT_PROTOCOL || (window.location.protocol === "https:" ? "wss" : "ws");
    const path = MQTT_PATH.trim();
    const suffix = path && !path.startsWith("/") ? `/${path}` : path;
    return `${protocol}://${MQTT_HOST}:${MQTT_PORT}${suffix}`;
})();

const isDev = import.meta.env.DEV;

// Reenviar estos comandos tras una reconexión evita que el ESP32 quede en un
// modo distinto al que muestra la interfaz.
const REPEATABLE_TOPICS = new Set([
    topics.action.mode,
    topics.mode.line,
    topics.mode.obstacle,
    topics.mode.navigation
]);

const pending = new Map();

const mqttService = {
    client: null,
    handler: null,

    connect() {
        if (this.client) return;

        this.client = mqtt.connect(BROKER_URL, {
            clientId: "control-" + (globalThis.crypto?.randomUUID?.() ?? Math.random().toString(36).slice(2)),
            clean: true,
            reconnectPeriod: 2000,
            connectTimeout: 5000
        });

        this.setupEvents();
    },

    setupEvents() {
        this.client.on("connect", () => {
            updateStatus("Conectado", "connected");
            if (isDev) {
                console.info("[MQTT] connected", {
                    url: BROKER_URL,
                    clientId: this.client.options.clientId
                });
            }

            this.client.subscribe(topics.status.location, (err) => {
                if (err) console.error(`[MQTT] subscribe failed (${topics.status.location})`, err);
            });

            for (const [topic, message] of pending) {
                this.publish(topic, message);
            }
        });

        this.client.on("reconnect", () => {
            updateStatus("Reconectando", "reconnecting");
            if (isDev) console.warn("[MQTT] reconnecting...");
        });

        this.client.on("offline", () => {
            updateStatus("Desconectado", "disconnected");
            if (isDev) console.warn("[MQTT] disconnected");
        });

        this.client.on("error", (err) => {
            updateStatus("Error", "error");
            console.error(`[MQTT] error: ${err.message}`);
            if (isDev && err.stack) console.error(err.stack);
        });

        this.client.on("message", (topic, message) => {
            let payload;

            try {
                payload = JSON.parse(message.toString());
            } catch (err) {
                console.error(`[MQTT] invalid message (${topic})`, err.message);
                if (isDev) console.error("Payload:", message.toString());
                return;
            }

            if (isDev) console.info(`[MQTT] message (${topic})`, payload);

            try {
                this.handler?.(topic, payload);
            } catch (err) {
                console.error(`[MQTT] handler failed (${topic})`, err);
            }
        });
    },

    onMessage(handler) {
        this.handler = handler;
    },

    publish(topic, message) {
        if (!message || typeof message !== "object" || Array.isArray(message)) {
            console.error(`[MQTT] invalid payload (${topic})`);
            return;
        }

        if (REPEATABLE_TOPICS.has(topic)) {
            pending.set(topic, message);
        }

        if (!this.client?.connected) {
            console.warn(`[MQTT] publish without connection (${topic})`);
            return;
        }

        const payload = JSON.stringify(message);

        if (isDev) console.info(`[MQTT] publishing to ${topic}`, message);

        this.client.publish(topic, payload, {qos: 0}, (err) => {
            if (err) console.error(`[MQTT] publish failed (${topic})`, err);
        });
    }
};

export default mqttService;
