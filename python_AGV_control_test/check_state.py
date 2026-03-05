import websocket
import json

ws = websocket.WebSocket()
ws.connect("ws://192.168.31.7:1202")

print("Listening for feedback...")

while True:
    msg = ws.recv()
    print(msg)