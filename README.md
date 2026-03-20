# Indoor_mobile_robot_dev

## Moble Robot Platform

## Control Schematic

## Approach Seedbed

<!-- <p align="center">
  <img src="images/approach_seedbed_case_1.gif" alt="Path generation case 1" width="640"/>
  <br>
  Seedbed path generation.
</p> -->

<table align="center">
  <tr>
    <td align="center">
      <img src="images/approach_seedbed_case_1.gif" width="400"><br>
      Case 1
    </td>
    <td align="center">
      <img src="images/approach_seedbed_case_2.gif" width="400"><br>
      Case 2
    </td>
  </tr>
  
</table>
Comparing different cases

## agv_lift_controller

ROS 2 C++ package that exposes three services to control the AGV lift mechanism
via a WebSocket connection (mirrors the original Python script).

---

### Services

| Service      | Relay1 | Relay2 | Effect          |
|--------------|--------|--------|-----------------|
| `/lift_up`   | 1      | 1      | Lift **UP**     |
| `/lift_stop` | 0      | 1      | **Stop** lift   |
| `/lift_down` | 0      | 0      | Lift **DOWN**   |

All three share the standard **`std_srvs/srv/Trigger`** service type:

```
# Request  — no fields required
---
# Response
bool   success
string message
```

---

### Dependencies

```bash
sudo apt install \
  libwebsocketpp-dev \
  libasio-dev \
  nlohmann-json3-dev
```

---

### Build

```bash
# Build
cd ~/amr_ws
colcon build --packages-select idr_robot_hw_cpp
source install/setup.bash
```

---

### Run

```bash
# Default AGV IP/port (192.168.31.7:1202)
ros2 run idr_robot_hw_cpp lift_controller_node

# Override via ROS 2 parameters
ros2 run idr_robot_hw_cpp lift_controller_node \
  --ros-args -p agv_ip:=192.168.1.100 -p agv_port:=1202
```

---

### Call services

```bash
# Lift UP
ros2 service call /lift_up std_srvs/srv/Trigger

# Stop
ros2 service call /lift_stop std_srvs/srv/Trigger

# Lift DOWN
ros2 service call /lift_down std_srvs/srv/Trigger
```

---

### WebSocket payload reference

Each service sends a JSON message identical to the Python original:

```json
{
  "packet": { "cmd": "region", "region": "TableDeal", "index": 1 },
  "msg": {
    "talk": "setTable",
    "address": "1440",
    "size": 8,
    "data": ["1","1","0","0","0","0","0","0"],
    "format": "uint8"
  }
}
```

The `data` array changes per command (see service table above).
