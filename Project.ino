#include <WiFi.h>
#include <WebServer.h>
#include <TinyGPS++.h>

// Access Point Settings
const char* ap_ssid = "ZephyroChair";
const char* ap_password = "wheelchair123";

WebServer server(80);

// Motor Pins
#define MOTOR_PWM_A 33
#define MOTOR_PWM_B 32
#define MOTOR_DIR_A 26  
#define MOTOR_DIR_B 25

// Motor Control Settings
int MAX_PWM = 180;
int currentSpeedA = 0;
int currentSpeedB = 0;
unsigned long lastCommandTime = 0;
char currentCommand = 'S';

// Web Server Handlers
void handleRoot() {
 String html = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ZephyroChair Control Panel</title>
    <style>
         :root {
            --primary: #4a6fa5;
            --secondary: #166088;
            --accent: #4fc3f7;
            --danger: #f44336;
            --success: #4caf50;
            --light: #f8f9fa;
            --dark: #212529;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #f5f5f5;
            margin: 0;
            padding: 0;
            display: flex;
            flex-direction: column;
            min-height: 100vh;
            color: var(--dark);
        }
        
        header {
            background: linear-gradient(135deg, var(--primary), var(--secondary));
            color: white;
            padding: 1rem;
            text-align: center;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
        }
        
        .container {
            display: flex;
            flex-direction: column;
            align-items: center;
            padding: 2rem;
            flex: 1;
        }
        
        .control-panel {
            background-color: white;
            border-radius: 15px;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
            padding: 2rem;
            width: 100%;
            max-width: 800px;
            margin-bottom: 2rem;
        }
        
        .panel-title {
            color: var(--secondary);
            margin-top: 0;
            border-bottom: 2px solid var(--accent);
            padding-bottom: 0.5rem;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        .joystick-container {
            display: flex;
            flex-direction: column;
            align-items: center;
            margin: 2rem 0;
        }
        
        .joystick {
            width: 200px;
            height: 200px;
            border-radius: 50%;
            background-color: #e9ecef;
            position: relative;
            box-shadow: inset 0 0 15px rgba(0, 0, 0, 0.1);
            margin-bottom: 1rem;
            touch-action: none;
        }
        
        .joystick-knob {
            width: 60px;
            height: 60px;
            border-radius: 50%;
            background: linear-gradient(135deg, var(--primary), var(--secondary));
            position: absolute;
            top: 70px;
            left: 70px;
            box-shadow: 0 4px 10px rgba(0, 0, 0, 0.2);
            cursor: pointer;
        }
        
        .emergency-stop {
            background-color: var(--danger);
            color: white;
            border: none;
            border-radius: 50px;
            padding: 1rem 2rem;
            font-size: 1.2rem;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s;
            box-shadow: 0 4px 10px rgba(244, 67, 54, 0.3);
            margin: 1rem 0;
            width: 100%;
            max-width: 300px;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        
        .emergency-stop:hover {
            background-color: #d32f2f;
            transform: translateY(-2px);
            box-shadow: 0 6px 15px rgba(244, 67, 54, 0.4);
        }
        
        .emergency-stop:active {
            transform: translateY(0);
            box-shadow: 0 2px 5px rgba(244, 67, 54, 0.4);
        }
        
        .status-panel {
            background-color: white;
            border-radius: 15px;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
            padding: 1.5rem;
            width: 100%;
            max-width: 800px;
            margin-bottom: 2rem;
        }
        
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 1rem;
        }
        
        .status-item {
            background-color: #f8f9fa;
            border-radius: 10px;
            padding: 1rem;
            text-align: center;
        }
        
        .status-label {
            font-size: 0.9rem;
            color: #6c757d;
            margin-bottom: 0.5rem;
        }
        
        .status-value {
            font-size: 1.5rem;
            font-weight: bold;
            color: var(--secondary);
        }
        
        .speed-control {
            width: 100%;
            margin: 1.5rem 0;
        }
        
        .speed-slider {
            width: 100%;
            height: 10px;
            -webkit-appearance: none;
            appearance: none;
            background: #e9ecef;
            outline: none;
            border-radius: 5px;
            margin: 1rem 0;
        }
        
        .speed-slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 25px;
            height: 25px;
            border-radius: 50%;
            background: var(--primary);
            cursor: pointer;
            transition: all 0.2s;
        }
        
        .speed-slider::-webkit-slider-thumb:hover {
            background: var(--secondary);
            transform: scale(1.1);
        }
        
        .speed-labels {
            display: flex;
            justify-content: space-between;
            margin-top: -0.5rem;
        }
        
        footer {
            background-color: var(--dark);
            color: white;
            text-align: center;
            padding: 1rem;
            font-size: 0.9rem;
        }
        
        @media (max-width: 600px) {
            .control-panel, .status-panel {
                padding: 1rem;
            }
            
            .joystick {
                width: 150px;
                height: 150px;
            }
            
            .joystick-knob {
                width: 50px;
                height: 50px;
                top: 50px;
                left: 50px;
            }
        }

    </style>
</head>
<body>
    <header>
        <h1>ZephyroChair Control Panel</h1>
    </header>
    
    <div class="container">
        <div class="control-panel">
            <h2 class="panel-title">Manual Control</h2>
            
            <div class="joystick-container">
                <div class="joystick" id="joystick">
                    <div class="joystick-knob" id="joystick-knob"></div>
                </div>
                <button class="emergency-stop" id="emergencyStop">Emergency Stop</button>
            </div>
            
            <div class="speed-control">
                <h3>Speed Control</h3>
                <input type="range" min="50" max="255" value="180" class="speed-slider" id="speedSlider">
                <div class="speed-labels">
                    <span>Slow</span>
                    <span>Medium</span>
                    <span>Fast</span>
                </div>
            </div>
        </div>
        
        <div class="status-panel">
            <h2 class="panel-title">Chair Status</h2>
            <div class="status-grid">
                <div class="status-item">
                    <div class="status-label">Current Speed</div>
                    <div class="status-value" id="currentSpeed">0%</div>
                </div>
                <div class="status-item">
                    <div class="status-label">Left Motor</div>
                    <div class="status-value" id="leftMotor">0</div>
                </div>
                <div class="status-item">
                    <div class="status-label">Right Motor</div>
                    <div class="status-value" id="rightMotor">0</div>
                </div>
                <div class="status-item">
                    <div class="status-label">Connection</div>
                    <div class="status-value" id="connectionStatus">Connected</div>
                </div>
            </div>
        </div>
    </div>
    
    <footer>
        <p>ZephyroChair Wheelchair Control System &copy; 2023</p>
    </footer>

    <script>
    // DOM Elements
    const joystick = document.getElementById('joystick');
    const joystickKnob = document.getElementById('joystick-knob');
    const emergencyStop = document.getElementById('emergencyStop');
    const speedSlider = document.getElementById('speedSlider');
    
    // Variables
    let isDragging = false;
    let maxPWM = 180;
    let lastDirection = '';
    let lastSentDirection = '';
    let sendInterval = null;
    
    // Initialize
    document.addEventListener('DOMContentLoaded', function() {
        setupJoystick();
        setupEventListeners();
    });
    
    // Joystick Setup
    function setupJoystick() {
        // Mouse/Touch events
        joystickKnob.addEventListener('mousedown', startDrag);
        joystickKnob.addEventListener('touchstart', startDrag);
        
        document.addEventListener('mousemove', drag);
        document.addEventListener('touchmove', drag);
        
        document.addEventListener('mouseup', stopDrag);
        document.addEventListener('touchend', stopDrag);
        
        function startDrag(e) {
            e.preventDefault();
            isDragging = true;
            startSendingCommands();
        }
        
        function drag(e) {
            if (!isDragging) return;
            e.preventDefault();
            
            const clientX = e.clientX || e.touches[0].clientX;
            const clientY = e.clientY || e.touches[0].clientY;
            
            const joystickRect = joystick.getBoundingClientRect();
            const centerX = joystickRect.left + joystickRect.width / 2;
            const centerY = joystickRect.top + joystickRect.height / 2;
            
            let x = clientX - centerX;
            let y = clientY - centerY;
            
            // Limit to joystick circle
            const distance = Math.sqrt(x * x + y * y);
            const radius = joystickRect.width / 2 - 30;
            
            if (distance > radius) {
                x = (x / distance) * radius;
                y = (y / distance) * radius;
            }
            
            // Position the knob
            joystickKnob.style.transform = `translate(${x}px, ${y}px)`;
            
            // Calculate direction
            const angle = Math.atan2(y, x);
            const degrees = (angle * 180 / Math.PI + 360) % 360;
            
            // Determine direction
            if (distance > 20) { // Deadzone threshold
                if (degrees > 315 || degrees <= 45) {
                    lastDirection = 'R'; // Right
                } else if (degrees > 45 && degrees <= 135) {
                    lastDirection = 'F'; // Forward
                } else if (degrees > 135 && degrees <= 225) {
                    lastDirection = 'L'; // Left
                } else {
                    lastDirection = 'B'; // Backward
                }
            } else {
                lastDirection = 'S'; // Stop
            }
        }
        
        function stopDrag() {
            if (!isDragging) return;
            isDragging = false;
            
            // Return knob to center
            joystickKnob.style.transform = 'translate(0, 0)';
            lastDirection = 'S';
            stopSendingCommands();
        }
        
        function startSendingCommands() {
            // Clear any existing interval
            if (sendInterval) clearInterval(sendInterval);
            
            // Send command immediately when direction changes
            sendInterval = setInterval(() => {
                if (lastDirection !== lastSentDirection) {
                    sendCommand(lastDirection);
                    lastSentDirection = lastDirection;
                }
            }, 50); // Check for direction changes every 50ms
        }
        
        function stopSendingCommands() {
            if (sendInterval) {
                clearInterval(sendInterval);
                sendInterval = null;
            }
            sendCommand('S');
            lastSentDirection = 'S';
        }
    }
    
    // Event Listeners
    function setupEventListeners() {
        // Emergency Stop
        emergencyStop.addEventListener('click', function() {
            stopSendingCommands();
            sendCommand('S');
        });
        
        // Speed Slider
        speedSlider.addEventListener('input', function() {
            maxPWM = parseInt(this.value);
            fetch('/speed?value=' + maxPWM);
        });
    }
    
    // Send command to server
    function sendCommand(cmd) {
        fetch('/control?cmd=' + cmd)
            .catch(error => console.error('Error:', error));
    }
</script>
</body>
</html>
)=====";
  server.send(200, "text/html", html);

}

