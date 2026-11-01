const THEME_KEY = "control-web-theme";
const THEME_LIGHT = "cmyk";
const THEME_DARK = "dark";

export function initTheme() {
    const savedTheme = localStorage.getItem(THEME_KEY);
    const html = document.documentElement;
    const controller = document.querySelector(".theme-controller");

    if (savedTheme) {
        // Compatibilidad con versiones anteriores que guardaban "light".
        const theme = savedTheme === "light" || savedTheme === THEME_LIGHT
            ? THEME_LIGHT
            : THEME_DARK;

        html.setAttribute("data-theme", theme);
        localStorage.setItem(THEME_KEY, theme);

        if (controller) {
            controller.checked = theme === THEME_LIGHT;
        }
    }

    if (controller) {
        controller.addEventListener("change", (event) => {
            const theme = event.target.checked ? THEME_LIGHT : THEME_DARK;
            html.setAttribute("data-theme", theme);
            localStorage.setItem(THEME_KEY, theme);
        });
    }
}
