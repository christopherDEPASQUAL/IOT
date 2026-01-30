import tailwindcss from '@tailwindcss/vite';
import { sveltekit } from '@sveltejs/kit/vite';
import { defineConfig } from 'vite';

export default defineConfig({
    plugins: [tailwindcss(), sveltekit()],
    envDir: '../bridge',
    envPrefix: ['VITE_', 'PUBLIC_', 'MQTT_', 'WS_']
});
