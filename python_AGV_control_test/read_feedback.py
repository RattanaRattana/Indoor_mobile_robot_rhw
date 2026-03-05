import websocket
import json
import csv
from datetime import datetime
import time

# Store odometry data
odom_data = []
last_time = None
frequencies = []

def on_message(ws, message):
    global last_time
    
    try:
        data = json.loads(message)
        msg = data.get('msg', {})
        
        if 'odomx' in msg:
            # Calculate frequency
            current_time = time.time()
            if last_time is not None:
                interval = current_time - last_time
                frequency = 1.0 / interval if interval > 0 else 0
                frequencies.append(frequency)
                
                # Keep only last 10 frequencies for average
                if len(frequencies) > 10:
                    frequencies.pop(0)
                
                avg_freq = sum(frequencies) / len(frequencies)
            else:
                avg_freq = 0
            
            last_time = current_time
            
            # Add timestamp
            msg['timestamp'] = datetime.now().isoformat()
            odom_data.append(msg)
            
            # Print to console with velocity and frequency
            print(f"[{len(odom_data)}] "
                  #f"Pos: x={msg['odomx']:7.2f} y={msg['odomy']:7.2f} θ={msg['odomtheta']:6.2f} | "
                  f"Vel: x={msg['velx']:6.2f} y={msg['vely']:6.2f} θ={msg['veltheta']:6.2f} | ")
                  #f"Freq: {avg_freq:5.1f} Hz")
            
    except Exception as e:
        print(f"Error: {e}")

def on_open(ws):
    print("Connected! Subscribing...")
    ws.send(json.dumps({"packet": {"cmd": "join", "region": "ControlModel"}, "msg": {}}))
    ws.send(json.dumps({"packet": {"cmd": "join", "region": "odom"}, "msg": {}}))
    print("Receiving data... Press Ctrl+C to stop and save\n")
    print("Format: [count] Pos: x y θ | Vel: x y θ | Freq: Hz\n")

def on_close(ws, close_status_code, close_msg):
    # Save data to CSV when connection closes
    if odom_data:
        filename = f"odom_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        with open(filename, 'w', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=odom_data[0].keys())
            writer.writeheader()
            writer.writerows(odom_data)
        print(f"\nSaved {len(odom_data)} records to {filename}")
        
        # Print statistics
        if len(frequencies) > 0:
            avg_freq = sum(frequencies) / len(frequencies)
            print(f"Average publish frequency: {avg_freq:.2f} Hz")
            print(f"Min: {min(frequencies):.2f} Hz, Max: {max(frequencies):.2f} Hz")

ws_url = "ws://192.168.31.7:1202/"
ws = websocket.WebSocketApp(ws_url, on_open=on_open, on_message=on_message, on_close=on_close)

try:
    ws.run_forever()
except KeyboardInterrupt:
    print("\nStopping...")
    ws.close()