<script setup>
import KnobControl from './KnobControl.vue'
import MeterBar from './MeterBar.vue'

defineProps({ state: { type: Object, required: true } })
const emit = defineEmits(['command'])
const knobs = [
  { index: 0, label: 'INPUT 1', y: 268, caption: 294 },
  { index: 1, label: 'INPUT 2', y: 348, caption: 374 },
  { index: 2, label: 'COAX (3/4)', y: 430, caption: 456 },
]

function drag(index, active, value) {
  const command = { op: 'mixer', index, active }
  if (value !== undefined) command.value = value
  emit('command', command)
}
</script>

<template>
  <section class="column mixer">
    <h2>MIXER</h2>
    <div class="caption" style="left: 0; top: 42px; width: 196px">OUTPUT 1-2</div>
    <MeterBar
      :x="62"
      :y="64"
      :h="150"
      :level="state.mixerOut[0].level"
      :peak="state.mixerOut[0].peak"
      :level2="state.mixerOut[1].level"
      :peak2="state.mixerOut[1].peak"
    />
    <template v-for="knob in knobs" :key="knob.index">
      <KnobControl
        :value="state.mix[knob.index]"
        :x="98"
        :y="knob.y"
        :readout="state.mixText[knob.index]"
        @start="drag(knob.index, true, state.mix[knob.index])"
        @move="drag(knob.index, true, $event)"
        @end="drag(knob.index, false)"
      />
      <div class="caption" :style="{ left: '0', top: `${knob.caption}px`, width: '196px' }">
        {{ knob.label }}
      </div>
    </template>
  </section>
</template>

<style scoped>
.mixer { left: 662px; top: 38px; width: 196px; height: 520px; }
</style>
