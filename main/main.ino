#include <WiFi.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <ESPping.h>

// Wi-Fi Credentials
const char* ssid = "FRD";       // Replace with your Wi-Fi SSID
const char* password = "22311824"; // Replace with your Wi-Fi password

// Hardware Pin Definitions
#define PIR_PIN 21    // Motion sensor
#define LED_PIN 23    // Indicator LED
#define LED_LIGHT 13  // Another LED output

// WebSocket Server (Ngrok)
const char* websocket_host = "a57b-24-200-192-186.ngrok-free.app"; // Ngrok host
const char* websocket_path = "/EspLight/ws/2658FA6D-BEEB-4861-9AF0-B84E2FDBA0EA"; // WebSocket path
const int websocket_port = 443; // Use 443 for WSS, or 80 for WS

// Control Variables
bool manualControl = false;
bool lightState = false;
bool lastMotionState = false;

// WebSocket Client
WebSocketsClient webSocket;

unsigned long previousMillis = 0;  // Store the last time the motion was checked
const long interval = 1000;        // Interval at which to check motion (1 second)

// WebSocket Event Handler
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
                lightState = false;
            }

            else if(msg == "{\"overide\":false,\"state\":0}") {
              manualControl = false;
              lightState = false;
            }
            break;
    }
}

void sendPOSTRequest(bool motion) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://a57b-24-200-192-186.ngrok-free.app/EspLight/2658fa6d-beeb-4861-9af0-b84e2fdba0ea";

    http.begin(url);
    http.addHeader("Content Type", "application/json");

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

void loop() {
    // Maintain WebSocket connection
    webSocket.loop();

    // Read motion sensor
    int motion = digitalRead(PIR_PIN);
    bool motionDetected = (motion == HIGH); 

    // when manual control, use lightState defined earlier instead of motionDetected
    if (manualControl == true) {
        // Update LEDs based on light state
    digitalWrite(LED_PIN, lightState ? HIGH : LOW);
    digitalWrite(LED_LIGHT, lightState ? HIGH : LOW);
    }
    else {
      // Update LEDs based on motion
      if (motion == HIGH) {
        digitalWrite(LED_PIN, HIGH);    // Turn on LED
        digitalWrite(LED_LIGHT, HIGH);  // Turn on additional LED
      }
      else if (motion == LOW) {
        digitalWrite(LED_PIN, LOW);     // Turn off LED
        digitalWrite(LED_LIGHT, LOW);   // Turn off additional LED
      }
    }

     // Check if it's time to send the motion update
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
  }

    // Send motion update only when state changes
    if (!manualControl && motionDetected != lastMotionState) {
        lastMotionState = motionDetected;

        sendPOSTRequest(motionDetected); // Send POST request to log motion event

}
}
