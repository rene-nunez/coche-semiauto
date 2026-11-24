import L from "leaflet";
import "leaflet/dist/leaflet.css";
import mqttService from "../mqtt/mqttService.js";
import {topics} from "../mqtt/topics.js";

const TILES_TEMA = {
    claro: "https://tiles.stadiamaps.com/tiles/alidade_smooth/{z}/{x}/{y}{r}.png",
    oscuro: "https://tiles.stadiamaps.com/tiles/alidade_smooth_dark/{z}/{x}/{y}{r}.png"
};

const navegacion = {
    mapa: null,
    destino: null,
    ultimoDestino: null,
    marcadorD: null,
    marcadorSC: null,
    reinicioTimeout: null,
    layerTema: null,
    layersControl: null,
    temaChangeHandler: null,
    mapaClickHandler: null,
    btnClick: null,

    contenedor: null,
    btnNavegacion: null,
    divDistancia: null,
    divError: null,
    divSatelites: null,

    // SIN_DESTINO: mapa listo, sin destino.
    // DESTINO_SELECCIONADO: destino marcado, pendiente de envío.
    // NAVEGANDO: el coche está en movimiento.
    // PAUSADO: detenido con un destino que se puede reanudar o cambiar.
    estado: "SIN_DESTINO",

    iconos: {
        smartcar: L.divIcon({
            className: "",
            html: `<svg xmlns="http://www.w3.org/2000/svg" width="35" height="35" viewBox="-1 -1 18 18">
                <path fill="var(--color-info)" stroke="rgba(0,0,0,0.35)" stroke-width="1.2" stroke-linejoin="round" d="M2.52 3.515A2.5 2.5 0 0 1 4.82 2h6.362c1 0 1.904.596 2.298 1.515l.792 1.848c.075.175.21.319.38.404.5.25.855.715.965 1.262l.335 1.679q.05.242.049.49v.413c0 .814-.39 1.543-1 1.997V13.5a.5.5 0 0 1-.5.5h-2a.5.5 0 0 1-.5-.5v-1.338c-1.292.048-2.745.088-4 .088s-2.708-.04-4-.088V13.5a.5.5 0 0 1-.5.5h-2a.5.5 0 0 1-.5-.5v-1.892c-.61-.454-1-1.183-1-1.997v-.413a2.5 2.5 0 0 1 .049-.49l.335-1.68c.11-.546.465-1.012.964-1.261a.8.8 0 0 0 .381-.404l.792-1.848ZM3 10a1 1 0 1 0 0-2 1 1 0 0 0 0 2m10 0a1 1 0 1 0 0-2 1 1 0 0 0 0 2M6 8a1 1 0 0 0 0 2h4a1 1 0 1 0 0-2zM2.906 5.189a.51.51 0 0 0 .497.731c.91-.073 3.35-.17 4.597-.17s3.688.097 4.597.17a.51.51 0 0 0 .497-.731l-.956-1.913A.5.5 0 0 0 11.691 3H4.309a.5.5 0 0 0-.447.276L2.906 5.19Z" />
            </svg>`,
            iconSize: [35, 35],
            iconAnchor: [17.5, 17.5],
            popupAnchor: [0, -18]
        }),

        destino: L.divIcon({
            className: "",
            html: `<svg xmlns="http://www.w3.org/2000/svg" width="36" height="36" viewBox="-2 -2 20 20">
                <path fill="var(--color-success)" stroke="rgba(0,0,0,0.35)" stroke-width="1.2" stroke-linejoin="round" d="M8 16s6-5.686 6-10A6 6 0 0 0 2 6c0 4.314 6 10 6 10zm0-7a3 3 0 1 1 0-6 3 3 0 0 1 0 6z" />
            </svg>`,
            iconSize: [36, 36],
            iconAnchor: [18, 36],
            popupAnchor: [0, -36]
        })
    },

    montar(contenedor) {
        this.contenedor = contenedor;

        contenedor.innerHTML = `
            <div id="mapa" class="absolute inset-0 z-0"></div>
            <div class="absolute bottom-12 md:bottom-6 left-1/2 -translate-x-1/2 w-[calc(100%-2rem)] max-w-md px-2 flex flex-col gap-3 z-20">
                <div class="grid grid-cols-3 bg-base-200/95 backdrop-blur-md rounded-2xl shadow-xl border border-base-300 overflow-hidden">
                    <div class="flex flex-col items-center py-3">
                        <span class="text-xs font-medium opacity-60">Distancia</span>
                        <span id="divDistancia" class="font-sans text-base font-semibold text-success">0.0m</span>
                    </div>
                    <div class="flex flex-col items-center py-3 border-x border-base-300">
                        <span class="text-xs font-medium opacity-60">Error</span>
                        <span id="divError" class="font-sans text-base font-semibold text-success">0°</span>
                    </div>
                    <div class="flex flex-col items-center py-3">
                        <span class="text-xs font-medium opacity-60">Satélites</span>
                        <span id="divSatelites" class="font-sans text-base font-semibold text-success">0</span>
                    </div>
                </div>
                <button id="btnNavegacion" class="btn btn-success w-full shadow-lg font-semibold normal-case tracking-wide active:scale-95 transition-transform duration-100">
                    Selecciona un destino
                </button>
            </div>
        `;

        this.enlazar();
        this.iniciarMapa();
        this.actualizarBoton();
    },

    enlazar() {
        this.btnNavegacion = this.contenedor.querySelector("#btnNavegacion");
        this.divDistancia = this.contenedor.querySelector("#divDistancia");
        this.divError = this.contenedor.querySelector("#divError");
        this.divSatelites = this.contenedor.querySelector("#divSatelites");

        this.mapaClickHandler = (event) => this.seleccionarDestino(event.latlng);
        this.btnClick = () => this.controlarDestino();

        this.btnNavegacion?.addEventListener("click", this.btnClick);
        this.btnNavegacion?.addEventListener("touchstart", () => this.btnNavegacion.classList.add("scale-95"), {passive: true});
        this.btnNavegacion?.addEventListener("touchend", () => this.btnNavegacion.classList.remove("scale-95"), {passive: true});
    },

    iniciarMapa() {
        if (this.mapa) this.mapa.remove();

        this.mapa = L.map("mapa", {
            center: [0, 0],
            zoom: 16,
            minZoom: 3,
            maxZoom: 21,
            maxBounds: [[-90, -180], [90, 180]],
            maxBoundsViscosity: 1.0,
            worldCopyJump: false
        });

        const themeCheckbox = document.querySelector(".theme-controller");
        const urlTema = () => themeCheckbox?.checked ? TILES_TEMA.claro : TILES_TEMA.oscuro;

        this.layerTema = L.tileLayer(urlTema(), {
            minZoom: 3,
            maxZoom: 21,
            noWrap: true,
            attribution: '&copy; <a href="https://stadiamaps.com/" target="_blank">Stadia Maps</a> &copy; <a href="https://openmaptiles.org/" target="_blank">OpenMapTiles</a> &copy; <a href="https://www.openstreetmap.org/copyright" target="_blank">OpenStreetMap</a>'
        }).addTo(this.mapa);

        this.temaChangeHandler = () => this.layerTema.setUrl(urlTema());
        themeCheckbox?.addEventListener("change", this.temaChangeHandler);

        const layerOutdoors = L.tileLayer("https://tiles.stadiamaps.com/tiles/outdoors/{z}/{x}/{y}{r}.png", {
            minZoom: 3,
            maxZoom: 21,
            noWrap: true,
            attribution: '&copy; <a href="https://stadiamaps.com/" target="_blank">Stadia Maps</a> &copy; <a href="https://openmaptiles.org/" target="_blank">OpenMapTiles</a> &copy; <a href="https://www.openstreetmap.org/copyright" target="_blank">OpenStreetMap</a>'
        });

        const layerSatellite = L.tileLayer("https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}", {
            minZoom: 3,
            maxZoom: 19,
            noWrap: true,
            attribution: 'Tiles © Esri — Source: Esri, Maxar, Earthstar Geographics, and the GIS User Community'
        });

        this.layersControl = L.control.layers({
            "Tema": this.layerTema,
            Outdoors: layerOutdoors,
            Satélite: layerSatellite
        }).addTo(this.mapa);

        this.mapa.on("click", this.mapaClickHandler);
    },

    popupDestino(latlng) {
        return `
            <div class="text-xs font-mono leading-tight">
                <div class="font-semibold tracking-wide mb-0.5">Destino</div>
                <div><span class="opacity-60">lat </span>${latlng.lat.toFixed(5)}</div>
                <div><span class="opacity-60">lon </span>${latlng.lng.toFixed(5)}</div>
            </div>
        `;
    },

    seleccionarDestino(latlng) {
        this.destino = latlng;

        const nuevo = !this.marcadorD;

        if (nuevo) {
            this.marcadorD = L.marker(latlng, {icon: this.iconos.destino})
                .addTo(this.mapa)
                .bindPopup(this.popupDestino(latlng));
        }

        this.marcadorD.setLatLng(latlng).setPopupContent(this.popupDestino(latlng));
        if (nuevo) this.marcadorD.openPopup();

        if (this.marcadorSC) {
            const dist = latlng.distanceTo(this.marcadorSC.getLatLng());
            if (this.divDistancia) this.divDistancia.textContent = `${dist.toFixed(2)}m`;
        }

        // Si ya estábamos navegando, el nuevo destino debe enviarse al coche;
        // de lo contrario la interfaz mostraría un destino distinto al real.
        if (this.estado === "NAVEGANDO" &&
            (!this.ultimoDestino || latlng.distanceTo(this.ultimoDestino) > 1)) {
            this.enviarAccion("iniciar");
        }

        if (this.estado === "SIN_DESTINO") this.estado = "DESTINO_SELECCIONADO";

        this.actualizarBoton();
    },

    controlarDestino() {
        switch (this.estado) {
            case "SIN_DESTINO":
            case "DESTINO_SELECCIONADO":
                if (!this.destino) return;

                this.enviarAccion("iniciar");
                this.estado = "NAVEGANDO";
                break;

            case "NAVEGANDO":
                this.enviarAccion("detener");
                this.estado = "PAUSADO";
                break;

            case "PAUSADO": {
                if (!this.ultimoDestino || this.destino.distanceTo(this.ultimoDestino) > 1) {
                    this.enviarAccion("iniciar");
                } else {
                    this.enviarAccion("reanudar");
                }

                this.estado = "NAVEGANDO";
                break;
            }
        }

        this.actualizarBoton();
    },

    enviarAccion(accion) {
        mqttService.publicar(topics.modo.navegacion, {
            accion,
            lat: Number(this.destino.lat.toFixed(6)),
            lon: Number(this.destino.lng.toFixed(6))
        });

        this.ultimoDestino = L.latLng(this.destino);
    },

    actualizarInterfaz(lat, lon, error, sat) {
        if (!this.mapa) return;

        if (this.divError && Number.isFinite(error)) this.divError.textContent = `${Math.round(error)}°`;
        if (this.divSatelites && Number.isInteger(sat)) this.divSatelites.textContent = sat;

        const pos = L.latLng(lat, lon);

        if (this.destino && this.divDistancia) {
            const dist = this.destino.distanceTo(pos);
            this.divDistancia.textContent = `${dist.toFixed(2)}m`;
        }

        const popup = `
            <div class="text-xs font-mono leading-tight">
                <div class="font-semibold tracking-wide mb-0.5">Coche semiautónomo</div>
                <div><span class="opacity-60">lat </span>${lat.toFixed(5)}</div>
                <div><span class="opacity-60">lon </span>${lon.toFixed(5)}</div>
            </div>
        `;

        if (!this.marcadorSC) {
            this.marcadorSC = L.marker(pos, {icon: this.iconos.smartcar}).addTo(this.mapa).bindPopup(popup);
            this.mapa.flyTo(pos, 19, {animate: true, duration: 1});
        } else {
            this.marcadorSC.setLatLng(pos).setPopupContent(popup);
        }
    },

    actualizarBoton() {
        if (!this.btnNavegacion) return;

        this.btnNavegacion.classList.remove("btn-success", "btn-info", "btn-secondary");

        let texto = "";

        if (this.estado === "SIN_DESTINO") {
            this.btnNavegacion.classList.add("btn-success");
            texto = "Selecciona un destino";
        } else if (this.estado === "DESTINO_SELECCIONADO") {
            this.btnNavegacion.classList.add("btn-success");
            texto = "Enviar destino";
        } else if (this.estado === "NAVEGANDO") {
            this.btnNavegacion.classList.add("btn-secondary");
            texto = "Detener navegación";
        } else if (this.estado === "PAUSADO") {
            if (!this.destino || !this.ultimoDestino || this.destino.distanceTo(this.ultimoDestino) > 1) {
                this.btnNavegacion.classList.add("btn-success");
                texto = "Enviar nuevo destino";
            } else {
                this.btnNavegacion.classList.add("btn-info");
                texto = "Reanudar navegación";
            }
        }

        this.btnNavegacion.textContent = texto;
    },

    limpiarDestino() {
        this.destino = null;
        this.ultimoDestino = null;
        this.estado = "SIN_DESTINO";
        this.actualizarBoton();
    },

    reiniciar() {
        if (this.reinicioTimeout) {
            clearTimeout(this.reinicioTimeout);
            this.reinicioTimeout = null;
        }

        if (this.marcadorD && this.destino) {
            this.marcadorD.setPopupContent(`
                <div class="text-xs font-mono leading-tight">
                    <span class="font-semibold tracking-wide">¡Destino alcanzado!</span>
                </div>
            `).openPopup();

            this.reinicioTimeout = setTimeout(() => {
                this.marcadorD?.closePopup();
                if (this.marcadorD) this.mapa?.removeLayer(this.marcadorD);

                this.marcadorD = null;
                this.reinicioTimeout = null;
                this.limpiarDestino();
            }, 3000);

        } else {
            if (this.marcadorD) this.mapa?.removeLayer(this.marcadorD);
            this.marcadorD = null;
            this.limpiarDestino();
        }
    },

    eliminar() {
        if (this.reinicioTimeout) {
            clearTimeout(this.reinicioTimeout);
            this.reinicioTimeout = null;
        }

        this.mapa?.off("click", this.mapaClickHandler);

        const themeCheckbox = document.querySelector(".theme-controller");
        themeCheckbox?.removeEventListener("change", this.temaChangeHandler);

        if (this.marcadorD) this.mapa?.removeLayer(this.marcadorD);
        if (this.marcadorSC) this.mapa?.removeLayer(this.marcadorSC);

        this.layersControl?.remove();
        this.layerTema?.remove();
        this.mapa?.remove();

        this.btnNavegacion?.removeEventListener("click", this.btnClick);

        this.btnNavegacion = null;
        this.divDistancia = null;
        this.divError = null;
        this.divSatelites = null;
        this.contenedor = null;

        this.mapa = null;
        this.marcadorD = null;
        this.marcadorSC = null;
        this.reinicioTimeout = null;
        this.layerTema = null;
        this.layersControl = null;
        this.temaChangeHandler = null;
        this.mapaClickHandler = null;
        this.btnClick = null;

        this.limpiarDestino();
    }
};

export default navegacion;
