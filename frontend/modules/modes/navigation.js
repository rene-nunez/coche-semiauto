import L from "leaflet";
import "leaflet/dist/leaflet.css";
import mqttService from "../mqtt/mqttService.js";
import {topics} from "../mqtt/topics.js";
import {ACTION, FIELD} from "../mqtt/protocol.js";

const TILE_THEME = {
    light: "https://tiles.stadiamaps.com/tiles/alidade_smooth/{z}/{x}/{y}{r}.png",
    dark: "https://tiles.stadiamaps.com/tiles/alidade_smooth_dark/{z}/{x}/{y}{r}.png"
};

export const STATE = Object.freeze({
    NO_DESTINATION: "NO_DESTINATION",
    DESTINATION_SELECTED: "DESTINATION_SELECTED",
    NAVIGATING: "NAVIGATING",
    PAUSED: "PAUSED"
});

const attribution = '&copy; <a href="https://stadiamaps.com/" target="_blank">Stadia Maps</a> &copy; <a href="https://openmaptiles.org/" target="_blank">OpenMapTiles</a> &copy; <a href="https://www.openstreetmap.org/copyright" target="_blank">OpenStreetMap</a>';

const navigation = {
    map: null,
    destination: null,
    lastDestination: null,
    destinationMarker: null,
    carMarker: null,
    resetTimeout: null,
    themeLayer: null,
    layersControl: null,
    themeChangeHandler: null,
    mapClickHandler: null,
    buttonClick: null,

    container: null,
    navigationButton: null,
    distanceValue: null,
    errorValue: null,
    satellitesValue: null,

    state: STATE.NO_DESTINATION,

    icons: {
        smartcar: L.divIcon({
            className: "",
            html: `<svg xmlns="http://www.w3.org/2000/svg" width="35" height="35" viewBox="-1 -1 18 18">
                <path fill="var(--color-info)" stroke="rgba(0,0,0,0.35)" stroke-width="1.2" stroke-linejoin="round" d="M2.52 3.515A2.5 2.5 0 0 1 4.82 2h6.362c1 0 1.904.596 2.298 1.515l.792 1.848c.075.175.21.319.38.404.5.25.855.715.965 1.262l.335 1.679q.05.242.049.49v.413c0 .814-.39 1.543-1 1.997V13.5a.5.5 0 0 1-.5.5h-2a.5.5 0 0 1-.5-.5v-1.338c-1.292.048-2.745.088-4 .088s-2.708-.04-4-.088V13.5a.5.5 0 0 1-.5.5h-2a.5.5 0 0 1-.5-.5v-1.892c-.61-.454-1-1.183-1-1.997v-.413a2.5 2.5 0 0 1 .049-.49l.335-1.68c.11-.546.465-1.012.964-1.261a.8.8 0 0 0 .381-.404l.792-1.848ZM3 10a1 1 0 1 0 0-2 1 1 0 0 0 0 2m10 0a1 1 0 1 0 0-2 1 1 0 0 0 0 2M6 8a1 1 0 0 0 0 2h4a1 1 0 1 0 0-2zM2.906 5.189a.51.51 0 0 0 .497.731c.91-.073 3.35-.17 4.597-.17s3.688.097 4.597.17a.51.51 0 0 0 .497-.731l-.956-1.913A.5.5 0 0 0 11.691 3H4.309a.5.5 0 0 0-.447.276L2.906 5.19Z" />
            </svg>`,
            iconSize: [35, 35],
            iconAnchor: [17.5, 17.5],
            popupAnchor: [0, -18]
        }),

        destination: L.divIcon({
            className: "",
            html: `<svg xmlns="http://www.w3.org/2000/svg" width="36" height="36" viewBox="-2 -2 20 20">
                <path fill="var(--color-success)" stroke="rgba(0,0,0,0.35)" stroke-width="1.2" stroke-linejoin="round" d="M8 16s6-5.686 6-10A6 6 0 0 0 2 6c0 4.314 6 10 6 10zm0-7a3 3 0 1 1 0-6 3 3 0 0 1 0 6z" />
            </svg>`,
            iconSize: [36, 36],
            iconAnchor: [18, 36],
            popupAnchor: [0, -36]
        })
    },

    mount(container) {
        this.container = container;

        container.innerHTML = `
            <div id="map" class="absolute inset-0 z-0"></div>
            <div class="absolute bottom-12 md:bottom-6 left-1/2 -translate-x-1/2 w-[calc(100%-2rem)] max-w-md px-2 flex flex-col gap-3 z-20">
                <div class="grid grid-cols-3 bg-base-200/95 backdrop-blur-md rounded-2xl shadow-xl border border-base-300 overflow-hidden">
                    <div class="flex flex-col items-center py-3">
                        <span class="text-xs font-medium opacity-60">Distancia</span>
                        <span id="distance-value" class="font-sans text-base font-semibold text-success">0.0m</span>
                    </div>
                    <div class="flex flex-col items-center py-3 border-x border-base-300">
                        <span class="text-xs font-medium opacity-60">Error</span>
                        <span id="error-value" class="font-sans text-base font-semibold text-success">0°</span>
                    </div>
                    <div class="flex flex-col items-center py-3">
                        <span class="text-xs font-medium opacity-60">Satélites</span>
                        <span id="satellites-value" class="font-sans text-base font-semibold text-success">0</span>
                    </div>
                </div>
                <button id="navigation-button" class="btn btn-success w-full shadow-lg font-semibold normal-case tracking-wide active:scale-95 transition-transform duration-100">
                    Selecciona un destino
                </button>
            </div>
        `;

        this.bind();
        this.initMap();
        this.updateButton();
    },

    bind() {
        this.navigationButton = this.container.querySelector("#navigation-button");
        this.distanceValue = this.container.querySelector("#distance-value");
        this.errorValue = this.container.querySelector("#error-value");
        this.satellitesValue = this.container.querySelector("#satellites-value");

        this.mapClickHandler = (event) => this.selectDestination(event.latlng);
        this.buttonClick = () => this.controlDestination();

        this.navigationButton?.addEventListener("click", this.buttonClick);
        this.navigationButton?.addEventListener("touchstart", () => this.navigationButton.classList.add("scale-95"), {passive: true});
        this.navigationButton?.addEventListener("touchend", () => this.navigationButton.classList.remove("scale-95"), {passive: true});
    },

    initMap() {
        if (this.map) this.map.remove();

        this.map = L.map("map", {
            center: [0, 0],
            zoom: 16,
            minZoom: 3,
            maxZoom: 21,
            maxBounds: [[-90, -180], [90, 180]],
            maxBoundsViscosity: 1.0,
            worldCopyJump: false
        });

        const themeCheckbox = document.querySelector(".theme-controller");
        const tileUrl = () => themeCheckbox?.checked ? TILE_THEME.light : TILE_THEME.dark;

        this.themeLayer = L.tileLayer(tileUrl(), {
            minZoom: 3,
            maxZoom: 21,
            noWrap: true,
            attribution
        }).addTo(this.map);

        this.themeChangeHandler = () => this.themeLayer.setUrl(tileUrl());
        themeCheckbox?.addEventListener("change", this.themeChangeHandler);

        const outdoorLayer = L.tileLayer("https://tiles.stadiamaps.com/tiles/outdoors/{z}/{x}/{y}{r}.png", {
            minZoom: 3,
            maxZoom: 21,
            noWrap: true,
            attribution
        });

        const satelliteLayer = L.tileLayer("https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}", {
            minZoom: 3,
            maxZoom: 19,
            noWrap: true,
            attribution: "Tiles © Esri — Source: Esri, Maxar, Earthstar Geographics, and the GIS User Community"
        });

        this.layersControl = L.control.layers({
            "Tema": this.themeLayer,
            Outdoors: outdoorLayer,
            "Satélite": satelliteLayer
        }).addTo(this.map);

        this.map.on("click", this.mapClickHandler);
    },

    destinationPopup(latlng) {
        return `
            <div class="text-xs font-mono leading-tight">
                <div class="font-semibold tracking-wide mb-0.5">Destino</div>
                <div><span class="opacity-60">lat </span>${latlng.lat.toFixed(5)}</div>
                <div><span class="opacity-60">lon </span>${latlng.lng.toFixed(5)}</div>
            </div>
        `;
    },

    selectDestination(latlng) {
        this.destination = latlng;

        const isNew = !this.destinationMarker;

        if (isNew) {
            this.destinationMarker = L.marker(latlng, {icon: this.icons.destination})
                .addTo(this.map)
                .bindPopup(this.destinationPopup(latlng));
        }

        this.destinationMarker.setLatLng(latlng).setPopupContent(this.destinationPopup(latlng));
        if (isNew) this.destinationMarker.openPopup();

        if (this.carMarker && this.distanceValue) {
            const distance = latlng.distanceTo(this.carMarker.getLatLng());
            this.distanceValue.textContent = `${distance.toFixed(2)}m`;
        }

        // Si ya estábamos navegando, el nuevo destino debe enviarse al coche;
        // de lo contrario la interfaz mostraría un destino distinto al real.
        if (this.state === STATE.NAVIGATING &&
            (!this.lastDestination || latlng.distanceTo(this.lastDestination) > 1)) {
            this.sendAction(ACTION.START);
        }

        if (this.state === STATE.NO_DESTINATION) this.state = STATE.DESTINATION_SELECTED;

        this.updateButton();
    },

    controlDestination() {
        switch (this.state) {
            case STATE.NO_DESTINATION:
            case STATE.DESTINATION_SELECTED:
                if (!this.destination) return;

                this.sendAction(ACTION.START);
                this.state = STATE.NAVIGATING;
                break;

            case STATE.NAVIGATING:
                this.sendAction(ACTION.STOP);
                this.state = STATE.PAUSED;
                break;

            case STATE.PAUSED:
                if (!this.lastDestination || this.destination.distanceTo(this.lastDestination) > 1) {
                    this.sendAction(ACTION.START);
                } else {
                    this.sendAction(ACTION.RESUME);
                }

                this.state = STATE.NAVIGATING;
                break;
        }

        this.updateButton();
    },

    sendAction(action) {
        mqttService.publish(topics.mode.navigation, {
            [FIELD.ACTION]: action,
            [FIELD.LAT]: Number(this.destination.lat.toFixed(6)),
            [FIELD.LON]: Number(this.destination.lng.toFixed(6))
        });

        this.lastDestination = L.latLng(this.destination);
    },

    updateInterface(lat, lon, error, satellites) {
        if (!this.map) return;

        if (this.errorValue && Number.isFinite(error)) this.errorValue.textContent = `${Math.round(error)}°`;
        if (this.satellitesValue && Number.isInteger(satellites)) this.satellitesValue.textContent = satellites;

        const position = L.latLng(lat, lon);

        if (this.destination && this.distanceValue) {
            const distance = this.destination.distanceTo(position);
            this.distanceValue.textContent = `${distance.toFixed(2)}m`;
        }

        const popup = `
            <div class="text-xs font-mono leading-tight">
                <div class="font-semibold tracking-wide mb-0.5">Coche semiautónomo</div>
                <div><span class="opacity-60">lat </span>${lat.toFixed(5)}</div>
                <div><span class="opacity-60">lon </span>${lon.toFixed(5)}</div>
            </div>
        `;

        if (!this.carMarker) {
            this.carMarker = L.marker(position, {icon: this.icons.smartcar}).addTo(this.map).bindPopup(popup);
            this.map.flyTo(position, 19, {animate: true, duration: 1});
        } else {
            this.carMarker.setLatLng(position).setPopupContent(popup);
        }
    },

    updateButton() {
        if (!this.navigationButton) return;

        this.navigationButton.classList.remove("btn-success", "btn-info", "btn-secondary");

        let text = "";

        if (this.state === STATE.NO_DESTINATION) {
            this.navigationButton.classList.add("btn-success");
            text = "Selecciona un destino";
        } else if (this.state === STATE.DESTINATION_SELECTED) {
            this.navigationButton.classList.add("btn-success");
            text = "Enviar destino";
        } else if (this.state === STATE.NAVIGATING) {
            this.navigationButton.classList.add("btn-secondary");
            text = "Detener navegación";
        } else if (this.state === STATE.PAUSED) {
            if (!this.destination || !this.lastDestination || this.destination.distanceTo(this.lastDestination) > 1) {
                this.navigationButton.classList.add("btn-success");
                text = "Enviar nuevo destino";
            } else {
                this.navigationButton.classList.add("btn-info");
                text = "Reanudar navegación";
            }
        }

        this.navigationButton.textContent = text;
    },

    clearDestination() {
        this.destination = null;
        this.lastDestination = null;
        this.state = STATE.NO_DESTINATION;
        this.updateButton();
    },

    reset() {
        if (this.resetTimeout) {
            clearTimeout(this.resetTimeout);
            this.resetTimeout = null;
        }

        if (this.destinationMarker && this.destination) {
            this.destinationMarker.setPopupContent(`
                <div class="text-xs font-mono leading-tight">
                    <span class="font-semibold tracking-wide">¡Destino alcanzado!</span>
                </div>
            `).openPopup();

            this.resetTimeout = setTimeout(() => {
                this.destinationMarker?.closePopup();
                if (this.destinationMarker) this.map?.removeLayer(this.destinationMarker);

                this.destinationMarker = null;
                this.resetTimeout = null;
                this.clearDestination();
            }, 3000);
        } else {
            if (this.destinationMarker) this.map?.removeLayer(this.destinationMarker);
            this.destinationMarker = null;
            this.clearDestination();
        }
    },

    remove() {
        if (this.resetTimeout) {
            clearTimeout(this.resetTimeout);
            this.resetTimeout = null;
        }

        this.map?.off("click", this.mapClickHandler);

        const themeCheckbox = document.querySelector(".theme-controller");
        themeCheckbox?.removeEventListener("change", this.themeChangeHandler);

        if (this.destinationMarker) this.map?.removeLayer(this.destinationMarker);
        if (this.carMarker) this.map?.removeLayer(this.carMarker);

        this.layersControl?.remove();
        this.themeLayer?.remove();
        this.map?.remove();

        this.navigationButton?.removeEventListener("click", this.buttonClick);

        this.navigationButton = null;
        this.distanceValue = null;
        this.errorValue = null;
        this.satellitesValue = null;
        this.container = null;

        this.map = null;
        this.destinationMarker = null;
        this.carMarker = null;
        this.resetTimeout = null;
        this.themeLayer = null;
        this.layersControl = null;
        this.themeChangeHandler = null;
        this.mapClickHandler = null;
        this.buttonClick = null;

        this.clearDestination();
    }
};

export default navigation;
