const CLASES_ESTADO = {
    connecting: "bg-info",
    connected: "bg-success",
    reconnecting: "bg-primary",
    disconnected: "bg-neutral",
    error: "bg-error"
};

export function actualizarEstado(message, status) {
    const indicador = document.getElementById("status-dot");
    const texto = document.getElementById("status-text");

    if (!indicador || !texto) return;

    texto.textContent = message;

    indicador.classList.remove(...Object.values(CLASES_ESTADO));
    indicador.classList.add(CLASES_ESTADO[status] || "bg-neutral");

    if (status === "connecting" || status === "reconnecting") {
        indicador.classList.add("animate-pulse");
    } else {
        indicador.classList.remove("animate-pulse");
    }
}
