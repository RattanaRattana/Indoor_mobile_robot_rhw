import socket
import json
import time

# not working yet

AGV_IP = "192.168.31.7"   # controller IP
PORT = 1202              # region server port


def send_lift(speed):
    """
    speed > 0 : lift up
    speed < 0 : lift down
    speed = 0 : stop
    """
    msg = {
        "cmd": "region",
        "region": "setUpdownvel",
        "Updownvel": speed
    }

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        sock.connect((AGV_IP, PORT))
        sock.sendall((json.dumps(msg) + "\n").encode("utf-8"))
        sock.close()
        print(f"Sent Updownvel = {speed}")
    except Exception as e:
        print("TCP error:", e)


if __name__ == "__main__":
    print("=== Lift control test ===")

    input("Press ENTER to lift UP")
    send_lift(0.5)

    time.sleep(2)

    input("Press ENTER to STOP")
    send_lift(0.0)

    time.sleep(1)

    input("Press ENTER to lift DOWN")
    send_lift(-0.5)

    time.sleep(2)

    input("Press ENTER to STOP")
    send_lift(0.0)

    print("Done.")
