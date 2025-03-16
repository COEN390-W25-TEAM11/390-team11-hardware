/*
  COEN/ELEC 390 Team 11
  - Connects to local WiFi
  - Turns on the LED when motion is detected by the PIR motion sensor

  PIR connections:
  -> A pin connected to PIR_PIN on ESP
  -> VCC pin connected to VUSB on ESP
  -> GND pin connected to GND on ESP

  LED connections:
  -> Cathode connected to GND on ESP
  -> Anode connected to resistor (220Ω used)
  -> Resistor connected to LED_PIN on ESP
*/

// ========== LIBRARIES ==========
#include <WiFi.h>      
#include <WebServer.h>  

// ========== PINS ==========
#define PIR_PIN 21    // Motion sensor output pin (connected to A pin)
#define LED_PIN 23    // LED that turns on when motion is detected
#define LED_LIGHT 13  // Additional LED indicator

// ========== WIFI AND WEB SERVER INFO ==========
const char* ssid = "FRD";           // Replace with your Wi-Fi SSID
const char* password = "22311824";  // Replace with your Wi-Fi password
WebServer server(80);               // Initialize web server on port 80

/* ========== ROOT '/' ENDPOINT ==========
  -> This function handles requests to the root ('/') URL
  -> Sends "Motion detected!" or "No motion" as plain text according to sensor readings
*/
void handleRoot() {
    int motion = digitalRead(PIR_PIN);                                        // Read motion sensor state
    String motionStatus = (motion == LOW) ? "Motion detected!" : "No motion"; // LOW indicates motion detected
    server.send(200, "text/plain", motionStatus);                             // Respond with motion status as plain text
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);

    // Connect to local WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { // Wait until connected
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP()); // Print the ESPs IP address

    // Endpoint definitions
    server.on("/", handleRoot);

    // Start the web server
    server.begin();

    // Initialize pin modes
    pinMode(LED_BUILTIN, OUTPUT); // Built-in LED (used for visual indicator that ESP is powered on)
    pinMode(PIR_PIN, INPUT);      // PIR motion sensor as input
    pinMode(LED_PIN, OUTPUT);     // LED as output (turns on when motion is detected)
    pinMode(LED_LIGHT, OUTPUT);   // LED as output (turns on when motion is detected)
}

// ========== LOOP ==========
void loop() {
    server.handleClient();              // Handle incoming client requests
    digitalWrite(LED_BUILTIN, HIGH);    // Keep built-in LED always on

    int motion = digitalRead(PIR_PIN);  // Read motion sensor state

    // Turn on LED according to motion detected
    if (motion == LOW) {
        Serial.println("Motion detected!");
        digitalWrite(LED_PIN, HIGH);    // Turn on LED
        digitalWrite(LED_LIGHT, HIGH);  // Turn on additional LED
    } else {
        Serial.println("No motion");
        digitalWrite(LED_PIN, LOW);     // Turn off LED
        digitalWrite(LED_LIGHT, LOW);   // Turn off additional LED
    }

    delay(200);  // Adjust as necessary
}
