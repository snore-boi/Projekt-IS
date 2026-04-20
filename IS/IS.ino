#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

const int LED_PIN = 5;
const int NUMPIXELS = 9;

const int TURN_PIN = 4;
const int NUMPIXEL = 1;



Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelTurn(NUMPIXEL, TURN_PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "Ali";
const char* password = "123456789";
const char* serverIP = "10.224.33.11";
const int serverPort = 9500;

WiFiClient client;

const unsigned long updateInterval = 100;
unsigned long lastUpdate = 0;

const unsigned long blinkInterval = 750;
unsigned long lastBlink = 0;



const int rowPins[3] = {19, 17, 15};
const int colPins[3] = {2, 23, 22};
bool lastButtonState[9] = {false};
bool started = true;
bool blinkState = false;


void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixelTurn.begin();
  
  for(int i = 0; i < 3; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH); 
    pinMode(colPins[i], INPUT_PULLUP);
  }

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) { delay(1000); }
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
  for (int i = 0; i < 5; i++){
    for (int j = 0; j < 9; j++){
      pixels.setPixelColor(j, pixels.Color(random(255), random(255), random(255)));
    }
    pixels.show();
    delay(500);
  }
  started = false;
}

  

void loop() {
  if(started){
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
      client.println("turn");

      unsigned long timeout = millis();
      while (client.available() == 0 && millis() - timeout < 2000) {}

      String response = client.readStringUntil('\n');
      if(response.equals("R")){
        pixelTurn.setPixelColor(0, pixelTurn.Color(255,0,0));
      }
      else {
        pixelTurn.setPixelColor(0, pixelTurn.Color(0,0,255));
      }

    }

    if (client.connect(serverIP, serverPort)) {
      client.println("update1");

      unsigned long timeout = millis();
      while (client.available() == 0 && millis() - timeout < 2000) {}

      String response = client.readStringUntil('\n');
      String second_response = client.readStringUntil('\n');

      for (int i = 0; i < 9; i++) {
        char p = response.charAt(i);
        char q = second_response.charAt(i);

        if (p == 'R') {
          pixels.setPixelColor(i, pixels.Color(255, 0, 0));
        } else if (p == 'B') {
          pixels.setPixelColor(i, pixels.Color(0, 0, 255));
        } else {
          pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        }
        if (blinkState && q == '1' ){
          pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        }
      }
      pixels.show();
      pixelTurn.show();
      client.stop();
    }
  }

  if (millis() - lastBlink > blinkInterval) {
    blinkState = !blinkState;
    lastBlink = millis();
  }

}