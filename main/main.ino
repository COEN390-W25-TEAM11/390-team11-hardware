// ========== LIBRARIES ==========
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <ESPping.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// ========== DATA STORAGE ===========
Preferences preferences;

// ========== WIFI INFORMATION ===========
const char* ssid = "BLITE4390";       // Replace with your Wi-Fi SSID
const char* password = "rMRXct56DfLG"; // Replace with your Wi-Fi password

// ========== PIN AND ID DEFINITIONS ==========
// Available pins for motion sensors
#define PIR_PIN1 21   
#define PIR_PIN2 17 
int sensorPins[2] = {PIR_PIN1, PIR_PIN2};

// Available pins for LEDs
#define LED_PIN1 23   
#define LED_PIN2 22 
int lightPins[2] = {LED_PIN1, LED_PIN2};

String espId = "fb3dcd74-f3b7-4bdb-b58a-626bd998724f"; // hardcoded espId for register endpoint

// ========== WEBSOCKET/NGROK DEFINITIONS ==========
const char* host = "8470-138-229-30-132.ngrok-free.app"; // Ngrok host

const int websocket_port = 443; // Use 443 for WSS, or 80 for WS
WebSocketsClient webSocket; // WebSocket Client
String websocket_path;

// ========== CONTROL VARIABLES ==========
bool manualControl[2] = {false, false};
bool lightState[2] = {false, false};
bool lastMotionState[2] = {false, false}; 
int brightness[2] = {255, 255};

// ========== REGISTER LIGHTS AND SENSORS (Register Endpoint) ==========
void registerESP() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = "https://" + String(host) + "/EspLight/register/" + espId;
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        // Create JSON object
        StaticJsonDocument<200> doc;
        JsonArray sensorArray = doc.createNestedArray("sensorPins");
        JsonArray lightArray = doc.createNestedArray("lightPins");

        // Add pin values
        sensorArray.add(sensorPins[0]);
        sensorArray.add(sensorPins[1]);

        lightArray.add(lightPins[0]);
        lightArray.add(lightPins[1]);

        // Serialize JSON to string
        String jsonPayload;
        serializeJson(doc, jsonPayload);

        Serial.println("Sending Register Request:");
        Serial.println(jsonPayload);

        // Send POST request
        int httpResponseCode = http.POST(jsonPayload);
        String response = http.getString();
        http.end();

        if (httpResponseCode > 0) {
            Serial.println("ESP Registered: " + response);
        } else {
            Serial.println("Failed to register ESP: " + httpResponseCode);
        }
    }
}

// ========== SEND POST REQUEST TO MOVEMENTUPDATE (MovementUpdate Endpoint) ==========
void sendPOSTRequest(bool motion, int PIR_PIN) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://" + String(host) + "/EspLight/movement/" + espId + "/" + String(PIR_PIN);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"movement\": " + String(motion ? "true" : "false") + "}";
    int httpResponseCode = http.POST(jsonPayload);
    http.end();

    if (httpResponseCode > 0) {
      Serial.println("Motion event sent: " + jsonPayload);
    }
    else {
      Serial.println("Error sending POST request");
    }
  }
}

