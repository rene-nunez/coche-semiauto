const THEME_KEY = "control-web-theme";
const THEME_LIGHT = "cmyk";
const THEME_DARK = "dark";

// En navegación privada o con almacenamiento bloqueado, localStorage lanza
// SecurityError; el tema simplemente no se persiste.
function leerTema() {
    try {
        return localStorage.getItem(THEME_KEY);
    } catch {
        return null;
    }
}

function guardarTema(tema) {
    try {
        localStorage.setItem(THEME_KEY, tema);
    } catch {
        // Sin persistencia
    }
}

export function initTheme() {
    const savedTheme = leerTema();
    const html = document.documentElement;
    const controller = document.querySelector(".theme-controller");

    if (savedTheme) {
        // Compatibilidad con versiones anteriores que guardaban "light".
        const theme = savedTheme === "light" || savedTheme === THEME_LIGHT
            ? THEME_LIGHT
            : THEME_DARK;

        html.setAttribute("data-theme", theme);
        guardarTema(theme);

        if (controller) {
            controller.checked = theme === THEME_LIGHT;
        }
    }

    if (controller) {
        controller.addEventListener("change", (event) => {
            const theme = event.target.checked ? THEME_LIGHT : THEME_DARK;
            html.setAttribute("data-theme", theme);
            guardarTema(theme);
        });
    }
}
