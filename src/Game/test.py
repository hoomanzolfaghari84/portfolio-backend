import asyncio
import websockets
import json

async def test():
    try:
        async with websockets.connect("ws://localhost:8765") as ws:
            # Send initial move
            await ws.send(json.dumps({"type": "move", "direction": "right"}))
            print("Sent initial move")
            # Keep receiving updates
            while True:
                response = await ws.recv()
                print(f"Received: {response}")
                # Simulate periodic input
                await asyncio.sleep(1)
                await ws.send(json.dumps({"type": "shoot", "angle": 0}))
                print("Sent shoot")
    except websockets.ConnectionClosedError as e:
        print(f"Connection closed: {e}")
    except Exception as e:
        print(f"Error: {e}")

asyncio.run(test())