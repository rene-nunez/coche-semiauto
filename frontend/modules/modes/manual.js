import mqttService from "../mqtt/mqttService.js";
import {topics} from "../mqtt/topics.js";
import {FIELD, LIGHT} from "../mqtt/protocol.js";

const LIGHTS_OFF = Object.freeze({
    [LIGHT.LEFT]: false,
    [LIGHT.BRAKE]: false,
    [LIGHT.RIGHT]: false
});

const LIGHT_BUTTONS = [
    [LIGHT.LEFT, "leftButton"],
    [LIGHT.BRAKE, "brakeButton"],
    [LIGHT.RIGHT, "rightButton"]
];

const manual = {
    container: null,
    joystick: null,
    puck: null,
    xValue: null,
    yValue: null,
    hornButton: null,
    leftButton: null,
    brakeButton: null,
    rightButton: null,

    dragging: false,
    latestMessage: {[FIELD.X]: 0, [FIELD.Y]: 0},
    sendInterval: null,
    lightInterval: null,
    lightState: {...LIGHTS_OFF},
    abortController: null,

    SEND_INTERVAL_MS: 50,
    DEAD_ZONE: 0.10,
    CURVE_POWER: 1.8,
    FILTER_ALPHA: 0.3, // 0 = muy filtrado, 1 = sin filtro
    lastX: 0,
    lastY: 0,

    get activeLight() {
        return Object.keys(this.lightState).find(type => this.lightState[type]) || null;
    },

    mount(container) {
        if (this.abortController) this.abortController.abort();

        this.container = container;

        container.innerHTML = `
            <div class="mode-wrapper mode-center gap-8">
                <div id="joystick-container" class="relative w-52 h-52 rounded-full border-4 border-base-300 shadow-inner shrink-0 bg-base-200/50">
                    <div class="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-2 h-2 bg-base-content/10 rounded-full pointer-events-none"></div>
                    <div id="joystick-puck" class="absolute w-16 h-16 rounded-full shadow-2xl top-1/2 left-1/2 cursor-grab" style="background: var(--color-success); transform: translate(-50%, -50%); touch-action: none; z-index: 10;"></div>
                </div>
                <div class="flex justify-center gap-8 text-xs font-mono opacity-70">
                    <p>X: <span id="joystick-x" class="font-black text-success text-sm">0.00</span></p>
                    <p>Y: <span id="joystick-y" class="font-black text-success text-sm">0.00</span></p>
                </div>
                <div class="flex flex-col items-center gap-3 w-full max-w-xs">
                    <div class="flex gap-3">
                        <button id="light-left" class="btn btn-outline btn-circle btn-lg">
                            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="h-7 w-7"><path stroke-linecap="round" stroke-linejoin="round" d="M10.5 19.5 3 12m0 0 7.5-7.5M3 12h18" /></svg>
                        </button>
                        <button id="light-brake" class="btn btn-outline btn-circle btn-lg">
                            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="h-7 w-7"><path stroke-linecap="round" stroke-linejoin="round" d="M12 9v3.75m-9.303 3.376c-.866 1.5.217 3.374 1.948 3.374h14.71c1.73 0 2.813-1.874 1.948-3.374L13.949 3.378c-.866-1.5-3.032-1.5-3.898 0L2.697 16.126ZM12 15.75h.007v.008H12v-.008Z" /></svg>
                        </button>
                        <button id="light-right" class="btn btn-outline btn-circle btn-lg">
                            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="h-7 w-7"><path stroke-linecap="round" stroke-linejoin="round" d="M13.5 4.5 21 12m0 0-7.5 7.5M21 12H3" /></svg>
                        </button>
                    </div>
                    <button id="horn-button" class="btn btn-success w-2/3 mt-2 active:scale-98 transition-transform duration-100">Claxon</button>
                </div>
            </div>
        `;

        this.bind();
    },

    bind() {
        this.abortController = new AbortController();
        const {signal} = this.abortController;

        this.joystick = this.container.querySelector("#joystick-container");
        this.puck = this.container.querySelector("#joystick-puck");
        this.xValue = this.container.querySelector("#joystick-x");
        this.yValue = this.container.querySelector("#joystick-y");
        this.hornButton = this.container.querySelector("#horn-button");
        this.leftButton = this.container.querySelector("#light-left");
        this.brakeButton = this.container.querySelector("#light-brake");
        this.rightButton = this.container.querySelector("#light-right");

        if (!this.puck || !this.joystick) return;

        this.puck.addEventListener("mousedown", () => this.startJoystick(), {signal});
        window.addEventListener("mousemove", (event) => this.moveJoystick(event), {signal});
        window.addEventListener("mouseup", () => this.stopJoystick(), {signal});
        window.addEventListener("blur", () => this.stopJoystick(), {signal});

        this.puck.addEventListener("touchstart", (event) => {
            event.preventDefault();
            this.startJoystick();
        }, {signal, passive: false});

        window.addEventListener("touchmove", (event) => {
            if (this.dragging) event.preventDefault();
            this.moveJoystick(event);
        }, {signal, passive: false});

        window.addEventListener("touchend", () => this.stopJoystick(), {signal});
        window.addEventListener("touchcancel", () => this.stopJoystick(), {signal});

        this.hornButton?.addEventListener("click", () => {
            mqttService.publish(topics.action.horn, {[FIELD.HORN]: 1});
        }, {signal});

        this.leftButton?.addEventListener("click", () => this.toggleLight(LIGHT.LEFT), {signal});
        this.brakeButton?.addEventListener("click", () => this.toggleLight(LIGHT.BRAKE), {signal});
        this.rightButton?.addEventListener("click", () => this.toggleLight(LIGHT.RIGHT), {signal});

        this.updateLights();
    },

    startJoystick() {
        this.dragging = true;

        if (!this.sendInterval) {
            this.sendInterval = setInterval(() => {
                mqttService.publish(topics.mode.manual, this.latestMessage);
            }, this.SEND_INTERVAL_MS);
        }
    },

    smooth(value) {
        const sign = Math.sign(value);
        const magnitude = Math.abs(value);

        if (magnitude <= this.DEAD_ZONE) return 0;

        const normalized = (magnitude - this.DEAD_ZONE) / (1 - this.DEAD_ZONE);
        const curved = Math.pow(normalized, this.CURVE_POWER);
        return sign * (this.DEAD_ZONE + curved * (1 - this.DEAD_ZONE));
    },

    moveJoystick(event) {
        if (!this.dragging) return;

        const rect = this.joystick.getBoundingClientRect();
        const radius = this.joystick.offsetWidth / 2;

        const clientX = event.touches ? event.touches[0].clientX : event.clientX;
        const clientY = event.touches ? event.touches[0].clientY : event.clientY;

        let dx = clientX - (rect.left + radius);
        let dy = clientY - (rect.top + radius);

        const distance = Math.sqrt(dx * dx + dy * dy);

        if (distance > radius) {
            dx *= radius / distance;
            dy *= radius / distance;
        }

        this.puck.style.transform = `translate(calc(-50% + ${dx}px), calc(-50% + ${dy}px))`;

        const rawX = dx / radius;
        const rawY = (dy / radius) * -1; // invertir eje Y (arriba positivo)

        const curveX = this.smooth(rawX);
        const curveY = this.smooth(rawY);

        const filteredX = this.FILTER_ALPHA * curveX + (1 - this.FILTER_ALPHA) * this.lastX;
        const filteredY = this.FILTER_ALPHA * curveY + (1 - this.FILTER_ALPHA) * this.lastY;

        this.lastX = filteredX;
        this.lastY = filteredY;

        this.latestMessage = {
            [FIELD.X]: parseFloat(filteredX.toFixed(2)),
            [FIELD.Y]: parseFloat(filteredY.toFixed(2))
        };

        if (this.xValue) this.xValue.innerText = this.latestMessage[FIELD.X].toFixed(2);
        if (this.yValue) this.yValue.innerText = this.latestMessage[FIELD.Y].toFixed(2);
    },

    stopJoystick() {
        if (!this.dragging) return;
        this.dragging = false;

        this.clearSendInterval();

        this.puck.style.transform = "translate(-50%, -50%)";
        this.lastX = 0;
        this.lastY = 0;
        this.latestMessage = {[FIELD.X]: 0, [FIELD.Y]: 0};

        if (this.xValue) this.xValue.innerText = "0.00";
        if (this.yValue) this.yValue.innerText = "0.00";

        mqttService.publish(topics.mode.manual, {[FIELD.X]: 0, [FIELD.Y]: 0});
    },

    clearSendInterval() {
        if (this.sendInterval) {
            clearInterval(this.sendInterval);
            this.sendInterval = null;
        }
    },

    resetLights() {
        this.lightState = {...LIGHTS_OFF};
    },

    publishLight(type) {
        mqttService.publish(topics.action.lights, {[FIELD.LIGHT]: type});
    },

    toggleLight(type) {
        const previous = this.activeLight;

        if (this.lightState[type]) {
            this.lightState[type] = false;
        } else {
            this.resetLights();
            this.lightState[type] = true;
        }

        this.updateLights();

        const current = this.activeLight;

        if (!current) {
            this.clearLightInterval();
            this.publishLight(LIGHT.OFF);
            return;
        }

        if (previous !== current || !this.lightInterval) {
            this.clearLightInterval();

            // Repetir cada 500 ms para producir el parpadeo.
            this.publishLight(current);
            this.lightInterval = setInterval(() => this.publishLight(current), 500);
        }
    },

    clearLightInterval() {
        if (this.lightInterval) {
            clearInterval(this.lightInterval);
            this.lightInterval = null;
        }
    },

    updateLights() {
        for (const [type, key] of LIGHT_BUTTONS) {
            const button = this[key];
            if (!button) continue;

            const active = this.lightState[type];
            button.classList.toggle("btn-success", active);
            button.classList.toggle("btn-outline", !active);
            button.classList.toggle("animate-pulse", active);
        }
    },

    remove() {
        const hadLights = this.activeLight !== null;
        this.resetLights();

        if (hadLights) this.publishLight(LIGHT.OFF);

        if (this.abortController) {
            this.abortController.abort();
            this.abortController = null;
        }

        this.stopJoystick();
        this.clearSendInterval();
        this.clearLightInterval();

        this.container = null;
        this.joystick = null;
        this.puck = null;
        this.xValue = null;
        this.yValue = null;
        this.hornButton = null;
        this.leftButton = null;
        this.brakeButton = null;
        this.rightButton = null;
    }
};

export default manual;
