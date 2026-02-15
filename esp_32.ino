#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <WebServer.h>
#include <Preferences.h>

WebServer localServer(80);
Preferences preferences;

// Pin Definitions
#define GARAGE_MOTOR_PIN_1  14
#define GARAGE_MOTOR_PIN_2  27
#define ULTRASONIC_TRIG     13
#define ULTRASONIC_ECHO     12
#define DOOR_REED_PIN       32
#define WINDOW_REED_PIN     33
#define DHTPIN              25
#define DHTTYPE             DHT11
#define COOLING_LED         26
#define HEATING_LED         15
#define SOLENOID_PIN        4
#define PWM_LED_PIN         21

// WiFi and Server
const char* ssid = "Licenta";
const char* password = "123456789";
const char* esp32CamIP = "192.168.94.55";

// Add these constants near the top of your file
const unsigned long SEND_RETRY_INTERVAL = 50;  // ms between HTTP attempts
const int PWM_CHANGE_THRESHOLD = 5;           // Min change to send update

// Auto close variables
bool autoCloseTriggered = false;
unsigned long autoCloseStartTime = 0;
const unsigned long AUTO_CLOSE_DELAY = 10000; // 10 seconds
bool lastCarPresent = false;


// System State
float currentTemp = 0.0;
int coolingThreshold;
int heatingThreshold;
bool garageOpen = false;
bool accessGranted = false;
bool carPresent = false;
bool doorOpen = false;
bool windowOpen = false;
int pwmValue;
const unsigned long sensorUpdateInterval = 1000; // Update sensors every 1000ms (1 second)

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  // Initialize preferences
  preferences.begin("smart-home", false);
  pwmValue = preferences.getInt("pwm", 128);
  coolingThreshold = preferences.getInt("coolThresh", 27);
  heatingThreshold = preferences.getInt("heatThresh", 22);
  accessGranted = false;
  preferences.putBool("access", false); // Reset on every startup

  // Initialize pins
  pinMode(GARAGE_MOTOR_PIN_1, OUTPUT);
  pinMode(GARAGE_MOTOR_PIN_2, OUTPUT);
  pinMode(COOLING_LED, OUTPUT);
  digitalWrite(COOLING_LED, HIGH);  // relay OFF by default
  pinMode(HEATING_LED, OUTPUT);
  pinMode(SOLENOID_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, accessGranted ? LOW : HIGH);

  pinMode(DOOR_REED_PIN, INPUT_PULLUP);
  pinMode(WINDOW_REED_PIN, INPUT_PULLUP);
  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);

  ledcAttach(PWM_LED_PIN, 5000, 8);
  ledcWrite(PWM_LED_PIN, pwmValue);

  dht.begin();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  // This line shows the IP address
  notifyCamAccess(false);

localServer.on("/cmd", HTTP_GET, []() {
    if (localServer.hasArg("cool") && localServer.hasArg("heat")) {
        coolingThreshold = localServer.arg("cool").toInt();
        heatingThreshold = localServer.arg("heat").toInt();
        preferences.putInt("coolThresh", coolingThreshold);
        preferences.putInt("heatThresh", heatingThreshold);
        Serial.println("Received thresholds - Cooling: " + String(coolingThreshold) + ", Heating: " + String(heatingThreshold));
    }

    if (localServer.hasArg("light")) {
        pwmValue = localServer.arg("light").toInt();
        preferences.putInt("pwm", pwmValue);
        setPWM(pwmValue);
        Serial.println("Received light PWM value: " + String(pwmValue));
    }

    if (localServer.hasArg("lock")) {
        Serial.println("Received LOCK command for door solenoid");
        setSolenoid(true); // Lock the door
    }

    if (localServer.hasArg("unlock")) {
        Serial.println("Received UNLOCK command for door solenoid");
        setSolenoid(false); // Unlock the door
    }

    if (localServer.hasArg("openGarage")) {
        Serial.println("Received OPEN GARAGE command");
        openGarage();
    }

    if (localServer.hasArg("closeGarage")) {
        Serial.println("Received CLOSE GARAGE command");
        closeGarage();
    }

    localServer.send(200, "text/plain", "OK");
});
localServer.begin();
}
// [Rest of your existing functions remain unchanged]
// (keep all your existing loop(), updateSensors(), handleTemperature(), etc.)

