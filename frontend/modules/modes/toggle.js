import mqttService from "../mqtt/mqttService.js";
import {ACTION, FIELD} from "../mqtt/protocol.js";

export function createToggleMode(topic) {
    return {
        container: null,
        toggle: null,
        active: false,
        changeHandler: null,

        mount(container) {
            this.container = container;

            container.innerHTML = `
                <div class="mode-wrapper mode-center">
                    <input type="checkbox" class="toggle toggle-success toggle-xl"/>
                </div>
            `;

            this.bind();
        },

        bind() {
            this.toggle = this.container.querySelector(".toggle");
            this.changeHandler = () => this.handleChange();
            this.toggle?.addEventListener("change", this.changeHandler);
        },

        handleChange() {
            this.active = this.toggle.checked;

            mqttService.publish(topic, {
                [FIELD.ACTION]: this.active ? ACTION.ACTIVATE : ACTION.DEACTIVATE
            });
        },

        remove() {
            this.toggle?.removeEventListener("change", this.changeHandler);

            if (this.active) {
                // Publicar la parada aunque se esté saliendo del modo.
                mqttService.publish(topic, {[FIELD.ACTION]: ACTION.DEACTIVATE});
            }

            this.container = null;
            this.toggle = null;
            this.changeHandler = null;
            this.active = false;
        }
    };
}
