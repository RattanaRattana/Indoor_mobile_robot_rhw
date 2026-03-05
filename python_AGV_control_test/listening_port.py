import websocket
import json

ws = websocket.WebSocket()
ws.connect("ws://192.168.31.7:1202")

print("Listening... (Ctrl+C to stop)")

try:
    while True:
        msg = ws.recv()
        print(msg)
except KeyboardInterrupt:
    pass
finally:
    ws.close()

# import websocket

# ports = [9002, 9080, 9090, 9091, 9099, 9999]

# for p in ports:
#     try:
#         ws = websocket.WebSocket()
#         ws.settimeout(1)
#         ws.connect(f"ws://192.168.31.7:{p}")
#         print(f"✅ WebSocket OK on port {p}")
#         ws.close()
#     except Exception:
#         print(f"❌ Not WebSocket: {p}")
