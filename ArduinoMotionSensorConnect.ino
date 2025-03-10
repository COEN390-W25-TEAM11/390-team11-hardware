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

#include <WiFi.h>
#include <WebServer.h>
const char* ssid = "FRD";       // Replace with your Wi-Fi SSID
const char* password = "22311824"; // Replace with your Wi-Fi password

#define PIR_PIN 21    // Connected to the A pin of the sensor
#define LED_PIN 23    
#define LED_LIGHT 13

WebServer server(80);  // Web server on port 80

void handleRoot() {
    int motion = digitalRead(PIR_PIN);  // Read sensor
    // Motion detected (LOW indicates motion)
    String motionStatus = (motion == LOW) ? "Motion detected!" : "No motion";

    // Send the motion status as response
    server.send(200, "text/plain", motionStatus);

}

void setup() {
    Serial.begin(115200);
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    Serial.println("Connected to WiFi!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.begin();

    pinMode(LED_BUILTIN, OUTPUT); // Only for visual cue that board is on
    pinMode(PIR_PIN, INPUT);      // To detect motion
    pinMode(LED_PIN, OUTPUT);     // To light up when motion is detected
    pinMode(LED_LIGHT, OUTPUT);   
}

void loop() {
    server.send(200, "text/plain", "ESP32 is connected!");
    server.handleClient();
    digitalWrite(LED_BUILTIN, HIGH); // Keep built in LED always on

    int motion = digitalRead(PIR_PIN);  // Read sensor

    // When motion is detected, sensor returns '0'
    if (motion == LOW) {
        Serial.println("Motion detected!");
        digitalWrite(LED_PIN, HIGH);
        digitalWrite(LED_LIGHT, HIGH); 
    } else {
        Serial.println("No motion");
        digitalWrite(LED_PIN, LOW);
        digitalWrite(LED_LIGHT, LOW);
    }

    delay(200);  // Arbitrary
}