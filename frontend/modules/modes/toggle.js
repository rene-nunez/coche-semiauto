import mqttService from "../mqtt/mqttService.js";

export function createToggleMode(topic) {
    return {
        contenedor: null,
        toggle: null,
        activo: false,
        toggleClick: null,

        montar(contenedor) {
            this.contenedor = contenedor;

            contenedor.innerHTML = `
                <div class="mode-wrapper mode-center">
                    <input type="checkbox" class="toggle toggle-success toggle-xl"/>
                </div>
            `;

            this.enlazar();
        },

        enlazar() {
            this.toggle = this.contenedor.querySelector(".toggle");
            this.toggleClick = () => this.controlar();
            this.toggle?.addEventListener("change", this.toggleClick);
        },

        controlar() {
            this.activo = this.toggle.checked;

            mqttService.publicar(topic, {
                accion: this.activo ? "activar" : "desactivar"
            });
        },

        eliminar() {
            this.toggle?.removeEventListener("change", this.toggleClick);

            if (this.activo) {
                // Publicar la parada aunque se esté saliendo del modo.
                mqttService.publicar(topic, {accion: "desactivar"});
            }

            this.contenedor = null;
            this.toggle = null;
            this.toggleClick = null;
            this.activo = false;
        }
    };
}
