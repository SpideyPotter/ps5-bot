#include <ps5Controller.h>

// L298N Motor Driver Pins
// Motor A (Left Motor)
#define ENA 25  // PWM pin for Motor A speed
#define IN1 26  // Direction pin 1 for Motor A
#define IN2 27  // Direction pin 2 for Motor A

// Motor B (Right Motor)
#define ENB 32  // PWM pin for Motor B speed
#define IN3 33  // Direction pin 1 for Motor B
#define IN4 14  // Direction pin 2 for Motor B

// PWM Configuration
#define PWM_FREQ 1000
#define PWM_RESOLUTION 8
#define PWM_CHANNEL_A 0
#define PWM_CHANNEL_B 1


#define PS5_MAC "E8:47:3A:82:2D:58"

// The opposite end of the vehicle is the default front. Square toggles it.
bool invertedDrive = true;
bool squareWasPressed = false;
bool wasConnected = false;
bool squareReady = false;
int previousLeft = 0;
int previousRight = 0;

#include "drive_control.h"

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENA, PWM_CHANNEL_A);
  ledcAttachPin(ENB, PWM_CHANNEL_B);
  stopMotors();
  if (!ps5.begin(PS5_MAC)) {
    Serial.println("ERROR: PS5 Bluetooth initialization failed");
    while (true) { stopMotors(); delay(1000); }
  }
  Serial.println("Waiting for PS5. Default orientation: INVERTED");
  Serial.println("R2 forward, L2 reverse, stick turns in place, Square flips front");
}

void loop() {
  bool connected = ps5.isConnected();
  if (!connected) {
    stopMotors();
    if (wasConnected) Serial.println("PS5 disconnected: motors stopped");
    wasConnected = false;
    squareReady = false;
    squareWasPressed = false;
    delay(20);
    return;
  }
  if (!wasConnected) Serial.println("PS5 connected");
  wasConnected = true;

  bool squarePressed = ps5.Square();
  // On reconnect, require Square to be released before recognizing a press.
  if (!squarePressed) squareReady = true;
  if (squareReady && squarePressed && !squareWasPressed) {
    invertedDrive = !invertedDrive;
    Serial.printf("Driving orientation: %s\n", invertedDrive ? "INVERTED" : "ORIGINAL");
  }
  squareWasPressed = squarePressed;

  MotorCommand command = mixDrive(ps5.R2Value(), ps5.L2Value(),
                                  ps5.LStickX(), ps5.R1(), invertedDrive);
  setMotors(command.left, command.right);

  static unsigned long lastLog = 0;
  if (millis() - lastLog >= 250) {
    Serial.printf("%s: L=%d R=%d\n", invertedDrive ? "INVERTED" : "ORIGINAL",
                  command.left, command.right);
    lastLog = millis();
  }
  delay(20);
}

void writeMotor(int channel, int pin1, int pin2, int speed) {
  digitalWrite(pin1, speed > 0 ? HIGH : LOW);
  digitalWrite(pin2, speed < 0 ? HIGH : LOW);
  ledcWrite(channel, abs(speed));
}

void setMotors(int left, int right) {
  left = constrain(left, -255, 255);
  right = constrain(right, -255, 255);
  // Disable both bridges before changing direction, including orientation flips.
  bool reversing = (left * previousLeft < 0) || (right * previousRight < 0);
  if (reversing) {
    stopMotors();
    delay(20);
  }
  writeMotor(PWM_CHANNEL_A, IN1, IN2, left);
  writeMotor(PWM_CHANNEL_B, IN3, IN4, right);
  previousLeft = left;
  previousRight = right;
}

void stopMotors() {
  ledcWrite(PWM_CHANNEL_A, 0);
  ledcWrite(PWM_CHANNEL_B, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  previousLeft = 0;
  previousRight = 0;
}
