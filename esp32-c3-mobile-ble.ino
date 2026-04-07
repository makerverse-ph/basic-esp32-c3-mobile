/*
  ESP32-C3 Super Mini + TB6612FNG
  BLE robot control with startup movement test + dead-man timeout

  Commands:
    F = Forward
    B = Backward
    L = Turn Left
    R = Turn Right
    S = Stop
    V<number> = Set persistent speed (0..255), e.g. V120

  Behavior:
    - F/B/L/R start motion using the currently saved speed
    - Robot auto-stops if no fresh motion command is received
      within COMMAND_TIMEOUT_MS
    - Best used with a controller that keeps sending commands
      while the button is held
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// ---------------- PIN DEFINITIONS ----------------
static const int AIN1_PIN = 1;
static const int AIN2_PIN = 3;
static const int PWMA_PIN = 0;

static const int BIN1_PIN = 5;
static const int BIN2_PIN = 6;
static const int PWMB_PIN = 7;

// ---------------- SPEED SETTINGS ----------------
static const int DEFAULT_SPEED = 180;

// ---------------- MOTOR INVERSION ----------------
// Change these if motion is opposite of expected.
static const bool LEFT_MOTOR_INVERT  = true;
static const bool RIGHT_MOTOR_INVERT = true;

// ---------------- SAFETY TIMEOUT ----------------
// Robot will stop if no fresh motion command is received
// within this time window.
static const unsigned long COMMAND_TIMEOUT_MS = 250;

// ---------------- BLE UUIDS ----------------
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// ---------------- MOTION WATCHDOG ----------------
unsigned long lastMotionCommandTime = 0;
bool motorsAreRunning = false;

// ---------------- PERSISTENT SPEED ----------------
int currentSpeed = DEFAULT_SPEED;

// ------------------------------------------------
// MOTOR CONTROL ABSTRACTION
// speed range: -255 to +255
// positive = forward
// negative = backward
// ------------------------------------------------
void setLeftMotor(int speedValue) {
  speedValue = constrain(speedValue, -255, 255);

  bool forward = (speedValue >= 0);
  int pwm = abs(speedValue);

  if (LEFT_MOTOR_INVERT) {
    forward = !forward;
  }

  if (pwm == 0) {
    digitalWrite(AIN1_PIN, LOW);
    digitalWrite(AIN2_PIN, LOW);
    analogWrite(PWMA_PIN, 0);
    return;
  }

  digitalWrite(AIN1_PIN, forward ? HIGH : LOW);
  digitalWrite(AIN2_PIN, forward ? LOW : HIGH);
  analogWrite(PWMA_PIN, pwm);
}

void setRightMotor(int speedValue) {
  speedValue = constrain(speedValue, -255, 255);

  bool forward = (speedValue >= 0);
  int pwm = abs(speedValue);

  if (RIGHT_MOTOR_INVERT) {
    forward = !forward;
  }

  if (pwm == 0) {
    digitalWrite(BIN1_PIN, LOW);
    digitalWrite(BIN2_PIN, LOW);
    analogWrite(PWMB_PIN, 0);
    return;
  }

  digitalWrite(BIN1_PIN, forward ? HIGH : LOW);
  digitalWrite(BIN2_PIN, forward ? LOW : HIGH);
  analogWrite(PWMB_PIN, pwm);
}

void drive(int leftSpeed, int rightSpeed) {
  setLeftMotor(leftSpeed);
  setRightMotor(rightSpeed);

  motorsAreRunning = !((leftSpeed == 0) && (rightSpeed == 0));

  Serial.print("Drive L=");
  Serial.print(leftSpeed);
  Serial.print(" R=");
  Serial.println(rightSpeed);
}

void stopMotors() {
  drive(0, 0);
  Serial.println("STOP");
}

void moveForward(int speedValue = DEFAULT_SPEED) {
  drive(speedValue, speedValue);
  Serial.println("FORWARD");
}

void moveBackward(int speedValue = DEFAULT_SPEED) {
  drive(-speedValue, -speedValue);
  Serial.println("BACKWARD");
}

void moveLeft(int speedValue = DEFAULT_SPEED) {
  // left wheel backward, right wheel forward
  drive(-speedValue, speedValue);
  Serial.println("LEFT");
}

void moveRight(int speedValue = DEFAULT_SPEED) {
  // left wheel forward, right wheel backward
  drive(speedValue, -speedValue);
  Serial.println("RIGHT");
}

void refreshMotionWatchdog() {
  lastMotionCommandTime = millis();
}

void setPersistentSpeed(int speedValue) {
  currentSpeed = constrain(speedValue, 0, 255);
  Serial.print("Speed set to: ");
  Serial.println(currentSpeed);
}

// ---------------- BLE CALLBACKS ----------------
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    deviceConnected = true;
    Serial.println("BLE Connected");
  }

  void onDisconnect(BLEServer *server) override {
    deviceConnected = false;
    Serial.println("BLE Disconnected");
    stopMotors();
    BLEDevice::startAdvertising();
    Serial.println("Advertising restarted");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String cmd = characteristic->getValue().c_str();
    if (cmd.length() == 0) return;

    cmd.trim();
    cmd.toUpperCase();

    Serial.print("RX: ");
    Serial.println(cmd);

    if (cmd.startsWith("V")) {
      String speedText = cmd.substring(1);
      int speedValue = speedText.toInt();
      setPersistentSpeed(speedValue);

      String ack = "ACK:V" + String(currentSpeed);
      characteristic->setValue(ack.c_str());
      characteristic->notify();
      return;
    }

    char c = cmd[0];

    switch (c) {
      case 'F':
        moveForward(currentSpeed);
        refreshMotionWatchdog();
        break;

      case 'B':
        moveBackward(currentSpeed);
        refreshMotionWatchdog();
        break;

      case 'L':
        moveLeft(currentSpeed);
        refreshMotionWatchdog();
        break;

      case 'R':
        moveRight(currentSpeed);
        refreshMotionWatchdog();
        break;

      case 'S':
        stopMotors();
        break;

      default:
        Serial.println("Unknown command");
        return;
    }

    characteristic->setValue(("ACK:" + cmd).c_str());
    characteristic->notify();
  }
};

// ---------------- STARTUP TEST ----------------
void startupTest() {
  Serial.println("Starting motor test...");

  moveForward(170);
  delay(500);
  stopMotors();
  delay(250);

  moveBackward(170);
  delay(500);
  stopMotors();
  delay(250);

  moveLeft(170);
  delay(400);
  stopMotors();
  delay(250);

  moveRight(170);
  delay(400);
  stopMotors();
  delay(250);

  stopMotors();
  Serial.println("Motor test complete.");
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\nESP32-C3 Robot Starting...");

  pinMode(AIN1_PIN, OUTPUT);
  pinMode(AIN2_PIN, OUTPUT);
  pinMode(PWMA_PIN, OUTPUT);

  pinMode(BIN1_PIN, OUTPUT);
  pinMode(BIN2_PIN, OUTPUT);
  pinMode(PWMB_PIN, OUTPUT);

  stopMotors();

  startupTest();

  BLEDevice::init("ESP32C3_Robot");

  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new MyServerCallbacks());

  BLEService *svc = server->createService(SERVICE_UUID);

  pCharacteristic = svc->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_WRITE_NR
  );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("READY");

  svc->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();

  Serial.println("BLE Ready...");
  Serial.println("Device name: ESP32C3_Robot");
  Serial.println("Commands: F B L R S V<number>");
  Serial.print("Default speed: ");
  Serial.println(currentSpeed);
  Serial.print("Command timeout (ms): ");
  Serial.println(COMMAND_TIMEOUT_MS);

  lastMotionCommandTime = millis();
}

// ---------------- LOOP ----------------
void loop() {
  unsigned long now = millis();

  if (motorsAreRunning && (now - lastMotionCommandTime > COMMAND_TIMEOUT_MS)) {
    Serial.println("Command timeout reached, stopping motors");
    stopMotors();
  }

  delay(10);
}