#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

// Motor control pins
#define ENABLE_PIN 2
#define IN1_PIN 4
#define IN2_PIN 5

// PWM setup
#define PWM_CHANNEL 0
#define PWM_FREQ 1000
#define PWM_RESOLUTION 8  // 8-bit resolution (0-255)

Adafruit_BNO055 bno = Adafruit_BNO055(55);

// Constants
const float ANGLE_TOLERANCE = 0.5;         // Allowable error
const float MAX_PITCH = 80.0;              // Max soft pitch limit
const float MIN_PITCH = -80.0;             // Min soft pitch limit
const float ABS_PITCH_LIMIT = 82.0;        // Emergency hard limit

// PWM parameters (used as base, actual values are updated case-wise)
const int MAX_PWM = 102;
const int MIN_PWM = 60;
const int START_PWM = 100;

float targetPitch = 0;
float filteredPitch = 0;
float initialPitchOffset = 0;
float startPitch = 0;
bool targetReceived = false;

bool emergencyStop = false;
unsigned long emergencyStartTime = 0;
const unsigned long EMERGENCY_DURATION = 5000; // 5 seconds emergency stop

bool zeroPitchDetected = false;
unsigned long zeroPitchStartTime = 0;
const unsigned long ZERO_TIMEOUT = 1000; // stall detection timeout

const float FILTER_FACTOR = 0.2; // low-pass filter for pitch

void setup() {
  Serial.begin(115200);

  // Set motor direction pins
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  // Initialize PWM
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENABLE_PIN, PWM_CHANNEL);
  stopMotor(); // stop motor at start

  // Initialize BNO055 IMU
  Wire.begin(21, 22); // SDA, SCL
  if (!bno.begin()) {
    Serial.println("BNO055 not detected. Check wiring.");
    while (1);
  }

  delay(1000);
  bno.setExtCrystalUse(true); // use external crystal
  delay(100);
  
  // Capture initial orientation offset
  sensors_event_t event;
  bno.getEvent(&event);
  initialPitchOffset = -event.orientation.y;

  Serial.print("Initial Offset: ");
  Serial.println(initialPitchOffset, 2);
  Serial.println("Enter target pitch (-80 to 80):");
}

