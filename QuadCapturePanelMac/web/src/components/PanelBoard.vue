<script setup>
import { onMounted, onUnmounted, ref } from 'vue'
import CompressorColumn from './CompressorColumn.vue'
import MixerColumn from './MixerColumn.vue'
import PreampColumn from './PreampColumn.vue'
import { send } from '../api'

const props = defineProps({
  state: { type: Object, required: true },
  socket: { type: Object, required: true },
})

const viewport = ref(null)
const scale = ref(1)
const menu = ref(false)
const rates = [
  { hz: 44100, label: '44.1 kHz' },
  { hz: 48000, label: '48 kHz' },
  { hz: 96000, label: '96 kHz' },
  { hz: 192000, label: '192 kHz' },
]

function fit() {
  const box = viewport.value
  if (!box) return
  scale.value = Math.min(box.clientWidth / 866, box.clientHeight / 592)
}

function command(value) {
  send(props.socket, value)
}

function chooseRate(hz) {
  menu.value = false
  command({ op: 'rate', hz })
}

let observer
onMounted(() => {
  fit()
  observer = new ResizeObserver(fit)
  observer.observe(viewport.value)
})
onUnmounted(() => observer?.disconnect())
</script>

<template>
  <div ref="viewport" class="viewport">
    <div
      class="stage"
      :style="{ transform: `scale(${scale})` }"
    >
      <header class="bar"><div class="plate">QUAD-CAPTURE</div></header>
      <PreampColumn :state="state" @command="command" />
      <CompressorColumn :state="state" @command="command" />
      <MixerColumn :state="state" @command="command" />
      <footer class="footer">
        <span>SAMPLE RATE</span>
        <button class="rate" @click="menu = !menu">{{ state.sampleRate }}</button>
        <span>CLOCK</span>
        <span class="clock">INTERNAL</span>
        <span v-if="state.error" class="error">{{ state.error }}</span>
        <span class="live" :class="{ off: !state.connected }">
          <i class="dot" />{{ state.connected ? 'IN LIVE' : 'OFFLINE' }}
        </span>
      </footer>
      <div v-if="menu" class="menu">
        <button
          v-for="rate in rates"
          :key="rate.hz"
          :class="{ active: state.sampleRate === rate.label }"
          @click="chooseRate(rate.hz)"
        >{{ rate.label }}</button>
      </div>
      <div v-if="state.settled && !state.connected" class="cover">
        <div class="cover-card">A placa está desconectada</div>
      </div>
    </div>
  </div>
</template>
