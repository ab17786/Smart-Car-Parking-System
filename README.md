# Smart Car Parking System — Entry/Exit Gate Version (Arduino Uno)


## 1. Project Overview

This project is a count-based smart parking system built on an Arduino Uno.
Instead of monitoring every individual parking slot, it tracks how many
vehicles are currently parked by counting vehicles as they pass through a
single **entry point** and a single **exit point**. Each point has one
proximity sensor and one servo-controlled barrier gate. The number of
available spaces is shown live on a 16x2 LCD.

## 2. Problem Statement

Monitoring every parking slot individually requires one sensor per slot,
which becomes expensive and harder to wire as a lot grows. A simpler
alternative is to control access at the entry and exit points and maintain a
running count of vehicles inside the lot. This project demonstrates that
approach at a small scale, using a single entry gate and a single exit gate.

## 3. Features

- Tracks parking availability by counting vehicles in vs. out, rather than
  using a sensor per slot.
- Automatic barrier gate control at both entry and exit using servo motors.
- Entry is only permitted (barrier opens) when space is available.
- "Parking Full" message shown on the LCD and entry barrier kept closed when
  the lot is at maximum capacity.
- Live display of available parking spaces on a 16x2 LCD.
- Simple presence-based debouncing so one vehicle at a sensor only changes
  the count once, not repeatedly.
- Count is clamped so it can never exceed maximum capacity or drop below
  zero.
- Clearly defined, easily adjustable pin numbers, gate angles, and capacity.

## 4. Hardware Components

| Component                        | Quantity | Purpose                                   |
|-----------------------------------|----------|--------------------------------------------|
| Arduino Uno                      | 1        | Main microcontroller                        |
| Proximity sensor (entry)         | 1        | Detects a vehicle arriving at the entry     |
| Proximity sensor (exit)          | 1        | Detects a vehicle arriving at the exit      |
| Servo motor (entry barrier)      | 1        | Opens/closes the entry gate                 |
| Servo motor (exit barrier)       | 1        | Opens/closes the exit gate                  |
| 16x2 LCD                         | 1        | Displays parking availability               |
| Jumper wires / breadboard        | As needed| Connections and assembly                    |

No IoT modules, Wi-Fi, mobile app integration, or individual slot sensors are
included, since these were not part of the confirmed original feature set.

## 5. Working Principle

The system does not know or care which specific slot a car parks in. It only
needs to know **how many** cars are currently inside the lot. This is done by:

1. Starting with a known `MAX_CAPACITY`.
2. Keeping a running counter, `carsParked`, starting at 0.
3. Incrementing the counter by exactly 1 each time a vehicle is detected
   entering (and space was available).
4. Decrementing the counter by exactly 1 each time a vehicle is detected
   leaving.
5. Deriving "available spaces" at any time as `MAX_CAPACITY - carsParked`,
   and showing that on the LCD.

## 6. Entry Flow

1. Entry proximity sensor detects a vehicle.
2. The system checks if `carsParked < MAX_CAPACITY`.
3. **If space is available:**
   - Entry servo opens the barrier.
   - The system waits for the vehicle to pass (fixed delay).
   - Entry servo closes the barrier.
   - `carsParked` is incremented by exactly 1.
   - The LCD is updated with the new available count.
4. **If parking is full:**
   - The barrier is NOT opened.
   - The LCD displays "Parking Full".

## 7. Exit Flow

1. Exit proximity sensor detects a vehicle.
2. Exit servo opens the barrier.
3. The system waits for the vehicle to pass (fixed delay).
4. Exit servo closes the barrier.
5. `carsParked` is decremented by exactly 1 (never below 0).
6. The LCD is updated with the new available count.

## 8. Vehicle-Counting Logic (Debounce Behavior)

A single vehicle can sit in front of a sensor for many loop iterations while
the barrier opens and it drives through. To make sure this only counts as
**one** entry or exit (not many), the code tracks whether a vehicle was
already "present" at that sensor on the previous check:

- The count only changes on the **transition** from "no vehicle detected" to
  "vehicle detected".
- Once a vehicle moves away and the sensor goes clear, the system re-arms
  itself, ready to detect the next vehicle as a new transition.

This is the same idea as electrical debouncing, but applied to vehicle
presence rather than rapid signal bouncing.

## 9. LCD Functionality

The 16x2 LCD continuously displays:

- Row 1: a static "Parking Status:" label.
- Row 2: either `Available: N` (where `N` is the number of free spaces), or
  `Parking Full` when `carsParked` equals `MAX_CAPACITY`.

The LCD is refreshed every time the count changes (on a successful entry or
exit), and also whenever an entry attempt is blocked because the lot is full.

