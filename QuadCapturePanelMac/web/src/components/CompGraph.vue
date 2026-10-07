<script setup>
import { onMounted, ref, watch } from 'vue'

const props = defineProps({
  gate: { type: Number, required: true },
  threshold: { type: Number, required: true },
  ratio: { type: Number, required: true },
  gain: { type: Number, required: true },
  bypassed: { type: Boolean, default: false },
})

const canvas = ref(null)
const plot = 104
const marks = [-60, -48, -36, -24, -12, 0]

function step(value, upper) {
  return Math.min(upper, Math.max(0, Math.round(Math.min(1, Math.max(0, value)) * upper)))
}

function samples() {
  const gateDb = -70 + step(props.gate, 50)
  const thresholdDb = step(props.threshold, 50) - 50
  const ratioDivisor = [1, 1.2, 1.5, 2, 2.8, 4, 8, 16, 0][step(props.ratio, 8)]
  const gainDb = step(props.gain, 74) - 50
  const outputDb = (input) => {
    if (input <= thresholdDb) return input + gainDb
    if (ratioDivisor === 0) return thresholdDb + gainDb
    return thresholdDb + (input - thresholdDb) / ratioDivisor + gainDb
  }
  const points = []
  if (gateDb > -60) {
    points.push([gateDb, -60], [gateDb, outputDb(gateDb)])
    if (gateDb < thresholdDb) points.push([thresholdDb, outputDb(thresholdDb)])
  } else {
    points.push([-60, outputDb(-60)])
    if (thresholdDb > -60) points.push([thresholdDb, outputDb(thresholdDb)])
  }
  if (points[points.length - 1][0] !== 0) points.push([0, outputDb(0)])
  return pin(points)
}

function pin(raw) {
  if (!raw.length) return []
  const pinned = [raw[0]]
  let previous = raw[0]
  for (const sample of raw.slice(1)) {
    const edges = previous[1] <= sample[1] ? [-60, 0] : [0, -60]
    for (const edge of edges) {
      const from = previous[1] - edge
      const to = sample[1] - edge
      if (from * to < 0) {
        const t = (edge - previous[1]) / (sample[1] - previous[1])
        pinned.push([previous[0] + t * (sample[0] - previous[0]), edge])
      }
    }
    pinned.push([Math.min(0, Math.max(-60, sample[0])), Math.min(0, Math.max(-60, sample[1]))])
    previous = sample
  }
  return pinned
}

function draw() {
  const node = canvas.value
  if (!node) return
  const context = node.getContext('2d')
  const field = props.bypassed ? '#8c8882' : '#e39b45'
  const under = props.bypassed ? '#5c5854' : '#c4621e'
  const grid = props.bypassed ? '#3e3c3a' : '#8a3a12'
  const curve = props.bypassed ? '#c8c4be' : '#fff8ec'
  const scale = props.bypassed ? '#8a8680' : '#d8d8d8'
  const px = (db) => ((db + 60) / 60) * plot
  const py = (db) => plot - ((db + 60) / 60) * plot
  const points = samples()
  context.clearRect(0, 0, plot, plot)
  context.fillStyle = field
  context.fillRect(0, 0, plot, plot)
  context.beginPath()
  context.moveTo(px(points[0][0]), py(points[0][1]))
  for (const point of points.slice(1)) context.lineTo(px(point[0]), py(point[1]))
  context.lineTo(plot, plot)
  context.lineTo(px(points[0][0]), plot)
  context.closePath()
  context.fillStyle = under
  context.fill()
  context.strokeStyle = grid
  context.lineWidth = 1
  marks.forEach((mark, index) => {
    const t = index / (marks.length - 1)
    context.beginPath()
    context.moveTo(t * plot, 0)
    context.lineTo(t * plot, plot)
    context.moveTo(0, plot - t * plot)
    context.lineTo(plot, plot - t * plot)
    context.stroke()
  })
  context.beginPath()
  context.moveTo(px(points[0][0]), py(points[0][1]))
  for (const point of points.slice(1)) context.lineTo(px(point[0]), py(point[1]))
  context.strokeStyle = curve
  context.lineWidth = 1.5
  context.stroke()
  context.fillStyle = scale
  context.font = '8px sans-serif'
  context.textAlign = 'center'
  marks.forEach((mark, index) => {
    const t = index / (marks.length - 1)
    context.fillText(String(mark), t * plot, plot - 2)
  })
}

onMounted(draw)
watch(() => [props.gate, props.threshold, props.ratio, props.gain, props.bypassed], draw)
</script>

<template>
  <canvas ref="canvas" class="graph" width="104" height="104" />
</template>

<style scoped>
.graph { position: absolute; }
</style>
