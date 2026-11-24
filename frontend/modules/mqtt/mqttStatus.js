const STATUS_CLASSES = {
    connecting: "bg-info",
    connected: "bg-success",
    reconnecting: "bg-primary",
    disconnected: "bg-neutral",
    error: "bg-error"
};

export function updateStatus(message, status) {
    const indicator = document.getElementById("status-dot");
    const text = document.getElementById("status-text");

    if (!indicator || !text) return;

    text.textContent = message;

    indicator.classList.remove(...Object.values(STATUS_CLASSES));
    indicator.classList.add(STATUS_CLASSES[status] || "bg-neutral");
    indicator.classList.toggle("animate-pulse", status === "connecting" || status === "reconnecting");
}
