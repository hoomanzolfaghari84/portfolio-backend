import asyncio
import websockets
import json
from game import GameEngine

class NetworkServer:
    def __init__(self, host="localhost", port=8765):
        self.host = host
        self.port = port
        self.game = GameEngine()
        self.clients = {}  # WebSocket -> player ID
        self.player_queue = []  # Waiting players

    async def game_loop(self):
        """Run the game loop and broadcast state."""
        while True:
            if self.game.is_running:
                self.game.update()
                state = json.dumps(self.game.get_state())
                if self.clients:
                    disconnected = []
                    for ws in list(self.clients.keys()):
                        try:
                            await ws.send(state)
                        except websockets.ConnectionClosed:
                            disconnected.append(ws)
                    # Remove disconnected clients
                    for ws in disconnected:
                        player_id = self.clients.pop(ws, None)
                        if player_id in self.player_queue:
                            self.player_queue.remove(player_id)
                        if self.game.is_running and player_id in self.game.players:
                            self.game.end()
                            print(f"Player {player_id} disconnected, ending game")
                        else:
                            print(f"Player {player_id} disconnected")
                    if not self.clients and self.game.is_running:
                        self.game.end()
                if self.game.is_game_over():
                    self.game.end()
                    self.player_queue = list(self.clients.values())  # Reset queue
            await asyncio.sleep(self.game.tick_rate)

    async def handler(self, websocket):
        """Handle WebSocket connections."""
        player_id = f"player_{len(self.clients) + 1}"
        self.clients[websocket] = player_id
        self.player_queue.append(player_id)
        print(f"Player {player_id} connected")

        if len(self.player_queue) >= 2 and not self.game.is_running:
            p1, p2 = self.player_queue[:2]
            self.player_queue = self.player_queue[2:]
            self.game.start(p1, p2)
            print(f"Game started with {p1} vs {p2}")

        try:
            async for message in websocket:
                data = json.loads(message)
                player_id = self.clients[websocket]
                self.game.process_input(player_id, data)
        except websockets.ConnectionClosed:
            # Handle disconnection in handler too
            player_id = self.clients.pop(websocket, None)
            if player_id in self.player_queue:
                self.player_queue.remove(player_id)
            if self.game.is_running and player_id in self.game.players:
                self.game.end()
                print(f"Player {player_id} disconnected, ending game")
            else:
                print(f"Player {player_id} disconnected")

    async def run(self):
        """Start the server and game loop."""
        server = await websockets.serve(self.handler, self.host, self.port)
        asyncio.create_task(self.game_loop())
        print(f"Server running on ws://{self.host}:{self.port}")
        await server.wait_closed()

if __name__ == "__main__":
    server = NetworkServer()
    asyncio.run(server.run())