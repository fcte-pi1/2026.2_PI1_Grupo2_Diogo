import react from '@vitejs/plugin-react'
import { defineConfig } from 'vitest/config'
import tailwindcss from '@tailwindcss/vite'

import svgr from 'vite-plugin-svgr';

// https://vite.dev/config/
export default defineConfig({
  plugins: [tailwindcss(), react(), svgr()],
  test: { environment: 'jsdom', globals: true },
  server: {
    proxy: {
      '/api': process.env.API_URL ?? 'http://localhost:8000',
      '/ws': { target: process.env.API_URL ?? 'http://localhost:8000', ws: true },
    }
  }
})
