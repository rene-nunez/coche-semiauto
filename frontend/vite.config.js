import {defineConfig} from "vite";
import tailwindcss from "@tailwindcss/vite";

export default defineConfig({
    plugins: [tailwindcss()],

    server: {
        port: 3000,
        host: true, // Accesible desde otros dispositivos de la red
        strictPort: true
        // Estas opciones solo afectan a npm run dev
    }
});
