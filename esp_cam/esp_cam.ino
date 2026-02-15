#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Preferences.h>


#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

void handleRoot();
void handleAuth();
void handleUpdate();
void handleLight();
void handleThresholds();
void handleCameraOn();
void handleCameraOff();
void handleStatus();
void handleStream();
void sendCommandToESP32(const String& path);

const char* ssid = "Licenta";
const char* password = "123456789";
const char* esp32Ip = "192.168.94.82";

WebServer server(80);
Preferences preferences;


// System State
bool accessGranted = false;
bool cameraActive = false;
String doorStatus = "unknown";
String windowStatus = "unknown";
String garageStatus = "unknown";
String temperature = "unknown";
String carPresent = "0";
int pwmValue;
int tempLow;
int tempHigh;

void setup() {
  Serial.begin(115200);

    // Initialize preferences
  preferences.begin("smart-home", false);
  pwmValue = preferences.getInt("pwm", 128);
  tempLow = preferences.getInt("tempLow", 18);
  tempHigh = preferences.getInt("tempHigh", 26);
  accessGranted = false;
  preferences.putBool("access", false);
  
  // Camera config
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  // This line shows the IP address
  // Server routes
  // Server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/auth", HTTP_GET, handleAuth);
  server.on("/update", HTTP_GET, handleUpdate);
  server.on("/lock", HTTP_GET, []() {
    Serial.println("Sending LOCK command to main ESP32");
    sendCommandToESP32("/cmd?lock=1");
    server.send(200, "text/plain", "OK");
});

server.on("/unlock", HTTP_GET, []() {
    Serial.println("Sending UNLOCK command to main ESP32");
    sendCommandToESP32("/cmd?unlock=1");
    server.send(200, "text/plain", "OK");
});

server.on("/openGarage", HTTP_GET, []() {
    Serial.println("Sending OPEN GARAGE command to main ESP32");
    sendCommandToESP32("/cmd?openGarage=1");
    server.send(200, "text/plain", "OK");
});

server.on("/closeGarage", HTTP_GET, []() {
    Serial.println("Sending CLOSE GARAGE command to main ESP32");
    sendCommandToESP32("/cmd?closeGarage=1");
    server.send(200, "text/plain", "OK");
});
  server.on("/light", HTTP_GET, handleLight);
  server.on("/thresholds", HTTP_GET, handleThresholds);
  server.on("/camera/on", HTTP_GET, handleCameraOn);
  server.on("/camera/off", HTTP_GET, handleCameraOff);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/stream", HTTP_GET, handleStream);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html><html><head>
  <title>Smart Home Hub</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --primary: #4361ee;
      --secondary: #3f37c9;
      --accent: #4895ef;
      --danger: #f72585;
      --success: #4cc9f0;
      --warning: #f8961e;
      --dark: #212529;
      --light: #f8f9fa;
      --gray: #6c757d;
    }
    
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    
    body {
      font-family: 'Roboto', sans-serif;
      background: linear-gradient(135deg, #f5f7fa 0%, #e4e8f0 100%);
      color: var(--dark);
      line-height: 1.6;
      padding: 20px;
      min-height: 100vh;
    }
    
    .container {
      max-width: 1200px;
      margin: 0 auto;
    }
    
    header {
      text-align: center;
      margin-bottom: 30px;
      animation: fadeIn 0.8s ease-out;
    }
    
    h1 {
      font-size: 2.5rem;
      color: var(--primary);
      margin-bottom: 10px;
      font-weight: 700;
    }
    
    .subtitle {
      color: var(--gray);
      font-size: 1.1rem;
    }
    
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 20px;
      margin-bottom: 30px;
    }
    
    .card {
      background: white;
      border-radius: 12px;
      padding: 25px;
      box-shadow: 0 10px 20px rgba(0,0,0,0.05);
      transition: transform 0.3s ease, box-shadow 0.3s ease;
    }
    
    .card:hover {
      transform: translateY(-5px);
      box-shadow: 0 15px 30px rgba(0,0,0,0.1);
    }
    
    .card-header {
      display: flex;
      align-items: center;
      margin-bottom: 20px;
      padding-bottom: 15px;
      border-bottom: 1px solid rgba(0,0,0,0.05);
    }
    
    .card-icon {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: var(--accent);
      display: flex;
      align-items: center;
      justify-content: center;
      margin-right: 15px;
      color: white;
      font-size: 1.2rem;
    }
    
    .card-title {
      font-size: 1.3rem;
      font-weight: 500;
      color: var(--dark);
    }
    
    .status-item {
      display: flex;
      justify-content: space-between;
      margin-bottom: 12px;
      padding: 10px 0;
      border-bottom: 1px dashed rgba(0,0,0,0.05);
    }
    
    .status-label {
      font-weight: 400;
      color: var(--gray);
    }
    
    .status-value {
      font-weight: 500;
    }
    
    .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      padding: 10px 20px;
      border-radius: 8px;
      font-weight: 500;
      cursor: pointer;
      transition: all 0.3s ease;
      border: none;
      text-decoration: none;
      font-size: 0.95rem;
      margin-right: 10px;
      margin-bottom: 10px;
    }
    
    .btn i {
      margin-right: 8px;
    }
    
    .btn-primary {
      background: var(--primary);
      color: white;
    }
    
    .btn-primary:hover {
      background: var(--secondary);
      transform: translateY(-2px);
    }
    
    .btn-danger {
      background: var(--danger);
      color: white;
    }
    
    .btn-danger:hover {
      opacity: 0.9;
      transform: translateY(-2px);
    }
    
    .btn-success {
      background: var(--success);
      color: white;
    }
    
    .btn-success:hover {
      opacity: 0.9;
      transform: translateY(-2px);
    }
    
    .btn-group {
      display: flex;
      flex-wrap: wrap;
      margin-top: 15px;
    }
    
    .slider-container {
      margin-top: 15px;
    }
    
    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 8px;
      border-radius: 4px;
      background: #e9ecef;
      outline: none;
      margin-bottom: 15px;
    }
    
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--primary);
      cursor: pointer;
      transition: all 0.2s ease;
    }
    
    .slider::-webkit-slider-thumb:hover {
      transform: scale(1.1);
    }
    
    .temp-control {
      display: flex;
      gap: 15px;
      margin-top: 15px;
    }
    
    .temp-input {
      flex: 1;
      padding: 10px;
      border: 1px solid #dee2e6;
      border-radius: 8px;
      font-size: 1rem;
    }
    
    #denied {
      text-align: center;
      padding: 50px 20px;
      background: white;
      border-radius: 12px;
      box-shadow: 0 10px 20px rgba(0,0,0,0.05);
      max-width: 500px;
      margin: 0 auto;
      animation: fadeIn 0.8s ease-out;
    }
    
    #denied h2 {
      color: var(--danger);
      margin-bottom: 15px;
    }
    
    #denied p {
      color: var(--gray);
      margin-bottom: 20px;
    }
    
    #denied .icon {
      font-size: 3rem;
      color: var(--danger);
      margin-bottom: 20px;
    }
    
    #granted {
      display: none;
      animation: fadeIn 0.8s ease-out;
    }
    
    .badge {
      display: inline-block;
      padding: 5px 10px;
      border-radius: 20px;
      font-size: 0.8rem;
      font-weight: 500;
    }
    
    .badge-success {
      background: rgba(76, 201, 240, 0.1);
      color: var(--success);
    }
    
    .badge-danger {
      background: rgba(247, 37, 133, 0.1);
      color: var(--danger);
    }
    
    .badge-warning {
      background: rgba(248, 150, 30, 0.1);
      color: var(--warning);
    }
    
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(20px); }
      to { opacity: 1; transform: translateY(0); }
    }
    
    @media (max-width: 768px) {
      .grid {
        grid-template-columns: 1fr;
      }
      
      .btn-group {
        justify-content: center;
      }
      
      .temp-control {
        flex-direction: column;
      }
    }
  </style>
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css">
  </head>
  <body>
    <div class="container">
      <div id="denied">
        <div class="icon"><i class="fas fa-fingerprint"></i></div>
        <h2>Access Required</h2>
        <p>Please authenticate using your fingerprint to access the Smart Home controls</p>
        <div class="spinner"><i class="fas fa-spinner fa-spin"></i></div>
      </div>
      
      <div id="granted">
        <header>
          <h1>Welcome to Your Smart Home</h1>
          <p class="subtitle">Control your home with a single tap</p>
        </header>
        
        <div class="grid">
          <div class="card">
            <div class="card-header">
              <div class="card-icon"><i class="fas fa-home"></i></div>
              <h3 class="card-title">Home Status</h3>
            </div>
            <div class="status-item">
              <span class="status-label"><i class="fas fa-door-open"></i> Front Door</span>
              <span class="status-value" id="doorStatus"><span class="badge badge-danger">Unknown</span></span>
            </div>
            <div class="status-item">
              <span class="status-label"><i class="fas fa-window-maximize"></i> Living Room Window</span>
              <span class="status-value" id="windowStatus"><span class="badge badge-danger">Unknown</span></span>
            </div>
            <div class="status-item">
              <span class="status-label"><i class="fas fa-warehouse"></i> Garage</span>
              <span class="status-value" id="garageStatus"><span class="badge badge-danger">Unknown</span></span>
            </div>
            <div class="status-item">
              <span class="status-label"><i class="fas fa-thermometer-half"></i> Temperature</span>
              <span class="status-value" id="tempStatus"><span class="badge badge-success">Unknown</span></span>
            </div>
            <div class="status-item">
              <span class="status-label"><i class="fas fa-car"></i> Car Present</span>
              <span class="status-value" id="carStatus"><span class="badge badge-warning">Unknown</span></span>
            </div>
          </div>
          
          <div class="card">
            <div class="card-header">
              <div class="card-icon"><i class="fas fa-lightbulb"></i></div>
              <h3 class="card-title">Lighting Control</h3>
            </div>
            <div class="slider-container">
              <input type="range" min="0" max="255" value=")rawliteral" + String(pwmValue) + R"rawliteral(" class="slider" id="pwmSlider">
              <p>Brightness: <strong id="pwmDisplay">)rawliteral" + String(pwmValue) + R"rawliteral(</strong></p>
            </div>
          </div>
        </div>
        
        <div class="grid">
          <div class="card">
            <div class="card-header">
              <div class="card-icon"><i class="fas fa-temperature-low"></i></div>
              <h3 class="card-title">Climate Control</h3>
            </div>
            <div class="temp-control">
                <div style="position: relative; flex: 1;">
                  <input type="number" id="lowTemp" class="temp-input" placeholder="Low threshold" value=")rawliteral" + String(tempLow) + R"rawliteral(">
                  <span style="position: absolute; right: 10px; top: 50%; transform: translateY(-50%); color: #6c757d;">&deg;C</span>
                </div>
                <div style="position: relative; flex: 1;">
                  <input type="number" id="highTemp" class="temp-input" placeholder="High threshold" value=")rawliteral" + String(tempHigh) + R"rawliteral(">
                  <span style="position: absolute; right: 10px; top: 50%; transform: translateY(-50%); color: #6c757d;">&deg;C</span>
                </div>
              </div>
            <div class="btn-group">
              <button class="btn btn-primary" onclick="setThresholds()"><i class="fas fa-save"></i> Save Settings</button>
            </div>
          </div>
          
          <div class="card">
            <div class="card-header">
              <div class="card-icon"><i class="fas fa-key"></i></div>
              <h3 class="card-title">Security Controls</h3>
            </div>
            <div class="btn-group">
              <button class="btn btn-success" onclick="fetch('/unlock')" id="unlockBtn"><i class="fas fa-lock-open"></i> Unlock Door</button>
              <button class="btn btn-danger" onclick="fetch('/lock')" id="lockBtn"><i class="fas fa-lock"></i> Lock Door</button>
              <button class="btn btn-primary" onclick="fetch('/openGarage')" id="openGarageBtn"><i class="fas fa-arrow-up"></i> Open Garage</button>
              <button class="btn btn-danger" onclick="closeGarage()" id="closeGarageBtn"><i class="fas fa-arrow-down"></i> Close Garage</button>
              <a href="/stream" class="btn btn-primary"><i class="fas fa-camera"></i> View Camera</a>
            </div>
          </div>
        </div>
      </div>
    </div>

    <script>
      // Initialize with stored values immediately
      let currentPWM = )rawliteral" + String(pwmValue) + R"rawliteral(;
      let currentLowTemp = )rawliteral" + String(tempLow) + R"rawliteral(;
      let currentHighTemp = )rawliteral" + String(tempHigh) + R"rawliteral(;
      let accessGranted = false;

      // Format status badges
      function formatStatus(value, type) {
        if (value.includes('open')) return `<span class="badge badge-danger">Open</span>`;
        if (value.includes('closed')) return `<span class="badge badge-success">Closed</span>`;
        if (type === 'car') {
          return value === '1' ? `<span class="badge badge-warning">Present</span>` : `<span class="badge badge-success">Absent</span>`;
        }
        return `<span class="badge">${value}</span>`;
      }

      // Initialize slider immediately with stored value
      document.addEventListener('DOMContentLoaded', function() {
        const slider = document.getElementById('pwmSlider');
        const display = document.getElementById('pwmDisplay');
        slider.value = currentPWM;
        display.textContent = currentPWM;
        
        // Initialize temp fields
        document.getElementById('lowTemp').value = currentLowTemp;
        document.getElementById('highTemp').value = currentHighTemp;
      });
    
      function updateUI() {
        fetch('/status')
          .then(res => res.json())
          .then(data => {
            if(data.accessGranted) {
              document.getElementById('denied').style.display = 'none';
              document.getElementById('granted').style.display = 'block';
              
              // Update status with formatted badges
              document.getElementById('doorStatus').innerHTML = formatStatus(data.door);
              document.getElementById('windowStatus').innerHTML = formatStatus(data.window);
              document.getElementById('garageStatus').innerHTML = formatStatus(data.garage);
              document.getElementById('tempStatus').innerHTML = data.temp + '&deg;C';
              document.getElementById('carStatus').innerHTML = formatStatus(data.car, 'car');
              
              // Update control states
              document.getElementById('lockBtn').disabled = !data.accessGranted;
              document.getElementById('unlockBtn').disabled = !data.accessGranted;
              document.getElementById('openGarageBtn').disabled = data.garage === "open" || !data.accessGranted;
              document.getElementById('closeGarageBtn').disabled = data.garage === "closed" || !data.accessGranted;
              
              // Update values if changed
              if(data.pwmValue != currentPWM) {
                currentPWM = data.pwmValue;
                document.getElementById('pwmSlider').value = currentPWM;
                document.getElementById('pwmDisplay').innerText = currentPWM;
              }
              if(data.tempLow != currentLowTemp) {
                currentLowTemp = data.tempLow;
                document.getElementById('lowTemp').value = currentLowTemp;
              }
              if(data.tempHigh != currentHighTemp) {
                currentHighTemp = data.tempHigh;
                document.getElementById('highTemp').value = currentHighTemp;
              }
            } else {
              document.getElementById('denied').style.display = 'block';
              document.getElementById('granted').style.display = 'none';
            }
          });
      }
      
      // Slider control with immediate response
      const slider = document.getElementById('pwmSlider');
      const display = document.getElementById('pwmDisplay');
      let lastSentValue = )rawliteral" + String(pwmValue) + R"rawliteral(;
      let updateTimeout;
      
      slider.addEventListener('input', function() {
        const value = parseInt(this.value);
        display.textContent = value;
        
        if (Math.abs(value - lastSentValue) >= 5) {
          clearTimeout(updateTimeout);
          updateTimeout = setTimeout(() => {
            fetch('/light?value=' + value)
              .then(response => {
                if (response.ok) lastSentValue = value;
              });
          }, 30);
        }
      });
      
      function setPWM(val) {
        fetch('/light?value=' + val);
      }
      
      function setThresholds() {
        const low = document.getElementById('lowTemp').value;
        const high = document.getElementById('highTemp').value;
        fetch('/thresholds?low=' + low + '&high=' + high);
      }
      
      function closeGarage() {
        if (confirm("Are you sure you want to close the garage?")) {
          fetch('/closeGarage');
        }
      }

      // Update UI every second
      setInterval(updateUI, 1000);
      updateUI(); // Initial update
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleAuth() {
  if (server.hasArg("access")) {
    accessGranted = (server.arg("access") == "1");
    //accessGranted = false;
    preferences.putBool("access", accessGranted); // Reset on boot

  }
  server.send(200, "text/plain", "OK");
}

void handleUpdate() {
  if (server.hasArg("door")) doorStatus = server.arg("door");
  if (server.hasArg("window")) windowStatus = server.arg("window");
  if (server.hasArg("garage")) garageStatus = server.arg("garage");
  if (server.hasArg("temp")) temperature = server.arg("temp");
  if (server.hasArg("car")) carPresent = server.arg("car");
  server.send(200, "text/plain", "OK");
}

void handleToggleLock() {
  sendCommandToESP32("/cmd?toggleLock=1");
  server.send(200, "text/plain", "OK");
}

void handleToggleGarage() {
  sendCommandToESP32("/cmd?toggleGarage=1");
  server.send(200, "text/plain", "OK");
}

void handleLight() {
  if (server.hasArg("value")) {
    pwmValue = server.arg("value").toInt();
    preferences.putInt("pwm", pwmValue);
    sendCommandToESP32("/cmd?light=" + String(pwmValue));
  }
  server.send(200, "text/plain", "OK");
}

void handleThresholds() {
  if (server.hasArg("low") && server.hasArg("high")) {
    tempLow = server.arg("low").toInt();
    tempHigh = server.arg("high").toInt();
    preferences.putInt("tempLow", tempLow);
    preferences.putInt("tempHigh", tempHigh);
    sendCommandToESP32("/cmd?cool=" + String(tempHigh) + "&heat=" + String(tempLow));
  }
  server.send(200, "text/plain", "OK");
}

void handleCameraOn() {
  cameraActive = true;
  server.sendHeader("Location", "/stream");
  server.send(302);
}

void handleCameraOff() {
  cameraActive = false;
  server.sendHeader("Location", "/");
  server.send(302);
}

void handleStatus() {
  String json = "{";
  json += "\"door\":\"" + doorStatus + "\",";
  json += "\"window\":\"" + windowStatus + "\",";
  json += "\"garage\":\"" + garageStatus + "\",";
  json += "\"temp\":\"" + temperature + "\",";
  json += "\"car\":\"" + carPresent + "\",";
  json += "\"pwmValue\":" + String(pwmValue) + ",";
  json += "\"tempLow\":" + String(tempLow) + ",";
  json += "\"tempHigh\":" + String(tempHigh) + ",";
  json += "\"accessGranted\":" + String(accessGranted ? "true" : "false");
  json += "}";
  server.send(200, "application/json; charset=utf-8", json);
}

void handleStream() {
  // First send the HTML page with the stream
  if (server.hasArg("stream") == false) {
    String html = R"rawliteral(
    <!DOCTYPE html><html><head>
    <title>Camera Stream</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f5f5f5; }
      .container { max-width: 800px; margin: 0 auto; text-align: center; }
      .btn { background: #f44336; color: white; border: none; padding: 10px 15px; border-radius: 4px; cursor: pointer; margin: 20px; }
      img { max-width: 100%; height: auto; border-radius: 8px; margin-top: 20px; }
    </style>
    <script>
      function refreshImage() {
        const img = document.getElementById('stream');
        img.src = '/stream?stream=1&' + new Date().getTime();
      }
      
      // Refresh image every 100ms (10fps)
      setInterval(refreshImage, 100);
    </script>
    </head>
    <body>
      <div class="container">
        <h1>Camera Stream</h1>
        <button class="btn" onclick="window.location.href='/'">Back to Dashboard</button>
        <div>
          <img id="stream" src="/stream?stream=1">
        </div>
      </div>
    </body>
    </html>
    )rawliteral";
    
    cameraActive = true;
    server.send(200, "text/html", html);
    return;
  }
  
  // Actual stream handling
  if (!cameraActive) {
    server.send(503, "text/plain", "Camera not active");
    return;
  }

  // Capture photo
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  // Send the photo
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  
  // Return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);
}

void sendCommandToESP32(const String& path) {
  HTTPClient http;
  http.begin("http://" + String(esp32Ip) + path);
  http.setTimeout(200);  // Reduce timeout
  http.addHeader("Cache-Control", "no-cache");
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP request failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}
