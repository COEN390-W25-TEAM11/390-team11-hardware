// ========== LIBRARIES ==========
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <ESPping.h>

// ========== WIFI INFORMATION ===========
const char* ssid = "BLITE4390";       // Replace with your Wi-Fi SSID
const char* password = "rMRXct56DfLG"; // Replace with your Wi-Fi password

// ========== PIN AND LIGHT DEFINITIONS ==========
#define PIR_PIN 21    // Motion sensor
#define LED_PIN 23    // Indicator LED
#define LED_LIGHT 13  // Another LED output
const char* LED_ID_1 = "6FA6C8AB-EE3B-47A6-B84A-D706B313366B";

// ========== WEBSOCKET/NGROK DEFINITIONS ==========
const char* websocket_host = "eb9e-138-229-30-132.ngrok-free.app"; // Ngrok host
const char* websocket_path = "/EspLight/ws/6FA6C8AB-EE3B-47A6-B84A-D706B313366B"; // WebSocket path
const int websocket_port = 443; // Use 443 for WSS, or 80 for WS
WebSocketsClient webSocket; // WebSocket Client

// ========== CONTROL VARIABLES ==========
bool manualControl = false;
bool lightState = false;
bool lastMotionState = false;

// ========== WEBSOCKET EVENT HANDLER ==========
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            Serial.println("WebSocket Connected!");
            webSocket.sendTXT("ESP32 Connected!");  // Send test message
            break;
        
        case WStype_DISCONNECTED:
            Serial.println("WebSocket Disconnected! Reconnecting...");
            webSocket.beginSSL(websocket_host, websocket_port, websocket_path);
            break;
        
        case WStype_TEXT:
            Serial.print("Received Message: ");
            Serial.println((char*)payload);

            String msg = (char*)payload;
            if (msg == "{\"overide\":true,\"state\":0}") {
                manualControl = true;
                lightState = true;
            }
            else if (msg == "{\"overide\":true,\"state\":1}") {
                manualControl = true;
                lightState = false;
            }
           else if (msg == "{\"overide\":false,\"state\":1}") {
                manualControl = false;
                lightState = true;
            }

            else if(msg == "{\"overide\":false,\"state\":0}") {
              manualControl = false;
              lightState = false;
            }
            break;
    }
}

// ========== SEND POST REQUEST TO MOVEMENTUPDATE ==========
void sendPOSTRequest(bool motion) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://eb9e-138-229-30-132.ngrok-free.app/EspLight/" + String(LED_ID_1);

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
    pinMode(PIR_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_LIGHT, OUTPUT);

    // WebSocket Connection
    Serial.println("Connecting to WebSocket: " + String(websocket_host) + websocket_path);
    webSocket.beginSSL(websocket_host, websocket_port, websocket_path);
    webSocket.onEvent(webSocketEvent);
}

// ========== LOOP ==========
void loop() {
  // Maintain WebSocket connection
  webSocket.loop();

  digitalWrite(LED_BUILTIN, HIGH); // keep on for visual indication that board is running

  // Read motion sensor
  int motion = digitalRead(PIR_PIN);
  bool motionDetected = (motion == HIGH); 

  Serial.print("manual control: ");
  Serial.println(manualControl);

  // when manual control, use lightState defined earlier instead of motionDetected
  if (manualControl == true) {
    // Update LEDs based on light state
    digitalWrite(LED_PIN, lightState ? HIGH : LOW);
    digitalWrite(LED_LIGHT, lightState ? HIGH : LOW);
  } else {
    // Update LEDs based on motion
    if (motion == HIGH) {
      Serial.println("Motion detected!");
      digitalWrite(LED_PIN, HIGH); 

      // Send motion update only when state changes
      if (motionDetected != lastMotionState) {
        lastMotionState = motionDetected;
        sendPOSTRequest(true);
      }
  
    } else {
      Serial.println("No motion");
      digitalWrite(LED_PIN, LOW);  
    }
  }
}