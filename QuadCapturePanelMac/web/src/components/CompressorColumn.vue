<script setup>
import CompGraph from './CompGraph.vue'
import KnobControl from './KnobControl.vue'
import MeterBar from './MeterBar.vue'

defineProps({ state: { type: Object, required: true } })
const emit = defineEmits(['command'])
const knobs = ['gate', 'threshold', 'ratio', 'attack', 'release', 'gain']
const labels = ['GATE', 'THRESHOLD', 'RATIO', 'ATTACK', 'RELEASE', 'GAIN']

function drag(channel, knob, active, value) {
  const command = { op: 'comp', channel, knob, active }
  if (value !== undefined) command.value = value
  emit('command', command)
}
</script>

<template>
  <section class="column compressor">
    <h2>COMPRESSOR</h2>
    <div v-for="channel in [0, 1]" :key="channel" class="strip" :style="{ top: `${40 + channel * 246}px` }">
      <button
        class="hw"
        :class="state.comp[channel].bypass ? 'on' : 'off'"
        style="left: 10px; top: 18px; width: 70px; height: 32px"
        @click="emit('command', { op: 'bypass', channel })"
      >BYPASS</button>
      <div class="caption" style="left: 88px; top: 0; width: 36px">GR</div>
      <MeterBar
        :x="94"
        :y="16"
        :h="100"
        :level="state.comp[channel].gr"
        :peak="state.comp[channel].gr"
        :clip="false"
      />
      <CompGraph
        :style="{ left: '196px', top: '8px' }"
        :gate="state.comp[channel].gate"
        :threshold="state.comp[channel].threshold"
        :ratio="state.comp[channel].ratio"
        :gain="state.comp[channel].gain"
        :bypassed="state.comp[channel].bypass"
      />
      <MeterBar
        :x="378"
        :y="8"
        :h="116"
        :level="state.comp[channel].out"
        :peak="state.comp[channel].outPeak"
      />
      <template v-for="(knob, index) in knobs" :key="knob">
        <div class="caption" :style="{ left: `${38 + index * 62}px`, top: '138px', width: '60px' }">
          {{ labels[index] }}
        </div>
        <KnobControl
          :value="state.comp[channel][knob]"
          :x="68 + index * 62"
          :y="180"
          :radius="18"
          :readout="state.comp[channel].text[index]"
          @start="drag(channel, knob, true, state.comp[channel][knob])"
          @move="drag(channel, knob, true, $event)"
          @end="drag(channel, knob, false)"
        />
      </template>
    </div>
    <button
      class="hw"
      :class="state.link ? 'on' : 'off'"
      style="left: 10px; top: 248px; width: 70px; height: 32px"
      @click="emit('command', { op: 'link' })"
    >LINK</button>
  </section>
</template>

<style scoped>
.compressor { left: 210px; top: 38px; width: 446px; height: 520px; }
.strip { position: absolute; left: 0; width: 446px; height: 210px; }
</style>
