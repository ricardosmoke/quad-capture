<script setup>
import { computed } from 'vue'

const props = defineProps({
  x: { type: Number, required: true },
  y: { type: Number, required: true },
  h: { type: Number, required: true },
  level: { type: Number, default: 0 },
  peak: { type: Number, default: 0 },
  level2: { type: Number, default: null },
  peak2: { type: Number, default: null },
  clip: { type: Boolean, default: true },
})

const labels = computed(() => (
  props.clip ? ['CLIP', '0', '-2', '-6', '-12', '-24', '-48'] : ['0', '-2', '-6', '-12', '-24', '-48']
))

function mark(value) {
  const clamped = Math.min(1, Math.max(0, value))
  return { '--level': clamped, '--peak': clamped }
}
</script>

<template>
  <div class="meter" :style="{ left: `${x}px`, top: `${y}px`, height: `${h + 14}px` }">
    <div class="meter-labels" :style="{ height: `${h}px` }">
      <span v-for="label in labels" :key="label">{{ label }}</span>
    </div>
    <div class="meter-tracks">
      <div class="track" :style="{ height: `${h}px`, ...mark(Math.max(level, 0)) }">
        <i class="peak" :style="{ bottom: `calc(${Math.min(1, Math.max(peak, level, 0)) * 100}% - 1px)` }" />
      </div>
      <div
        v-if="level2 !== null"
        class="track"
        :style="{ height: `${h}px`, ...mark(Math.max(level2, 0)) }"
      >
        <i class="peak" :style="{ bottom: `calc(${Math.min(1, Math.max(peak2 ?? level2, 0)) * 100}% - 1px)` }" />
      </div>
    </div>
    <div v-if="level2 !== null" class="meter-names"><span>1</span><span>2</span></div>
  </div>
</template>

<style scoped>
.meter { position: absolute; display: flex; gap: 4px; }
.meter-labels {
  width: 34px;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  color: #d8d8d8;
  font-size: 8px;
}
.meter-tracks { display: flex; gap: 4px; }
.track {
  position: relative;
  width: 12px;
  background: #101010;
  border-radius: 2px;
  overflow: hidden;
}
.track::before {
  content: "";
  position: absolute;
  left: 2px;
  right: 2px;
  top: 2px;
  bottom: 2px;
  background: linear-gradient(to top, #3dde4a 0 55%, #f0d040 55% 82%, #ff3a32 82% 100%);
  clip-path: inset(calc((1 - var(--level)) * 100%) 0 0 0);
}
.peak {
  position: absolute;
  left: 1px;
  right: 1px;
  height: 2px;
  background: #5cfff0;
}
.meter-names {
  position: absolute;
  left: 38px;
  bottom: 0;
  display: flex;
  gap: 8px;
  font-size: 8px;
  font-weight: 700;
}
</style>
