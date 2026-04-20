#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

const int LED_PIN = 5;
const int NUMPIXELS = 9;

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "Gabriel - IPhone (2)";
const char* password = "ottmar123";
const char* serverIP = "172.20.10.8"; // DIN DATORS IP
const int serverPort = 8500;

WiFiClient client;

const unsigned long updateInterval = 200;
unsigned long lastUpdate = 0;

const int rowPins[3] = {15, 17, 19};
const int colPins[3] = {22, 23, 2};
bool lastButtonState[9] = {false};
bool started = true;


void setup() {
  Serial.begin(115200);
  pixels.begin();
  
  for(int i = 0; i < 3; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH); 
    pinMode(colPins[i], INPUT_PULLUP);
  }

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) { delay(500); }
  Serial.println("Ansluten till WiFi!");
}

void sendClick(String message) {
  if (client.connect(serverIP, serverPort)) {
    client.println(message);
    client.readStringUntil('\n');
    client.stop();
  }
}

void startShow(){
  for (int i; i > 5; i++){
    for (int j; j > 9; j++){
      pixels.setPixelColor(j, pixels.Color(random(255), random(255), random(255)));
    }
    pixels.show();
    delay(1000);
  }
  started = false;
}

  

void loop() {
  if (started){
    startShow();
  }
  
  if(WiFi.status() != WL_CONNECTED) return;

  for(int r = 0; r < 3; r++) {
    digitalWrite(rowPins[r], LOW);
    for(int c = 0; c < 3; c++) {
      int index = r * 3 + c;
      String button = String(r) + ":" + String(c);
      bool isPressed = (digitalRead(colPins[c]) == LOW);

      if(isPressed && !lastButtonState[index]) {
        sendClick(button);
      }
      lastButtonState[index] = isPressed;
    }
    digitalWrite(rowPins[r], HIGH);
  }

  if (millis() - lastUpdate > updateInterval) {
    lastUpdate = millis();

    if (client.connect(serverIP, serverPort)) {
      client.println("update");

      unsigned long timeout = millis();
      while (client.available() == 0 && millis() - timeout < 2000) {}

      String response = client.readStringUntil('\n');

      for (int i = 0; i < 9; i++) {
        char p = response.charAt(i);

        if (p == 'R') {
          pixels.setPixelColor(i, pixels.Color(255, 0, 0));
        } else if (p == 'B') {
          pixels.setPixelColor(i, pixels.Color(0, 0, 255));
        } else {
          pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        }
      }

      pixels.show();
      client.stop();
    }
  }
}