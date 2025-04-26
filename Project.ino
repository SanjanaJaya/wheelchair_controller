#include <WiFi.h>
#include <WebServer.h>
#include <TinyGPS++.h>

// ─── Access Point Settings ───────────────────────────────
const char* ap_ssid = "ZephyroChair";
const char* ap_password = "wheelchair123";

WebServer server(80);

// ─── Motor Pins ──────────────────────────────────────────
#define MOTOR_PWM_A 33
#define MOTOR_PWM_B 32
#define MOTOR_DIR_A 26  
#define MOTOR_DIR_B 25

// ─── Ultrasonic Sensor Pins ──────────────────────────────
#define TRIG_PIN_FRONT_LEFT 14
#define ECHO_PIN_FRONT_LEFT 27
#define TRIG_PIN_FRONT_RIGHT 12
#define ECHO_PIN_FRONT_RIGHT 13
#define TRIG_PIN_BACK_LEFT 5
#define ECHO_PIN_BACK_LEFT 18
#define TRIG_PIN_BACK_RIGHT 4
#define ECHO_PIN_BACK_RIGHT 19

#define OBSTACLE_DISTANCE 20  // cm

// ─── Joystick Pins ───────────────────────────────────────
#define JOY_X 34
#define JOY_Y 35
#define JOY_CENTER 2048
#define JOY_DEADZONE 300

// ─── GPS Pins ────────────────────────────────────────────
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17

// ─── Motor Control Settings ──────────────────────────────
int MAX_PWM = 180;
#define STEP_DELAY  20
#define STEP_SIZE   15

int currentSpeedA = 0;
int currentSpeedB = 0;

// ─── GPS Setup ───────────────────────────────────────────
TinyGPSPlus gps;
HardwareSerial mySerial(1);

// ─── Web Server Handlers ─────────────────────────────────
void handleRoot() {
  String html = R"=====( ... your HTML page here ... )=====";
  server.send(200, "text/html", html);
}

void handleControl() {
  if (server.hasArg("cmd")) {
    char command = server.arg("cmd").charAt(0);
    Serial.print("🌐 Web Command: "); Serial.println(command);
    
    switch (command) {
      case 'F':
        digitalWrite(MOTOR_DIR_A, HIGH);
        digitalWrite(MOTOR_DIR_B, HIGH);
        gradualSpeedChange(MAX_PWM, MAX_PWM);
        break;
      case 'B':
        digitalWrite(MOTOR_DIR_A, LOW);
        digitalWrite(MOTOR_DIR_B, LOW);  // 🔥 fixed: both LOW for backward
        gradualSpeedChange(MAX_PWM, MAX_PWM);
        break;
      case 'L':
        digitalWrite(MOTOR_DIR_A, HIGH);
        digitalWrite(MOTOR_DIR_B, HIGH);
        gradualSpeedChange(0, MAX_PWM);
        break;
      case 'R':
        digitalWrite(MOTOR_DIR_A, HIGH);
        digitalWrite(MOTOR_DIR_B, HIGH);
        gradualSpeedChange(MAX_PWM, 0);
        break;
      case 'S':
        smoothStop();
        break;
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("value")) {
    MAX_PWM = server.arg("value").toInt();
    Serial.print("⚡ Speed set to: "); Serial.println(MAX_PWM);
  }
  server.send(200, "text/plain", "OK");
}

// ─── Distance Reading ───────────────────────────────────
long readDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}

bool isObstacleDetected() {
  long dFL = readDistanceCM(TRIG_PIN_FRONT_LEFT, ECHO_PIN_FRONT_LEFT);
  long dFR = readDistanceCM(TRIG_PIN_FRONT_RIGHT, ECHO_PIN_FRONT_RIGHT);
  long dBL = readDistanceCM(TRIG_PIN_BACK_LEFT, ECHO_PIN_BACK_LEFT);
  long dBR = readDistanceCM(TRIG_PIN_BACK_RIGHT, ECHO_PIN_BACK_RIGHT);

  Serial.print("FL: "); Serial.print(dFL); Serial.print(" | ");
  Serial.print("FR: "); Serial.print(dFR); Serial.print(" | ");
  Serial.print("BL: "); Serial.print(dBL); Serial.print(" | ");
  Serial.print("BR: "); Serial.println(dBR);

  return (dFL < OBSTACLE_DISTANCE || dFR < OBSTACLE_DISTANCE ||
          dBL < OBSTACLE_DISTANCE || dBR < OBSTACLE_DISTANCE);
}

// ─── Smooth Control ─────────────────────────────────────
void smoothStop() {
  gradualSpeedChange(0, 0);
  Serial.println("🛑 Smooth Stop Complete.");
}

void gradualSpeedChange(int targetA, int targetB) {
  while (currentSpeedA != targetA || currentSpeedB != targetB) {
    currentSpeedA = (currentSpeedA < targetA) ? min(currentSpeedA + STEP_SIZE, targetA) : max(currentSpeedA - STEP_SIZE, targetA);
    currentSpeedB = (currentSpeedB < targetB) ? min(currentSpeedB + STEP_SIZE, targetB) : max(currentSpeedB - STEP_SIZE, targetB);
    analogWrite(MOTOR_PWM_A, currentSpeedA);
    analogWrite(MOTOR_PWM_B, currentSpeedB);
    delay(STEP_DELAY);
  }
}