void loop() {
  checkSerialFingerprint();
  localServer.handleClient();
  updateSensors();
  if (autoCloseTriggered && (millis() - autoCloseStartTime >= AUTO_CLOSE_DELAY)) {
    Serial.println("Auto-close timer expired. Closing garage.");
    closeGarage();
    autoCloseTriggered = false;
  }
}

void updateSensors() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < sensorUpdateInterval) return;
  lastUpdate = millis();

  if (accessGranted) {
    handleTemperature();
    handleUltrasonicGarage();
  }

  doorOpen = (digitalRead(DOOR_REED_PIN) == HIGH);
  windowOpen = (digitalRead(WINDOW_REED_PIN) == HIGH);

  sendSensorDataToCam();
}

void handleTemperature() {
  float temp = dht.readTemperature();
  if (!isnan(temp)) {
    currentTemp = temp;
    digitalWrite(COOLING_LED, currentTemp >= coolingThreshold ? LOW : HIGH);
    digitalWrite(HEATING_LED, currentTemp <= heatingThreshold ? HIGH : LOW);
  }
}

void handleUltrasonicGarage() {
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG, LOW);

  long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);
  int distance = duration * 0.034 / 2;

  bool previousCarPresent = carPresent;
  carPresent = (distance > 0 && distance < 15);

  // If car presence changed and garage is open, start auto-close timer
  if (garageOpen && carPresent != previousCarPresent && !autoCloseTriggered) {
    Serial.println("Car presence changed while garage is open. Starting auto-close countdown.");
    autoCloseStartTime = millis();
    autoCloseTriggered = true;
  }
}


void openGarage() {
    Serial.println("Opening garage door...");
    digitalWrite(GARAGE_MOTOR_PIN_1, HIGH);
    digitalWrite(GARAGE_MOTOR_PIN_2, LOW);
    delay(1500); // Adjust this delay based on your garage door operation time
    digitalWrite(GARAGE_MOTOR_PIN_1, LOW);
    digitalWrite(GARAGE_MOTOR_PIN_2, LOW);
    garageOpen = true;
    Serial.println("Garage door opened");
}

void closeGarage() {
    Serial.println("Closing garage door...");
    digitalWrite(GARAGE_MOTOR_PIN_1, LOW);
    digitalWrite(GARAGE_MOTOR_PIN_2, HIGH);
    delay(1450); // Adjust this delay based on your garage door operation time
    digitalWrite(GARAGE_MOTOR_PIN_1, LOW);
    digitalWrite(GARAGE_MOTOR_PIN_2, LOW);
    garageOpen = false;
    Serial.println("Garage door closed");
    autoCloseTriggered = false;
}

void checkSerialFingerprint() {
  while (Serial2.available()) {
    String msg = Serial2.readStringUntil('\n');
    msg.trim();
    if (msg == "GRANTED" && !accessGranted) {
      accessGranted = true;
      setSolenoid(true);
      notifyCamAccess(true);
    } else if (msg == "DENIED") {
      Serial.println("Access denied. Ignoring...");
      // Do NOT revoke access.
    }
  }
}

void notifyCamAccess(bool access) {
  HTTPClient http;
  http.begin("http://" + String(esp32CamIP) + "/auth?access=" + (access ? "1" : "0"));
  http.GET();
  http.end();
}

void sendSensorDataToCam() {
  HTTPClient http;
  String url = "http://" + String(esp32CamIP) + "/update?";
  url += "door=" + String(doorOpen ? "open" : "closed");
  url += "&window=" + String(windowOpen ? "open" : "closed");
  url += "&garage=" + String(garageOpen ? "open" : "closed");
  url += "&temp=" + (isnan(currentTemp) ? "nan" : String(currentTemp, 1));
  url += "&car=" + String(carPresent ? "1" : "0");
  url += "&pwm=" + String(pwmValue);  // Add current PWM value
  
  http.begin(url);
  http.setTimeout(200);  // Reduce timeout
  http.addHeader("Cache-Control", "no-cache");
  http.GET();
  http.end();
}

void setPWM(int value) {
  pwmValue = constrain(value, 0, 255);  // Ensure value is within valid range
  preferences.putInt("pwm", pwmValue);  // Save to preferences
  ledcWrite(PWM_LED_PIN, pwmValue);
  Serial.println("Setting PWM to: " + String(pwmValue));
  sendSensorDataToCam();
}


void setSolenoid(bool unlocked) {
  digitalWrite(SOLENOID_PIN, unlocked ? HIGH : LOW);
}

void setThresholds(int cooling, int heating) {
  coolingThreshold = cooling;
  heatingThreshold = heating;
}