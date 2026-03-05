import websocket
import json
import time

# not yet testing

AGV_IP = "192.168.31.7"
PORT = 1202   # WebSocket port


def send_lift(state):
    """
    state > 0 : lift up  (Relay1=1, Relay2=1)
    state < 0 : lift down (Relay1=0, Relay2=0)
    state = 0 : stop (Relay1=0, Relay2=1)
    """

    if state > 0:
        data = ["1", "1", "0", "0", "0", "0", "0", "0"]   # UP
    elif state < 0:
        data = ["0", "0", "0", "0", "0", "0", "0", "0"]   # DOWN / STOP
    else:
    	data = ["0", "1", "0", "0", "0", "0", "0", "0"]

    cmd = {
        "packet": {
            "cmd": "region",
            "region": "TableDeal",
            "index": 1
        },
        "msg": {
            "talk": "setTable",
            "address": "1440",
            "size": 8,
            "data": data,
            "format": "uint8"
        }
    }

    try:
        ws = websocket.WebSocket()
        ws.connect(f"ws://{AGV_IP}:{PORT}")
        ws.send(json.dumps(cmd))
        ws.close()
        print(f"Lift command sent, state={state}")

    except Exception as e:
        print("WebSocket error:", e)


if __name__ == "__main__":
    print("=== Lift control test ===")

    input("Press ENTER to lift UP")
    send_lift(1.0)

    time.sleep(2)
    
    input("Press ENTER to stop")
    send_lift(0.0)

    time.sleep(2)


    input("Press ENTER to lift DOWN")
    send_lift(-1.0)

    time.sleep(2)

    print("Done.")
