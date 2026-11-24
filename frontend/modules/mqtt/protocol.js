export const MODE = Object.freeze({
    MANUAL: "manual",
    LINE: "seguidor",
    OBSTACLE: "obstaculos",
    NAVIGATION: "navegacion"
});

export const ACTION = Object.freeze({
    ACTIVATE: "activar",
    DEACTIVATE: "desactivar",
    START: "iniciar",
    STOP: "detener",
    RESUME: "reanudar"
});

export const LIGHT = Object.freeze({
    LEFT: "izq",
    BRAKE: "prev",
    RIGHT: "der",
    OFF: "off"
});

export const FIELD = Object.freeze({
    MODE: "modo",
    ACTION: "accion",
    LIGHT: "tipo",
    HORN: "estado",
    X: "x",
    Y: "y",
    LAT: "lat",
    LON: "lon",
    ERROR: "error",
    SATELLITES: "sat",
    DESTINATION: "destino"
});