void handleControl() {
  if (server.hasArg("cmd")) {
    currentCommand = server.arg("cmd").charAt(0);
    lastCommandTime = millis();
  }
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("value")) {
    MAX_PWM = server.arg("value").toInt();
  }
  server.send(200, "text/plain", "OK");
}

void executeCommand(char command) {
  switch (command) {
    case 'F':
      digitalWrite(MOTOR_DIR_A, HIGH);
      digitalWrite(MOTOR_DIR_B, HIGH);
      analogWrite(MOTOR_PWM_A, MAX_PWM);
      analogWrite(MOTOR_PWM_B, MAX_PWM);
      currentSpeedA = MAX_PWM;
      currentSpeedB = MAX_PWM;
      break;
    case 'B':
      digitalWrite(MOTOR_DIR_A, LOW);
      digitalWrite(MOTOR_DIR_B, LOW);
      analogWrite(MOTOR_PWM_A, MAX_PWM);
      analogWrite(MOTOR_PWM_B, MAX_PWM);
      currentSpeedA = MAX_PWM;
      currentSpeedB = MAX_PWM;
      break;
    case 'L':
      digitalWrite(MOTOR_DIR_A, HIGH);
      digitalWrite(MOTOR_DIR_B, HIGH);
      analogWrite(MOTOR_PWM_A, MAX_PWM * 0.3);
      analogWrite(MOTOR_PWM_B, MAX_PWM);
      currentSpeedA = MAX_PWM * 0.3;
      currentSpeedB = MAX_PWM;
      break;
    case 'R':
      digitalWrite(MOTOR_DIR_A, HIGH);
      digitalWrite(MOTOR_DIR_B, HIGH);
      analogWrite(MOTOR_PWM_A, MAX_PWM);
      analogWrite(MOTOR_PWM_B, MAX_PWM * 0.3);
      currentSpeedA = MAX_PWM;
      currentSpeedB = MAX_PWM * 0.3;
      break;
    case 'S':
      analogWrite(MOTOR_PWM_A, 0);
      analogWrite(MOTOR_PWM_B, 0);
      currentSpeedA = 0;
      currentSpeedB = 0;
      break;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize motor pins
  pinMode(MOTOR_PWM_A, OUTPUT); pinMode(MOTOR_PWM_B, OUTPUT);
  pinMode(MOTOR_DIR_A, OUTPUT); pinMode(MOTOR_DIR_B, OUTPUT);
  
  // Start WiFi AP
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("AP IP address: "); Serial.println(WiFi.softAPIP());
  
  // Setup server handlers
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.on("/speed", handleSpeed);
  server.begin();
}

void loop() {
  server.handleClient();
  
  // Check if we haven't received a command in 200ms (timeout)
  if (millis() - lastCommandTime > 200) {
    currentCommand = 'S'; // Auto-stop if no recent commands
  }
  
  // Execute the current command
  executeCommand(currentCommand);
  
  delay(10); // Small delay to prevent watchdog timer issues
}