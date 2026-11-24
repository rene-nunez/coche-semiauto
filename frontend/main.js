import "./css/style.css";
import {initTheme} from "./modules/theme/theme.js";
import mqttService from "./modules/mqtt/mqttService.js";
import {topics} from "./modules/mqtt/topics.js";
import {ACTION, FIELD, MODE} from "./modules/mqtt/protocol.js";

import manual from "./modules/modes/manual.js";
import line from "./modules/modes/line.js";
import obstacle from "./modules/modes/obstacle.js";
import navigation, {STATE} from "./modules/modes/navigation.js";

const MODES = {
    "1": {module: manual, label: "Manual", mode: MODE.MANUAL},
    "2": {module: line, label: "Seguidor", mode: MODE.LINE},
    "3": {module: obstacle, label: "Obstáculos", mode: MODE.OBSTACLE},
    "4": {module: navigation, label: "Navegación", mode: MODE.NAVIGATION}
};

initTheme();
mqttService.connect();

const modeSelectBtn = document.getElementById("mode-select-btn");
const modeSelectMenu = document.getElementById("mode-select-menu");
const modeInterface = document.getElementById("mode-interface");

let currentMode = null;

function setMode(mode, name) {
    currentMode?.remove?.();

    modeInterface.innerHTML = "";
    currentMode = mode;
    currentMode?.mount?.(modeInterface);

    mqttService.publish(topics.action.mode, {[FIELD.MODE]: name});
}

// Al recargar o cerrar no se ejecutan los remove(): publicar la parada evita
// que el coche siga avanzando sin supervisión.
window.addEventListener("pagehide", () => {
    if (currentMode === manual) {
        mqttService.publish(topics.mode.manual, {[FIELD.X]: 0, [FIELD.Y]: 0});
    } else if (currentMode === line || currentMode === obstacle) {
        const topic = currentMode === line ? topics.mode.line : topics.mode.obstacle;
        mqttService.publish(topic, {[FIELD.ACTION]: ACTION.DEACTIVATE});
    } else if (currentMode === navigation && navigation.state === STATE.NAVIGATING) {
        mqttService.publish(topics.mode.navigation, {[FIELD.ACTION]: ACTION.STOP});
    }
});

// El navegador puede conservar la página en caché al volver: no se recarga el
// script, así que hay que reenviar el estado activo.
window.addEventListener("pageshow", (event) => {
    if (!event.persisted) return;

    if (currentMode === line || currentMode === obstacle) {
        currentMode.handleChange();
    } else if (currentMode === navigation && navigation.state === STATE.NAVIGATING && navigation.destination) {
        navigation.sendAction(ACTION.START);
    }
});

mqttService.onMessage((topic, payload) => {
    if (!payload) return;

    if (topic === topics.status.location && currentMode === navigation) {
        const lat = payload[FIELD.LAT];
        const lon = payload[FIELD.LON];

        if (typeof lat === "number" && typeof lon === "number") {
            navigation.updateInterface(lat, lon, payload[FIELD.ERROR], payload[FIELD.SATELLITES]);
        }

        if (payload[FIELD.DESTINATION] === true) navigation.reset();
    }
});

modeSelectMenu.addEventListener("click", (event) => {
    const item = event.target.closest("[data-value]");
    const selected = item && MODES[item.dataset.value];
    if (!selected) return;

    modeSelectBtn.blur();
    document.activeElement?.blur();
    modeSelectBtn.textContent = selected.label;
    setMode(selected.module, selected.mode);
});
