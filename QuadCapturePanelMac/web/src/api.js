export async function sessionOk() {
  const response = await fetch('/api/session')
  return response.ok
}

export async function login(password) {
  const response = await fetch('/api/login', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ password }),
  })
  return response.ok
}

export function openSocket(onState, onClose) {
  const proto = location.protocol === 'https:' ? 'wss' : 'ws'
  const socket = new WebSocket(`${proto}://${location.host}/ws`)
  socket.onmessage = (event) => {
    onState(JSON.parse(event.data))
  }
  socket.onclose = () => onClose()
  return socket
}

export function send(socket, command) {
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(JSON.stringify(command))
  }
}
