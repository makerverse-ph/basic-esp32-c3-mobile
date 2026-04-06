
# ESP32-C3 Two-Wheel Robot (TB6612FNG + BLE Control)

This project controls a simple two-wheeled mobile robot using an **ESP32-C3 Super Mini**, a **TB6612FNG motor driver**, and **BLE** commands from a phone or other BLE client.

## Features

- BLE control using single-character commands:
  - `F` = Forward
  - `B` = Backward
  - `L` = Turn Left
  - `R` = Turn Right
  - `S` = Stop
- Startup movement test sequence
- Differential drive control for two TT motors
- Simple BLE setup compatible with ESP32-C3

---

## Hardware Used

- ESP32-C3 Super Mini
- TB6612FNG motor driver
- 2 × TT motors
- Buck converter
- BLE-capable phone or device

---

## Wiring

### TB6612FNG to ESP32-C3

| TB6612FNG | ESP32-C3 |
|---|---|
| STBY | 3.3V |
| AIN1 | GPIO1 |
| AIN2 | GPIO3 |
| PWMA | GPIO0 |
| BIN1 | GPIO5 |
| BIN2 | GPIO6 |
| PWMB | GPIO7 |

### Power

| Connection | Notes |
|---|---|
| TB6612FNG logic supply | From regulated supply as required by your board |
| ESP32-C3 power | Powered from the same 5V buck output line |
| Grounds | **Must be common** between ESP32-C3, TB6612FNG, and power source |


## BLE Information

**BLE device name:**
`ESP32C3_Robot`

### BLE commands

Send these single-character commands to the BLE characteristic:

- `F` → move forward
- `B` → move backward
- `L` → turn left
- `R` → turn right
- `S` → stop

### Recommended BLE apps

Use a BLE client app such as:

- **nRF Connect**
- **LightBlue**

Do **not** expect it to behave like classic Bluetooth serial. The **ESP32-C3 uses BLE**, not classic `BluetoothSerial`.

---

## How the Robot Moves

This is a two-wheel differential drive robot:

- **Forward**: both wheels move robot forward
- **Backward**: both wheels move robot backward
- **Left**: left wheel backward, right wheel forward
- **Right**: left wheel forward, right wheel backward
- **Stop**: both motors stop

---

## Startup Test

On boot, the robot performs a simple movement test:

1. Forward
2. Backward
3. Left
4. Right
5. Stop

This helps verify wiring and motor direction before BLE control begins.

---

## Arduino IDE Notes

Make sure you have:

- ESP32 board support installed
- The correct **ESP32-C3 Super Mini** board selected
- Required BLE libraries available through the ESP32 Arduino core

---

## Troubleshooting

### 1. BLE device does not appear

- Use a **BLE app**, not the normal Bluetooth pairing screen
- Open Serial Monitor at **115200**
- Confirm the board prints BLE startup messages
- Make sure the board is powered and close to your phone

### 2. Motors move in the wrong direction

Check:

- motor wiring polarity
- TB6612FNG direction pin wiring
- especially **BIN2** on the right motor side

### 3. One motor only moves in one direction

This usually means:

- one direction pin is miswired
- one GPIO is not connected properly
- one motor driver input is loose
- one side of the driver is incorrectly connected

In this project, the fix was:
- **correct the BIN2 wiring**

### 4. Robot resets or behaves erratically

Possible causes:

- weak power supply
- insufficient current from the buck converter
- missing common ground
- motor noise causing voltage dips

Recommended:
- use a stable buck converter
- keep grounds common
- add power decoupling if needed

---

## Example Command Flow

After connecting with your BLE app:

1. Send `F`
2. Robot moves forward
3. Send `L`
4. Robot turns left
5. Send `S`
6. Robot stops

---
