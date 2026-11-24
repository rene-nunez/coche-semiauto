import "./css/style.css";
import {initTheme} from "./modules/theme/theme.js";
import mqttService from "./modules/mqtt/mqttService.js";
import {topics} from "./modules/mqtt/topics.js";

import manual from "./modules/modes/manual.js";
import seguidor from "./modules/modes/seguidor.js";
import obstaculos from "./modules/modes/obstaculos.js";
import navegacion from "./modules/modes/navegacion.js";

initTheme();
mqttService.conectar();

const modeSelectBtn = document.getElementById("mode-select-btn");
const modeSelectMenu = document.getElementById("mode-select-menu");
const modeInterface = document.getElementById("mode-interface");

let currentMode = null;

function setMode(mode, name) {
    currentMode?.eliminar?.();

    modeInterface.innerHTML = "";
    currentMode = mode;

    currentMode?.montar?.(modeInterface);

    mqttService.publicar(topics.accion.modo, {modo: name});
}

// Al recargar o cerrar no se ejecutan los eliminar(): publicar la parada evita
// que el coche siga avanzando sin supervisión.
window.addEventListener("pagehide", () => {
    if (currentMode === manual) {
        mqttService.publicar(topics.modo.manual, {x: 0, y: 0});
    } else if (currentMode === seguidor) {
        mqttService.publicar(topics.modo.seguidor, {accion: "desactivar"});
    } else if (currentMode === obstaculos) {
        mqttService.publicar(topics.modo.obstaculos, {accion: "desactivar"});
    } else if (currentMode === navegacion && navegacion.estado === "NAVEGANDO") {
        mqttService.publicar(topics.modo.navegacion, {accion: "detener"});
    }
});

// El navegador puede conservar la página en caché al volver: no se recarga el
// script, así que hay que reenviar el estado activo.
window.addEventListener("pageshow", (event) => {
    if (!event.persisted) return;

    if (currentMode === seguidor || currentMode === obstaculos) {
        currentMode.controlar();
    } else if (currentMode === navegacion && navegacion.estado === "NAVEGANDO" && navegacion.destino) {
        navegacion.enviarAccion("iniciar");
    }
});

mqttService.recibir((topic, payload) => {
    if (!payload) return;

    if (topic === topics.estado.ubicacion && currentMode === navegacion) {
        const {lat, lon, error, sat, destino} = payload;

        if (typeof lat === "number" && typeof lon === "number") {
            navegacion.actualizarInterfaz(lat, lon, error, sat);
        }

        if (destino === true) {
            navegacion.reiniciar();
        }
    }
});

modeSelectMenu.addEventListener("click", (event) => {
    const item = event.target.closest("[data-value]");
    if (!item) return;

    const value = item.dataset.value;

    modeSelectBtn.blur();
    document.activeElement?.blur();

    switch (value) {
        case "1":
            modeSelectBtn.textContent = "Manual";
            setMode(manual, "manual");
            break;

        case "2":
            modeSelectBtn.textContent = "Seguidor";
            setMode(seguidor, "seguidor");
            break;

        case "3":
            modeSelectBtn.textContent = "Obstáculos";
            setMode(obstaculos, "obstaculos");
            break;

        case "4":
            modeSelectBtn.textContent = "Navegación";
            setMode(navegacion, "navegacion");
            break;
    }
});