## 10. System Architecture

```
 ┌───────────────────┐                     ┌───────────────────┐
 │  Entry Proximity   │                     │  Exit Proximity    │
 │      Sensor        │                     │      Sensor        │
 └─────────┬──────────┘                     └─────────┬──────────┘
           │ digital in                                │ digital in
           ▼                                            ▼
 ┌──────────────────────────────────────────────────────────────┐
 │                          Arduino Uno                          │
 │   - handleEntry() / handleExit()                              │
 │   - carsParked counter (0..MAX_CAPACITY)                       │
 │   - openGate() / closeGate() [shared servo helper functions]   │
 │   - updateLCD() / showParkingFull()                            │
 └───────────┬───────────────────────────────┬────────────────────┘
             │ PWM                            │ PWM
             ▼                                ▼
   ┌───────────────────┐            ┌───────────────────┐
   │  Entry Barrier      │            │  Exit Barrier       │
   │  (Servo Motor)      │            │  (Servo Motor)      │
   └───────────────────┘            └───────────────────┘

             Arduino also drives:
                      │
                      ▼
             ┌───────────────────┐
             │     16x2 LCD        │
             │ (availability text)│
             └───────────────────┘
```

## 11. Folder Structure

```
smart-car-parking-gate-system/
├── README.md
└── src/
    └── smart_parking_gate_system.ino
```

## 12. Setup Instructions

1. **Hardware wiring:**
   - Connect the entry proximity sensor's output to `ENTRY_SENSOR_PIN` (2),
     and the exit sensor's output to `EXIT_SENSOR_PIN` (3). Connect their
     VCC/GND as required by your specific sensor module.
   - Connect the entry servo signal wire to `ENTRY_SERVO_PIN` (9), and the
     exit servo signal wire to `EXIT_SERVO_PIN` (10). Power servos from an
     appropriate supply per their datasheet.
   - Wire the 16x2 LCD in 4-bit mode to `LCD_RS` (12), `LCD_EN` (11),
     `LCD_D4` (5), `LCD_D5` (6), `LCD_D6` (7), `LCD_D7` (8), plus standard
     power, ground, and contrast (potentiometer) connections.
2. **Software setup:**
   - Install the [Arduino IDE](https://www.arduino.cc/en/software).
   - Ensure the built-in `Servo` and `LiquidCrystal` libraries are available
     (both ship with the Arduino IDE by default).
   - Open `src/smart_parking_gate_system.ino`.
   - Select **Tools > Board > Arduino Uno** and the correct **Port**.
3. **Configure constants for your setup:**
   - `MAX_CAPACITY` — set to your lot's actual number of spaces.
   - `GATE_OPEN_ANGLE` / `GATE_CLOSED_ANGLE` — set to match your physical
     barrier mounting.
   - `SENSOR_DETECTED_STATE` — flip between `LOW`/`HIGH` if your sensor's
     detected-state logic differs.
   - `GATE_OPEN_DURATION_MS` — adjust to give vehicles enough time to pass.
4. **Upload** the sketch to the Arduino Uno.
5. **Test** by simulating a vehicle at each sensor and confirming the
   barrier opens/closes and the LCD count updates correctly, including the
   "Parking Full" case.

## 13. Limitations

- Counting-based logic assumes every detected entry/exit corresponds to
  exactly one vehicle passing fully through; it cannot verify this
  physically (e.g., no confirmation sensor after the barrier).
- Uses fixed `delay()` calls while a gate is open, which pauses the rest of
  the loop (e.g., the exit sensor is not checked while the entry gate delay
  is running, and vice versa).
- No persistent storage — the `carsParked` count resets to 0 on power-loss
  or restart, regardless of how many vehicles are actually still parked.
- Relies on a single sensor per gate; it cannot detect two vehicles passing
  very close together as two separate events.
- No individual slot-level detail — the system only reports a total count,
  not which specific spaces are occupied.

## 14. Future Improvements

- Replace blocking `delay()` calls with non-blocking, `millis()`-based gate
  timing so entry and exit can be monitored simultaneously without delay.
- Add a secondary sensor after each barrier to confirm a vehicle has fully
  passed before closing the gate.
- Add persistent storage (e.g., EEPROM) so the count survives a power cycle.
- Add per-slot sensors as an optional upgrade for detailed slot-level status
  alongside the existing count-based approach.
- Add logging or a buzzer alert for the "Parking Full" condition.

---

**Disclaimer:** This repository is a reconstruction created for portfolio
purposes after the original project files were lost. It reflects the
described design and components as accurately as possible, but is a fresh
implementation rather than the original submitted code.

