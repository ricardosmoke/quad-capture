<script setup>
import { computed } from 'vue'

const props = defineProps({
  value: { type: Number, required: true },
  x: { type: Number, required: true },
  y: { type: Number, required: true },
  radius: { type: Number, default: 22 },
  readout: { type: String, default: '' },
})
const emit = defineEmits(['start', 'move', 'end'])

const size = computed(() => (props.radius + 3) * 2)
let dragging = false
let originY = 0
let originX = 0
let originValue = 0

const tip = computed(() => {
  const degrees = (props.value * 270 - 135 - 90) * Math.PI / 180
  const reach = props.radius - 3
  return {
    x2: props.radius + 3 + Math.cos(degrees) * reach,
    y2: props.radius + 3 + Math.sin(degrees) * reach,
  }
})

function down(event) {
  dragging = true
  originY = event.clientY
  originX = event.clientX
  originValue = props.value
  event.currentTarget.setPointerCapture(event.pointerId)
  emit('start')
}

function move(event) {
  if (!dragging) return
  const delta = (-(event.clientY - originY) + (event.clientX - originX)) / 160
  emit('move', Math.min(1, Math.max(0, originValue + delta)))
}

function up() {
  if (!dragging) return
  dragging = false
  emit('end')
}
</script>

<template>
  <div
    class="knob"
    :style="{ left: `${x - radius - 3}px`, top: `${y - radius - 3}px`, width: `${size}px`, height: `${size}px` }"
    @pointerdown="down"
    @pointermove="move"
    @pointerup="up"
    @pointercancel="up"
  >
    <svg :width="size" :height="size">
      <line
        :x1="radius + 3"
        :y1="radius + 3"
        :x2="tip.x2"
        :y2="tip.y2"
        stroke="#f3e27a"
        stroke-width="2"
      />
    </svg>
  </div>
  <div v-if="readout" class="readout" :style="{ left: `${x + radius + 6}px`, top: `${y - 8}px` }">
    {{ readout }}
  </div>
</template>
