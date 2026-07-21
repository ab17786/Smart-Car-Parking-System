#include <Servo.h>
#include <LiquidCrystal.h>

// ---------------------------------------------------------------------------
// 1. PIN CONFIGURATION
// ---------------------------------------------------------------------------
// Change these to match your actual wiring.

// Proximity sensors (entry/exit vehicle detection)
const int ENTRY_SENSOR_PIN = 2;
const int EXIT_SENSOR_PIN  = 3;

// Servo motors controlling the barrier gates
const int ENTRY_SERVO_PIN = 9;
const int EXIT_SERVO_PIN  = 10;

// 16x2 LCD pins (using the standard LiquidCrystal library in 4-bit mode)
// Wiring order below matches: LiquidCrystal(RS, EN, D4, D5, D6, D7)
const int LCD_RS = 12;
const int LCD_EN = 11;
const int LCD_D4 = 5;
const int LCD_D5 = 6;
const int LCD_D6 = 7;
const int LCD_D7 = 8;

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// ---------------------------------------------------------------------------
// 2. SENSOR LOGIC LEVEL
// ---------------------------------------------------------------------------
// Many IR/proximity sensor modules pull their output LOW when an object is
// detected, and HIGH when clear. If your sensor behaves the opposite way,
// flip this constant.
const int SENSOR_DETECTED_STATE = LOW;

// ---------------------------------------------------------------------------
// 3. SERVO GATE ANGLES
// ---------------------------------------------------------------------------
// Adjust these to match how your physical barrier is mounted.
const int GATE_CLOSED_ANGLE = 0;
const int GATE_OPEN_ANGLE   = 90;

// How long (ms) to keep the gate open so a vehicle can pass through
// before closing it again. Adjust based on your real-world timing.
const unsigned long GATE_OPEN_DURATION_MS = 3000;

Servo entryServo;
Servo exitServo;

// ---------------------------------------------------------------------------
// 4. PARKING CAPACITY / COUNT STATE
// ---------------------------------------------------------------------------
// Fixed maximum number of parking spaces available in the lot.
const int MAX_CAPACITY = 10;

// Number of vehicles currently parked. Starts at 0 (assume empty lot on
// power-up). This is the core piece of state the whole system revolves
// around: everything else (LCD text, barrier permission) is derived from it.
int carsParked = 0;

// ---------------------------------------------------------------------------
// 5. SIMPLE DEBOUNCE / STATE-TRACKING FOR EACH SENSOR
// ---------------------------------------------------------------------------
// A vehicle sitting in front of a sensor for multiple loop() cycles must
// only trigger ONE count change, not one per loop iteration. We do this by
// remembering whether the sensor was already "occupied" on the previous
// check, and only acting on the transition from "clear" to "detected".
// This is a simple two-state FSM (CLEAR / DETECTED) applied per sensor,
// similar in spirit to a debounce, but based on presence rather than
// rapid electrical bounce.

bool entryVehiclePresent = false; // was a vehicle at the entry last check?
bool exitVehiclePresent  = false; // was a vehicle at the exit last check?

// ---------------------------------------------------------------------------
// 6. SETUP
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(9600); // useful for debugging alongside the LCD

  pinMode(ENTRY_SENSOR_PIN, INPUT);
  pinMode(EXIT_SENSOR_PIN, INPUT);

  entryServo.attach(ENTRY_SERVO_PIN);
  exitServo.attach(EXIT_SERVO_PIN);

  // Make sure both gates start closed
  entryServo.write(GATE_CLOSED_ANGLE);
  exitServo.write(GATE_CLOSED_ANGLE);

  lcd.begin(16, 2); // 16 columns, 2 rows

  updateLCD(); // show initial availability on power-up
}

// ---------------------------------------------------------------------------
// 7. MAIN LOOP
// ---------------------------------------------------------------------------
void loop() {
  handleEntry();
  handleExit();
}

// ---------------------------------------------------------------------------
// 8. ENTRY HANDLING
// ---------------------------------------------------------------------------
void handleEntry() {
  bool detectedNow = (digitalRead(ENTRY_SENSOR_PIN) == SENSOR_DETECTED_STATE);

  // Only react on the transition from "no vehicle" to "vehicle detected".
  // This stops one car sitting at the sensor from incrementing the count
  // repeatedly while it waits for the barrier to lift and the car to pass.
  if (detectedNow && !entryVehiclePresent) {
    entryVehiclePresent = true; // remember: entry point is currently occupied

    if (carsParked < MAX_CAPACITY) {
      // Space is available -> let the vehicle in
      openGate(entryServo);
      delay(GATE_OPEN_DURATION_MS); // hold gate open for the vehicle to pass
      closeGate(entryServo);

      carsParked++; // increment exactly once per detected entry
      // Safety clamp: count should never exceed MAX_CAPACITY
      if (carsParked > MAX_CAPACITY) {
        carsParked = MAX_CAPACITY;
      }

      updateLCD();
    } else {
      // Parking is full -> keep gate closed and inform via LCD
      showParkingFull();
    }
  }

  // Reset the "present" flag once the vehicle has moved away from the sensor,
  // so the NEXT vehicle can be detected as a fresh transition.
  if (!detectedNow) {
    entryVehiclePresent = false;
  }
}

// ---------------------------------------------------------------------------
// 9. EXIT HANDLING
// ---------------------------------------------------------------------------
void handleExit() {
  bool detectedNow = (digitalRead(EXIT_SENSOR_PIN) == SENSOR_DETECTED_STATE);

  if (detectedNow && !exitVehiclePresent) {
    exitVehiclePresent = true;

    // Open the exit barrier to let the vehicle leave
    openGate(exitServo);
    delay(GATE_OPEN_DURATION_MS);
    closeGate(exitServo);

    carsParked--; // decrement exactly once per detected exit

    // Safety clamp: count should never go below zero
    if (carsParked < 0) {
      carsParked = 0;
    }

    updateLCD();
  }

  if (!detectedNow) {
    exitVehiclePresent = false;
  }
}

// ---------------------------------------------------------------------------
// 10. GATE HELPER FUNCTIONS
// ---------------------------------------------------------------------------
// Reusable so both the entry and exit gates share the same open/close logic.
void openGate(Servo &gateServo) {
  gateServo.write(GATE_OPEN_ANGLE);
}

void closeGate(Servo &gateServo) {
  gateServo.write(GATE_CLOSED_ANGLE);
}

// ---------------------------------------------------------------------------
// 11. LCD DISPLAY FUNCTIONS
// ---------------------------------------------------------------------------

// Shows current availability. Called whenever carsParked changes.
void updateLCD() {
  int availableSpaces = MAX_CAPACITY - carsParked;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Parking Status:");

  lcd.setCursor(0, 1);
  if (availableSpaces <= 0) {
    lcd.print("Parking Full");
  } else {
    lcd.print("Available: ");
    lcd.print(availableSpaces);
  }
}

// Shown specifically when a vehicle is waiting at the entry but the lot
// is already full, so the barrier is intentionally NOT opened.
void showParkingFull() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Parking Status:");
  lcd.setCursor(0, 1);
  lcd.print("Parking Full");
}