// ========== WEBSOCKET EVENT HANDLER ==========
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            Serial.println("WebSocket Connected!");
            webSocket.sendTXT("ESP32 Connected!");  // Send test message
            break;
        
        case WStype_DISCONNECTED:
            Serial.println("WebSocket Disconnected! Reconnecting...");
            webSocket.beginSSL(host, websocket_port, websocket_path.c_str());
            break;
        
        case WStype_TEXT:
            Serial.print("Received Message: ");
            Serial.println((char*)payload);

            String msg = (char*)payload;

            // Extract the pin number from the message
            int pinIndex = msg.indexOf("\"pin\":");
            int pinNumber = -1;
            if (pinIndex != -1) {
                String pinValue = msg.substring(pinIndex + 6, msg.indexOf(",", pinIndex));
                pinNumber = pinValue.toInt();
            }

            // Extracting "overide" value
            int overrideIndex = msg.indexOf("\"overide\":");
            bool overrideState = false;
            if (overrideIndex != -1) {
                String overrideValue = msg.substring(overrideIndex + 10, msg.indexOf(",", overrideIndex));
                overrideState = (overrideValue == "true");
            }

            // Extracting "state" value
            int stateIndex = msg.indexOf("\"state\":");
            bool newState = false;
            if (stateIndex != -1) {
                String stateValue = msg.substring(stateIndex + 8, msg.indexOf(",", stateIndex));
                newState = (stateValue == "1");
            }

            // Extracting "brightness" value if present
            int brightnessIndex = msg.indexOf("\"brightness\":");
            int newBrightness = 255; // default
            if (brightnessIndex != -1) {
                String brightnessValue = msg.substring(brightnessIndex + 12);
                brightnessValue = brightnessValue.substring(0, brightnessValue.indexOf("}")); // Extract numeric value
                newBrightness = brightnessValue.toInt();

                if (newBrightness >= 0 && newBrightness <= 255) {  // Ensure valid range
                    Serial.print("Updated Brightness: ");
                    Serial.println(newBrightness);
                }
            }

            // Apply extracted values
            for (int i = 0; i < 2; i++) {
                // Check if the pin number from the message matches the current light
                if (pinNumber == lightPins[i]) {
                    if (overrideState) {
                        // Manual control mode
                        manualControl[i] = overrideState;
                        lightState[i] = newState;  // Set the state for this particular light
                        brightness[i] = newBrightness; // Update brightness for the current light
                    } else {
                        // Auto-control mode (sensor-based behavior will apply here)
                        lightState[i] = newState;  // Update state based on sensor detection or manual override
                    }

                    // Debugging prints
                    // Serial.print("Light Pin ");
                    // Serial.print(lightPins[i]); // Print the pin number
                    // Serial.print(" - Manual Control: ");
                    // Serial.println(manualControl ? "Enabled" : "Disabled");
                    // Serial.print("Light Pin ");
                    // Serial.print(lightPins[i]); // Print the pin number
                    // Serial.print(" - State: ");
                    // Serial.println(lightState[i] ? "On" : "Off");
                    // Serial.print("Light Pin ");
                    // Serial.print(lightPins[i]); // Print the pin number
                    // Serial.print(" - Brightness: ");
                    // Serial.println(brightness);
                }
            }
            break;
    }
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    Serial.println("Starting ESP32...");
    
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    // Test internet connectivity
    bool success = Ping.ping("www.google.com", 3);
    if (!success) Serial.println("Ping failed");
    else Serial.println("Ping successful.");

    // Setup hardware
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIR_PIN1, INPUT);
    pinMode(PIR_PIN2, INPUT);
    pinMode(LED_PIN1, OUTPUT);
    pinMode(LED_PIN2, OUTPUT);

    // Register ESP on the server
    registerESP();

    // WebSocket Connection
    websocket_path = "/EspLight/ws/" + espId;
    Serial.println("Connecting to WebSocket: " + String(host) + websocket_path);
    webSocket.beginSSL(host, websocket_port, websocket_path);
    webSocket.onEvent(webSocketEvent);
}

// ========== LOOP ==========
void loop() {
  // Maintain WebSocket connection
  webSocket.loop();

  // keep on for visual indication that board is running
  digitalWrite(LED_BUILTIN, HIGH);

  for (int i = 0; i < 2; i++) {
  int motion = digitalRead(sensorPins[i]);
  bool motionDetected = (motion == HIGH);

  // Compare with the last known state
  if (motionDetected != lastMotionState[i]) {
    // State changed — update LED and notify
    lastMotionState[i] = motionDetected;

    if (!manualControl[i]) {
      digitalWrite(lightPins[i], motionDetected ? HIGH : LOW);

      // only send true motion 
      if (motionDetected) {
        sendPOSTRequest(motionDetected, sensorPins[i]);
      }
    }
  } else {
    // If no change in motion, still update LED in case of manual mode toggle
    if (!manualControl[i]) {
      digitalWrite(lightPins[i], motionDetected ? HIGH : LOW);
    } else {
      digitalWrite(lightPins[i], lightState[i] ? HIGH : LOW);
    }
  }
}
}