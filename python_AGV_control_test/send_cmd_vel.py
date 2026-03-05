import websocket
import json
import time

ws = websocket.WebSocket()
ws.connect("ws://192.168.31.7:1202")

cmd = {
    "packet": {
        "cmd": "region",
        "region": "cmd_vel",
        "index": 1
    },
    "msg": {
        "xvel": 0.2,
        "yvel": 0,
        "thetavel": 0.00
    }
}

ws.send(json.dumps(cmd))
print("cmd_vel sent")

time.sleep(0.1)
ws.close()
