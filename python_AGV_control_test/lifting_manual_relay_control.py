import can
import time
from pynput import keyboard

CAN_INTERFACE = 'socketcan'
CAN_CHANNEL   = 'can0'
CAN_BITRATE   = 500000
CAN_ID_0x38   = 0x38

# State flag
current_direction = 0  # 0 = no key, 1 = up, -1 = down
running = True

def send_can(bus, data_value):
    byte_value = data_value & 0xFF
    msg = can.Message(
        arbitration_id=CAN_ID_0x38,
        data=[byte_value],
        is_extended_id=False
    )
    try:
        bus.send(msg)
        print(f"Sent: {data_value} (0x{byte_value:02X})")
    except can.CanError as e:
        print(f"CAN send error: {e}")

def on_press(key):
    global current_direction
    try:
        if key == keyboard.Key.up:
            current_direction = 1
        elif key == keyboard.Key.down:
            current_direction = -1
    except AttributeError:
        pass

def on_release(key):
    global current_direction, running
    if key == keyboard.Key.up or key == keyboard.Key.down:
        current_direction = 0
    # Press Q to quit
    try:
        if key.char == 'q':
            running = False
            return False  # Stop listener
    except AttributeError:
        pass

def main():
    global running

    print("Connecting to CAN bus...")
    try:
        bus = can.interface.Bus(
            interface=CAN_INTERFACE,
            channel=CAN_CHANNEL,
            bitrate=CAN_BITRATE
        )
        print("CAN bus connected successfully!")
    except Exception as e:
        print(f"Failed to connect to CAN bus: {e}")
        return

    print("\n--- CAN Direction Controller ---")
    print("UP ARROW    : Send +1  (Pin7=HIGH, Pin8=LOW  -> Extend)")
    print("DOWN ARROW  : Send -1  (Pin7=LOW,  Pin8=HIGH -> Retract)")
    print("Release key : Stop sending (Both pins LOW via timeout)")
    print("Q           : Quit")
    print("--------------------------------\n")

    # Start keyboard listener in background thread
    listener = keyboard.Listener(on_press=on_press, on_release=on_release)
    listener.start()

    try:
        while running:
            if current_direction == 1:
                send_can(bus, 1)
                time.sleep(0.1)
            elif current_direction == -1:
                send_can(bus, -1)
                time.sleep(0.1)
            else:
                time.sleep(0.05)  # No key pressed, Arduino will timeout

    except KeyboardInterrupt:
        print("\nInterrupted by user")

    finally:
        listener.stop()
        bus.shutdown()
        print("CAN bus disconnected.")

if __name__ == "__main__":
    main()
