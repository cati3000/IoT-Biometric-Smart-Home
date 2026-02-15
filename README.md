# IoT Biometric Smart Home

A distributed IoT smart home system featuring biometric fingerprint authentication, live video streaming, and remote web control using ESP32, ESP32-CAM, and Arduino Uno.

## Project Overview

This system implements a secure home automation ecosystem where access to the control interface is restricted by biometric validation. It uses a distributed hardware architecture to balance processing loads:
* **Arduino Uno:** Handles biometric fingerprint validation and sensor management.
* **ESP32-CAM:** Hosts the web server, handles HTTP requests, and provides a live video stream.
* **ESP32 (30-pin):** Acts as the main controller for sensors and actuators, executing commands received from the web interface.

## Features

* **Biometric Security:** Access to the system is granted only after valid fingerprint authentication.
* **Web Interface:** Responsive dashboard for real-time monitoring and control.
* **Live Surveillance:** Real-time video streaming via the ESP32-CAM.
* **Automation:**
    * Smart lighting control with PWM intensity adjustment.
    * Automated temperature control (Heating/Cooling logic).
    * Garage door control with auto-close functionality.
* **Status Monitoring:** Real-time feedback on door/window states and temperature.

## Hardware Architecture

The system is built on three main development boards communicating via Serial (UART) and Wi-Fi:

1.  **Arduino Uno**: Interfaces with the optical fingerprint sensor. It communicates authentication status ("GRANTED" or "DENIED") to the ESP32 via a voltage divider.
2.  **ESP32 (30-pin)**: The central processing unit. It reads data from sensors (DHT11, magnetic reeds, ultrasonic) and drives actuators (relays, motors, LEDs).
3.  **ESP32-CAM**: The Wi-Fi gateway. It serves the HTML/JS frontend and bridges user commands to the main ESP32.

### Key Components
* Optical Fingerprint Sensor Module
* DHT11 Temperature & Humidity Sensor
* Ultrasonic Sensor (HC-SR04)
* Solenoid Lock & Relays
* DC Motors & L9110/L298N Drivers
* Magnetic Reed Switches

## Installation and Setup

1.  **Wiring**: Connect the components according to the wiring diagrams in the `documentation` file.
    * *Note:* Ensure a voltage divider is used on the RX line of the ESP32 when connecting to the Arduino Uno TX to shift 5V logic to 3.3V.
2.  **Network Configuration**: Update the `ssid` and `password` variables in both ESP32 and ESP32-CAM files to match your local Wi-Fi network.
3.  **Upload Code**:
    * Flash the `arduino_uno` code to the Arduino board.
    * Flash the `esp32_main` code to the ESP32 dev board.
    * Flash the `esp32_cam` code using an FTDI programmer or Arduino as ISP.

## Usage

1.  Power on the system.
2.  Scan a registered fingerprint on the sensor.
3.  Upon successful validation, the web interface becomes accessible.
4.  Navigate to the IP address displayed in the Serial Monitor to control the home automation features.
