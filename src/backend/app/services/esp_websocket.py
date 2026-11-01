import asyncio

from fastapi import WebSocket
from starlette.websockets import WebSocketDisconnect, WebSocketState


class EspWebSocketManager:
    """Tracks the ESP WebSocket and serializes backend-to-ESP messages."""

    def __init__(self) -> None:
        self._socket: WebSocket | None = None
        self._send_lock = asyncio.Lock()

    def connect(self, websocket: WebSocket) -> None:
        self._socket = websocket

    def disconnect(self, websocket: WebSocket) -> None:
        if self._socket is websocket:
            self._socket = None

    async def send_json(self, payload: dict) -> bool:
        websocket = self._socket
        if websocket is None or websocket.client_state != WebSocketState.CONNECTED:
            return False

        async with self._send_lock:
            try:
                await websocket.send_json(payload)
                return True
            except (WebSocketDisconnect, RuntimeError):
                self.disconnect(websocket)
                return False


esp_websocket_manager = EspWebSocketManager()
