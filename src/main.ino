/*
  COEN/ELEC 390 Team 11
  Code that turns on the LED when motion is detected by the PIR motion sensor.
*/

#define PIR_PIN 21    // Connected to the A pin of the sensor
#define LED_PIN 23    

void setup() {
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT); // Only for visual cue that board is on
    pinMode(PIR_PIN, INPUT);      // To detect motion
    pinMode(LED_PIN, OUTPUT);     // To light up when motion is detected
}

void loop() {
    digitalWrite(LED_BUILTIN, HIGH); // Keep built in LED always on

    int motion = digitalRead(PIR_PIN);  // Read sensor

    // When motion is detected, sensor returns '0'
    if (motion == LOW) {
        Serial.println("Motion detected!");
        digitalWrite(LED_PIN, HIGH); 
    } else {
        Serial.println("No motion");
        digitalWrite(LED_PIN, LOW);  
    }

    delay(500);  // Arbitrary
}
