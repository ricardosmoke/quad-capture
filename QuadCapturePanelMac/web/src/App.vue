<script setup>
import { onMounted, onUnmounted, ref, shallowRef } from 'vue'
import { openSocket, sessionOk } from './api'
import LoginView from './components/LoginView.vue'
import PanelBoard from './components/PanelBoard.vue'

const authed = ref(false)
const state = ref(null)
const socket = shallowRef(null)
let closed = false
let generation = 0

function connect() {
  const mine = ++generation
  socket.value = openSocket(
    (next) => { state.value = next },
    () => {
      if (!closed && authed.value && mine === generation) setTimeout(connect, 1000)
    },
  )
}

async function enter() {
  authed.value = true
  connect()
}

onMounted(async () => {
  if (await sessionOk()) enter()
})

onUnmounted(() => {
  closed = true
  socket.value?.close()
})
</script>

<template>
  <LoginView v-if="!authed" @enter="enter" />
  <PanelBoard v-else-if="state && socket" :state="state" :socket="socket" />
  <div v-else class="login"><p>Conectando…</p></div>
</template>
