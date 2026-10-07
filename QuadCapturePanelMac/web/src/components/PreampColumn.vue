<script setup>
import KnobControl from './KnobControl.vue'
import MeterBar from './MeterBar.vue'

defineProps({ state: { type: Object, required: true } })
const emit = defineEmits(['command'])

function sens(channel, value) {
  emit('command', { op: 'sens', channel, value })
}
</script>

<template>
  <section class="column preamp">
    <h2>PREAMP</h2>
    <div v-for="channel in [0, 1]" :key="channel" class="strip" :style="{ top: `${40 + channel * 278}px` }">
      <div class="number" style="left: 8px; top: 36px; width: 28px">{{ channel + 1 }}</div>
      <button
        class="hw"
        :class="state.loCut[channel] ? 'on' : 'off'"
        style="left: 44px; top: 8px; width: 72px; height: 30px"
        @click="emit('command', { op: 'loCut', channel })"
      >LO-CUT</button>
      <button
        class="hw"
        :class="state.phase[channel] ? 'on' : 'off'"
        style="left: 44px; top: 44px; width: 72px; height: 26px"
        @click="emit('command', { op: 'phase', channel })"
      >PHASE</button>
      <MeterBar
        :x="126"
        :y="4"
        :h="118"
        :level="state.pre[channel].level"
        :peak="state.pre[channel].peak"
      />
      <div class="caption" style="left: 44px; top: 78px; width: 72px">SENS</div>
      <KnobControl
        :value="state.sens[channel]"
        :x="80"
        :y="124"
        @move="sens(channel, $event)"
      />
      <div class="lcd" style="left: 48px; top: 156px; width: 68px; height: 26px">
        {{ state.sensText[channel] }}
      </div>
    </div>
    <button
      class="hw"
      :class="state.autoSens === 'on' ? 'on' : 'off'"
      style="left: 44px; top: 252px; width: 72px; height: 40px"
      @click="emit('command', { op: 'autoSens' })"
    >AUTO SENS</button>
  </section>
</template>

<style scoped>
.preamp { left: 8px; top: 38px; width: 196px; height: 520px; }
.strip { position: absolute; left: 0; width: 196px; height: 200px; }
</style>
