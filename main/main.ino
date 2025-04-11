// ========== LIBRARIES ==========
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <ESPping.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#define DEBUG_WEBSOCKETS

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
const char* host = "0c0f-138-229-30-132.ngrok-free.app"; // Ngrok host
const char* websocket_path = "/EspLight/ws/fb3dcd74-f3b7-4bdb-b58a-626bd998724f";
const int websocket_port = 443; // Use 443 for WSS, or 80 for WS
WebSocketsClient webSocket; // WebSocket Client

// ========== LIGHT OBJECT ==========
class Light {
  public:
    int pin;
    bool overide;
    int state;
    int brightness;
    int assignedSensor;

    Light(int p, int a) {
      pin = p; 
      overide = false;
      state = 0;
      brightness = 255;
      assignedSensor = a;
    }

    void update(int newState, int newBrightness, bool newOveride) {
      state = newState;
      brightness = newBrightness;
      overide = newOveride;
    }

};

// ========== SENSOR OBJECT ==========
class Sensor {
  public:
    int pin;
    int sensitivity;
    int timeout;
    int assignedLight;
    bool manualControl;
    bool lastMotionState;
    unsigned long lastMotionTime;
    unsigned long lastPrintTime;

    Sensor(int p, int a) {
      pin = p;
      sensitivity = 0;
      timeout = 1000;
      assignedLight = a;
      manualControl = false;
      lastMotionState = false;
      lastMotionTime = 0;
      lastPrintTime = 0;
    }

    void update(int newSensitivity, int newTimeout) {
      sensitivity = newSensitivity;
      timeout = newTimeout;
    }
};

// ========== LOCAL LIGHT AND SENSOR OBJECTS ==========
Light availableLights[2] = { Light(LED_PIN1, 0), Light(LED_PIN2, 0) };
Sensor availableSensors[2] = { Sensor(PIR_PIN1, 0), Sensor(PIR_PIN2, 0) };

// ========== REGISTER LIGHTS AND SENSORS (Register Endpoint) ==========
void registerESP() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = "https://" + String(host) + "/EspLight/register/" + espId;
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        // create json object
        StaticJsonDocument<200> doc;
        JsonArray sensorArray = doc.createNestedArray("sensorPins");
        JsonArray lightArray = doc.createNestedArray("lightPins");

        // add pin values
        sensorArray.add(sensorPins[0]);
        sensorArray.add(sensorPins[1]);

        lightArray.add(lightPins[0]);
        lightArray.add(lightPins[1]);

        // json to string
        String jsonPayload;
        serializeJson(doc, jsonPayload);

        Serial.println("Sending Register Request:");
        Serial.println(jsonPayload);

        // send post request
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
    String url = "https://" + String(host) + "/EspLight/movement/" + espId + "/" + String(PIR_PIN)
                  + "?movement=" + (motion ? "true" : "false");

    http.begin(url);
    // http.addHeader("Content-Type", "application/json");

    // String jsonPayload = "{\"movement\": " + String(motion ? "true" : "false") + "}";
    // int httpResponseCode = http.POST(jsonPayload);
    int httpResponseCode = http.POST("");
     http.end();

    if (httpResponseCode > 0) {
      Serial.println("Motion event sent: " + url);
    }
    else {
      Serial.println("Error sending POST request");
    }
  }
}

