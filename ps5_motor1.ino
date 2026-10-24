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

// Motor control variables
int leftMotorSpeed = 0;
int rightMotorSpeed = 0;

void setup() {
  Serial.begin(115200);
  
  // Configure motor control pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // Setup PWM channels
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENA, PWM_CHANNEL_A);
  ledcAttachPin(ENB, PWM_CHANNEL_B);
  
  // Initialize motors to stopped
  stopMotors();
  
  // Initialize PS5 controller
  ps5.begin(PS5_MAC);
  Serial.println("Waiting for PS5 controller to connect...");
  Serial.println("Press PS button to pair");
}

void loop() {
  if (ps5.isConnected()) {
    // Read controller inputs
    int l2Value = ps5.L2Value();      // 0-255
    int r2Value = ps5.R2Value();      // 0-255
    int leftX = ps5.LStickX();        // -128 to 127 (left stick X for turning)
    bool r1Pressed = ps5.R1();        // R1 button for full thrust
    
    // Calculate motor speeds
    if (r1Pressed) {
      // R1 pressed: Full thrust forward
      leftMotorSpeed = 255;
      rightMotorSpeed = 255;
      moveForward(leftMotorSpeed, rightMotorSpeed);
      Serial.println("R1: FULL THRUST FORWARD");
    }
    else {
      // Normal operation with L2/R2 triggers
      int forwardSpeed = r2Value;   // R2 for forward
      int reverseSpeed = l2Value;   // L2 for reverse
      
      // Apply turning from left analog stick
      int turnValue = map(leftX, -128, 127, -100, 100);
      
      if (forwardSpeed > 10) {
        // Moving forward with R2
        leftMotorSpeed = forwardSpeed;
        rightMotorSpeed = forwardSpeed;
        
        // Apply differential turning
        if (turnValue > 10) {  // Turning right
          rightMotorSpeed = forwardSpeed - (forwardSpeed * turnValue / 100);
        }
        else if (turnValue < -10) {  // Turning left
          leftMotorSpeed = forwardSpeed + (forwardSpeed * turnValue / 100);
        }
        
        moveForward(leftMotorSpeed, rightMotorSpeed);
        Serial.printf("Forward: L=%d R=%d Turn=%d\n", leftMotorSpeed, rightMotorSpeed, turnValue);
      }
      else if (reverseSpeed > 10) {
        // Moving backward with L2
        leftMotorSpeed = reverseSpeed;
        rightMotorSpeed = reverseSpeed;
        
        // Apply differential turning (reversed)
        if (turnValue > 10) {  // Turning right while reversing
          rightMotorSpeed = reverseSpeed - (reverseSpeed * turnValue / 100);
        }
        else if (turnValue < -10) {  // Turning left while reversing
          leftMotorSpeed = reverseSpeed + (reverseSpeed * turnValue / 100);
        }
        
        moveBackward(leftMotorSpeed, rightMotorSpeed);
        Serial.printf("Reverse: L=%d R=%d Turn=%d\n", leftMotorSpeed, rightMotorSpeed, turnValue);
      }
      else {
        // No trigger pressed, stop motors
        stopMotors();
      }
    }
    
    delay(50);  // Small delay for stability
  }
  else {
    // Controller not connected, stop motors
    stopMotors();
    delay(1000);
  }
}

void moveForward(int leftSpeed, int rightSpeed) {
  // Motor A forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  ledcWrite(PWM_CHANNEL_A, leftSpeed);
  
  // Motor B forward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  ledcWrite(PWM_CHANNEL_B, rightSpeed);
}

void moveBackward(int leftSpeed, int rightSpeed) {
  // Motor A backward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  ledcWrite(PWM_CHANNEL_A, leftSpeed);
  
  // Motor B backward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  ledcWrite(PWM_CHANNEL_B, rightSpeed);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  ledcWrite(PWM_CHANNEL_A, 0);
  ledcWrite(PWM_CHANNEL_B, 0);
}