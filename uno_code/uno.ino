#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>

SoftwareSerial mySerial(4, 5); // RX, TX for fingerprint sensor
Adafruit_Fingerprint finger(&mySerial);

void setup() {
  Serial.begin(9600);
  mySerial.begin(57600);
  finger.begin(57600);

  delay(1000);

  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor found!");
  } else {
    Serial.println("Fingerprint sensor NOT found!");
    while (1) delay(100); // halt if sensor not found
  }
}

void loop() {
  uint8_t p = finger.getImage();
  
  if (p == FINGERPRINT_NOFINGER) {
    // No finger detected, do nothing, no spam
    delay(100);
    return;
  }
  
  if (p != FINGERPRINT_OK) {
    Serial.println("Error reading finger image");
    sendAccessStatus(false);
    delay(1000);
    return;
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    Serial.println("Failed to convert image");
    sendAccessStatus(false);
    delay(1000);
    return;
  }

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.print("Fingerprint ID found: ");
    Serial.println(finger.fingerID);
    sendAccessStatus(true);
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Fingerprint not recognized");
    sendAccessStatus(false);
  } else {
    Serial.print("Fingerprint search error: ");
    Serial.println(p);
    sendAccessStatus(false);
  }
  
  delay(1000); // wait a second before next scan to avoid spamming
}

void sendAccessStatus(bool granted) {
  if (granted) {
    Serial.println("GRANTED");
  } else {
    Serial.println("DENIED");
  }
}