void loop() {
  sensors_event_t event;
  bno.getEvent(&event);
  
  // Compute raw and filtered pitch
  float rawPitch = event.orientation.y + initialPitchOffset;
  filteredPitch = filteredPitch * (1 - FILTER_FACTOR) + rawPitch * FILTER_FACTOR;

  // Handle new serial target pitch input
  if (Serial.available()) {
    handleTargetInput();
  }

  // Recover from emergency stop after duration
  if (emergencyStop && (millis() - emergencyStartTime >= EMERGENCY_DURATION)) {
    emergencyStop = false;
    Serial.println("Emergency stop deactivated after 5 seconds");
  }

  // If emergency is active, keep motor stopped and print pitch
  if (emergencyStop) {
    stopMotor();
    Serial.print("Emergency active | Raw: ");
    Serial.print(rawPitch, 2);
    Serial.print(" | Filtered: ");
    Serial.println(filteredPitch, 2);
    return;
  }

  // Emergency trigger if filtered pitch exceeds safety bounds
  if (!emergencyStop && (filteredPitch > ABS_PITCH_LIMIT || filteredPitch < -ABS_PITCH_LIMIT)) {
    Serial.println("\nEMERGENCY: Angle limit exceeded.");
    emergencyStop = true;
    emergencyStartTime = millis();
    stopMotor();
    return;
  }

  // Stall detection (motor stuck at pitch = 0)
  if (filteredPitch == 0.0f) {
    if (!zeroPitchDetected) {
      zeroPitchStartTime = millis();
      zeroPitchDetected = true;
    } else if (millis() - zeroPitchStartTime > ZERO_TIMEOUT) {
      Serial.println("\nSTALL DETECTED: ZERO PITCH TIMEOUT");
      emergencyStop = true;
      emergencyStartTime = millis();
      stopMotor();
      return;
    }
  } else {
    zeroPitchDetected = false;
  }

  // Print pitch info continuously
  Serial.print("Raw Pitch: ");
  Serial.print(rawPitch, 1);
  Serial.print("° | Filtered Pitch: ");
  Serial.print(filteredPitch, 1);
  Serial.print("°");

  if (targetReceived) {
    float error = targetPitch - filteredPitch;
    float absError = abs(error);

    // Stop if soft limits are reached
    if ((filteredPitch >= MAX_PITCH && error > 0) ||
        (filteredPitch <= MIN_PITCH && error < 0)) {
      stopMotor();
      Serial.println(" | Soft limit reached.");
      targetReceived = false;
      return;
    }

    // Stop if within tolerance
    if (absError <= ANGLE_TOLERANCE) {
      stopMotor();
      Serial.println(" | Target Reached.");
      Serial.println("Enter new target pitch (-80 to 80):");
      targetReceived = false;
      return;
    }

    // Set PWM parameters based on error range
    int max_pwm = 102, min_pwm = 60, start_pwm = 100;  // default values
    float totalError = abs(targetPitch - startPitch);
    float coveredError = abs(filteredPitch - startPitch);

    if (error > 15) {
      max_pwm = 100; min_pwm = 60; start_pwm = 90;
    } else if (error > 0 && error <= 15) {
      max_pwm = 92; min_pwm = 60; start_pwm = 90;
    } else if (error < 0 && error >= -15) {
      max_pwm = 102; min_pwm = 70; start_pwm = 100;
    } else if (error < -15) {
      max_pwm = 102; min_pwm = 70; start_pwm = 100;
    }

    int pwm = 0;
    if (totalError > ANGLE_TOLERANCE) {
      float progress = constrain(coveredError / totalError, 0.0, 1.0);

      // Ramp up/down PWM based on progress
      if (progress < 0.33f) {
        pwm = map(progress * 100, 0, 33, start_pwm, max_pwm);
      } else {
        pwm = map(progress * 100, 33, 100, max_pwm, min_pwm);
      }

      pwm = constrain(pwm, min_pwm, max_pwm);
    }

    // Print control info
    Serial.print(" | Target: ");
    Serial.print(targetPitch, 1);
    Serial.print("° | Error: ");
    Serial.print(error, 2);
    Serial.print("° | PWM: ");
    Serial.print(pwm);
    Serial.print(" | Phase: ");
    Serial.print(constrain(coveredError / totalError, 0.0, 1.0), 2);
    Serial.println();

    applyMotorOutput(pwm, error > 0); // move forward/backward based on error sign
  } else {
    stopMotor(); // idle condition
    Serial.println();
  }

  delay(10); // loop delay
}

// Apply motor direction and speed via PWM
void applyMotorOutput(int pwm, bool forward) {
  digitalWrite(IN1_PIN, forward ? HIGH : LOW);
  digitalWrite(IN2_PIN, forward ? LOW : HIGH);
  ledcWrite(PWM_CHANNEL, pwm);
}

// Stop motor completely
void stopMotor() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  ledcWrite(PWM_CHANNEL, 0);
}

// Read and validate target pitch from serial
void handleTargetInput() {
  float input = Serial.parseFloat();
  if (input > MAX_PITCH) {
    targetPitch = MAX_PITCH;
    Serial.println("Clamped to +80°");
  } else if (input < MIN_PITCH) {
    targetPitch = MIN_PITCH;
    Serial.println("Clamped to -80°");
  } else {
    targetPitch = input;
  }

  Serial.print("New Target Set: ");
  Serial.println(targetPitch);
  targetReceived = true;
  startPitch = filteredPitch; // store starting point
  stopMotor(); // stop before new movement
  while (Serial.available()) Serial.read(); // clear buffer
}