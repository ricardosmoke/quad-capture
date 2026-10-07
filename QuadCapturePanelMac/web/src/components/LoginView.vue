<script setup>
import { ref } from 'vue'
import { login } from '../api'

const emit = defineEmits(['enter'])
const password = ref('')
const failed = ref(false)

async function submit() {
  failed.value = false
  if (await login(password.value)) {
    emit('enter')
    return
  }
  failed.value = true
}
</script>

<template>
  <div class="login">
    <form @submit.prevent="submit">
      <h1>QUAD-CAPTURE</h1>
      <input v-model="password" type="password" autocomplete="current-password" autofocus />
      <p v-if="failed" class="bad">Senha incorreta</p>
      <button type="submit">Entrar</button>
    </form>
  </div>
</template>