// ─── Status API for Web Interface ───────────────────────
void handleStatus() {
  char statusJson[200];
  sprintf(statusJson, "{\"speed\":%d,\"motorA\":%d,\"motorB\":%d}", 
    MAX_PWM, currentSpeedA, currentSpeedB);
  server.send(200, "application/json", statusJson);
}

// ─── Setup ──────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("🚀 ZephyroChair Initializing...");

  pinMode(MOTOR_PWM_A, OUTPUT); pinMode(MOTOR_PWM_B, OUTPUT);
  pinMode(MOTOR_DIR_A, OUTPUT); pinMode(MOTOR_DIR_B, OUTPUT);

  pinMode(TRIG_PIN_FRONT_LEFT, OUTPUT); pinMode(ECHO_PIN_FRONT_LEFT, INPUT);
  pinMode(TRIG_PIN_FRONT_RIGHT, OUTPUT); pinMode(ECHO_PIN_FRONT_RIGHT, INPUT);
  pinMode(TRIG_PIN_BACK_LEFT, OUTPUT); pinMode(ECHO_PIN_BACK_LEFT, INPUT);
  pinMode(TRIG_PIN_BACK_RIGHT, OUTPUT); pinMode(ECHO_PIN_BACK_RIGHT, INPUT);

  analogReadResolution(12);
  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);

  mySerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("📍 GPS initialized");

  analogWrite(MOTOR_PWM_A, 0);
  analogWrite(MOTOR_PWM_B, 0);
  digitalWrite(MOTOR_DIR_A, LOW);
  digitalWrite(MOTOR_DIR_B, LOW);

  WiFi.softAP(ap_ssid, ap_password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("📶 Access Point started: "); Serial.println(ap_ssid);
  Serial.print("📱 AP IP address: "); Serial.println(myIP);

  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.on("/speed", handleSpeed);
  server.on("/status", handleStatus);
  server.begin();
  Serial.println("🌐 HTTP server started");
}

// ─── Loop ───────────────────────────────────────────────
void loop() {
  server.handleClient();

  int xValue = analogRead(JOY_X);
  int yValue = analogRead(JOY_Y);
  bool obstacle = isObstacleDetected();

  int targetSpeedA = 0;
  int targetSpeedB = 0;

  bool joystickActive = (abs(xValue - JOY_CENTER) > JOY_DEADZONE || 
                         abs(yValue - JOY_CENTER) > JOY_DEADZONE);

  if (joystickActive && !obstacle) {
    if (yValue > JOY_CENTER + JOY_DEADZONE) {
      digitalWrite(MOTOR_DIR_A, HIGH);
      digitalWrite(MOTOR_DIR_B, HIGH);
      int pwm = map(yValue, JOY_CENTER + JOY_DEADZONE, 4095, 0, MAX_PWM);
      targetSpeedA = targetSpeedB = pwm;
      Serial.println("🕹 Joystick: Forward");
    } 
    else if (yValue < JOY_CENTER - JOY_DEADZONE) {
      digitalWrite(MOTOR_DIR_A, LOW);
      digitalWrite(MOTOR_DIR_B, LOW); // 🔥 fixed: both LOW for backward
      int pwm = map(yValue, 0, JOY_CENTER - JOY_DEADZONE, MAX_PWM, 0);
      targetSpeedA = targetSpeedB = pwm;
      Serial.println("🕹 Joystick: Backward");
    } 
    else if (xValue > JOY_CENTER + JOY_DEADZONE) {
      digitalWrite(MOTOR_DIR_A, HIGH);
      digitalWrite(MOTOR_DIR_B, HIGH);
      targetSpeedA = MAX_PWM; 
      targetSpeedB = 0;
      Serial.println("🕹 Joystick: Right");
    } 
    else if (xValue < JOY_CENTER - JOY_DEADZONE) {
      digitalWrite(MOTOR_DIR_A, HIGH);
      digitalWrite(MOTOR_DIR_B, HIGH);
      targetSpeedA = 0; 
      targetSpeedB = MAX_PWM;
      Serial.println("🕹 Joystick: Left");
    }

    gradualSpeedChange(targetSpeedA, targetSpeedB);
  } 
  else if (!joystickActive && targetSpeedA == 0 && targetSpeedB == 0) {
    if (currentSpeedA > 0 || currentSpeedB > 0) {
      smoothStop();
    }
  }

  if (obstacle && (currentSpeedA > 0 || currentSpeedB > 0)) {
    Serial.println("🚧 Obstacle detected! Smooth stopping...");
    smoothStop();
  }

  while (mySerial.available()) {
    gps.encode(mySerial.read());
    if (gps.location.isUpdated()) {
      float latitude = gps.location.lat();
      float longitude = gps.location.lng();
      Serial.print("📍 GPS: "); Serial.print(latitude, 6);
      Serial.print(", "); Serial.println(longitude, 6);
    }
  }

  delay(100);
}