// ========== PARSE MESSAGE FROM WEBSOCKETS ==========
void parseMessage(JsonDocument& doc) {

  // get lights info
  JsonArray lights = doc["Lights"];
  for (JsonObject light : lights) {
    int pin = light["Pin"];
    bool overide = light["Overide"];
    int state = light["State"];
    int brightness = light["Brightness"];

    for (int i=0; i<2; i++) {
      if (availableLights[i].pin == pin) {
        availableLights[i].overide = overide;
        availableLights[i].state = state;
        availableLights[i].brightness = brightness;

        if (availableSensors[i].assignedLight == pin) {
          availableSensors[i].manualControl = overide;

          // Serial.print("Sensor Pin: ");
          // Serial.print(availableSensors[i]);
          // Serial.print(" set to ");
          // Serial.println("overide");
        }

        Serial.print("Light Pin: ");
        Serial.print(availableLights[i].pin);
        Serial.println(" updated!");
      }
    }
  }

  // get sensors info
  JsonArray sensors = doc["Sensors"];
  for (JsonObject sensor : sensors) {
    int pin = sensor["Pin"];
    int sensitivity = sensor["Sensitivity"];
    int timeout = sensor["Timeout"];

    for (int i=0; i<2; i++) {
      if (availableSensors[i].pin == pin) {
        availableSensors[i].sensitivity = sensitivity;
        availableSensors[i].timeout = timeout;

        // Serial.print("Sensor Pin: ");
        // Serial.print(availableSensors[i].pin);
        // Serial.println(" updated!");
      }
    }
  }

  // get assigned info
  JsonArray assigned = doc["Assigned"];
  for (JsonObject assignment : assigned) {
    int sensorPin = assignment["SensorPin"];
    int lightPin = assignment["LightPin"];

    for (int i=0; i<2; i++) {
      if (availableSensors[i].pin == sensorPin) {
        availableSensors[i].assignedLight = lightPin;
      }
      if (availableLights[i].pin == lightPin) {
        availableLights[i].assignedSensor = sensorPin;
      }
    }

    Serial.print("Combination sensor ");
    Serial.print(sensorPin);
    Serial.print(" and light ");
    Serial.print(lightPin);
    Serial.println(" received!");
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
            webSocket.beginSSL(host, websocket_port, websocket_path);
            break;
        
        case WStype_TEXT:
            Serial.println("--- Received Message: ---");

            // create json message and parse
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, (char*)payload);

            if (error) {
              Serial.print("Failed to parse JSON: ");
              Serial.println(error.f_str());
              return;
            }

            parseMessage(doc);
            break;
      }
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    Serial.println("Starting ESP32...");
    
    Serial.println("Starting WiFiManager...");
    WiFi.mode(WIFI_STA);
    WiFiManager wm;
    //wm.resetSettings(); // for testing
    bool res;
    res = wm.autoConnect("SmartHome390"); // connect to the hotspot SmartHome390

    if(!res) {
        Serial.println("Failed to connect");
        ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }

    // WebSocket Connection
    Serial.println("Connecting to WebSocket: " + String(host) + websocket_path);
    webSocket.beginSSL(host, websocket_port, websocket_path);
    webSocket.onEvent(webSocketEvent);

    // Setup hardware
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIR_PIN1, INPUT);
    pinMode(PIR_PIN2, INPUT);
    pinMode(LED_PIN1, OUTPUT);
    pinMode(LED_PIN2, OUTPUT);

    // Register ESP on the server
    registerESP();
}

// ========== LOOP ==========
void loop() {
  
  webSocket.loop();
  digitalWrite(LED_BUILTIN, HIGH); // keep on for visual indication that board is running

  // parse through the available lights and sensors
  for (int i = 0; i < 2; i++) {

    int motion = digitalRead(availableSensors[i].pin); 
    bool motionDetected = (motion == HIGH);
    int assignedLightPin = availableSensors[i].assignedLight;

    // find index of associated light
    int lightIndex = -1;
    for (int j = 0; j < 2; j++) {
      if (availableLights[j].pin == assignedLightPin) {
        lightIndex = j;
        break;
      }
    }

    if (lightIndex == -1) {
      //Serial.println("No matching light found!");
      continue;
    }

    // control in manual or sensor mode
    if (availableLights[lightIndex].overide) {
      digitalWrite(availableLights[lightIndex].pin, availableLights[lightIndex].state ? HIGH : LOW);
    } else {
      if (motionDetected) {
        digitalWrite(availableLights[lightIndex].pin, HIGH);

        //if (!availableSensors[i].lastMotionState) { // only post when state changes
          sendPOSTRequest(true, availableSensors[i].pin); 
        //}

        availableSensors[i].lastMotionState = true; // update last motion state
        availableSensors[i].lastMotionTime = millis(); // reset timer

      } else {
        unsigned long currentTime = millis();
        unsigned long elapsed = currentTime - availableSensors[i].lastMotionTime;

        if (availableSensors[i].lastMotionState) {
        
          if (currentTime - availableSensors[i].lastPrintTime >= 1000) { // print every second
            Serial.print("Sensor ");
            Serial.print(availableSensors[i].pin);
            Serial.print(" - No motion for ");
            Serial.print(elapsed / 1000.0, 2);
            Serial.println(" seconds");
            availableSensors[i].lastPrintTime = currentTime;
          }

          if (elapsed >= availableSensors[i].timeout) { // if time elapsed
            availableSensors[i].lastMotionState = false; // update last motion state
            digitalWrite(availableLights[lightIndex].pin, LOW); // turn light off
            Serial.println("Light turned OFF due to timeout.");
            //sendPOSTRequest(false, availableSensors[i].pin); 
          }
        }
      }
    }
  }
}