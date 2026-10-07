import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  plugins: [vue()],
  base: '/',
  build: {
    outDir: '../QuadCapturePanel/Web',
    emptyOutDir: true,
  },
})
