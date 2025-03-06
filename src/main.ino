/*
  COEN/ELEC 390 Team 11
  Base code that turns on the LED when motion is detected by the PIR motion sensor.

  PIR connections:
  -> A pin connected to PIR_PIN on ESP
  -> VCC pin connected to VUSB on ESP
  -> GND pin connected to GND on ESP

  LED connections:
  -> Cathode connected to GND on ESP
  -> Anode connected to resistor (220 used)
  -> Resistor connected to LED_PIN on ESP
*/

// ===== WIFI =====
#include <WiFi.h>
const char* ssid = "CHANGE-THIS-TO-SSID"; // CHANGE IT
const char* password = "CHANGE-THIS-TO-PASSWORD"; // CHANGE IT

// ===== Pin definitions =====
#define PIR_PIN 21    // Connected to the A pin of the sensor
#define LED_PIN 23    

void setup_wifi() {

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Print the ESP32's IP address
  Serial.print("ESP32 Web Server's IP address: ");
  Serial.println(WiFi.localIP());

}

void setup() {

  // ===== Begin the serial monitor =====
  Serial.begin(9600);

  // ===== Pin setup =====
  pinMode(LED_BUILTIN, OUTPUT); // Only for visual cue that board is on
  pinMode(PIR_PIN, INPUT);      // To detect motion
  pinMode(LED_PIN, OUTPUT);     // To light up when motion is detected

  // ===== WIFI setup =====
  setup_wifi();

}

void loop() {
  
  // ===== Keep built in LED always on =====
  digitalWrite(LED_BUILTIN, HIGH); 

  // ===== Control the LED according to sensor input =====
  int motion = digitalRead(PIR_PIN);  // Read sensor
  if (motion == LOW) { // When motion is detected, sensor returns '0'
    Serial.println("Motion detected!");
    digitalWrite(LED_PIN, HIGH); 
    Serial.println(WiFi.localIP()); // Sometimes the stuff in setup_wifi() doesn't print so I just put it here
  } else {
    Serial.println("No motion");
    digitalWrite(LED_PIN, LOW);  
  }

  // ===== Arbitrary delay =====
  delay(500); 
}
