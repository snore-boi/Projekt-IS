#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

const int LED_PIN = 5;
const int NUMPIXELS = 64;

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



const int rowPins[8] = {19, 17, 15};
const int colPins[8] = {2, 23, 22};
bool lastButtonState[64] = {false};
bool started = true;
bool blinkState = false;


void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixelTurn.begin();
  
  for(int i = 0; i < 8; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH); 
  }

  for(int i = 0; i < 8; i++){
    pinMode(colPins[i], INPUT_PULLUP);
  }

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) { delay(300); }
  Serial.println("Ansluten till WiFi!");
}

void clearBoard(){
  for(int i = 0; i < NUMPIXELS; i++){
    pixels.setPixelColor(i,0);
  }
}

void setLed(int index, int r, int g, int b){
  if(index >= 0 && index < NUMPIXELS){
    pixels.setPixelColor(index, pixels.Color(r,g,b));
  }
}

void parseSelect(String msg){
  int sep = msg.indexOf('|');
  if(sep == -1){
    return;
  }

  String data = msg.substring(sep+1);
  int start = 0;

  while(start < data.length()){
    int comma = data.indexOf(',', start);
  

  String item;
  if(comma == -1){
    item = data.substring(start);

    start = data.length();
  } else{
    item = data.substring(start, comma);
    start = comma +1;
  }

  int dash = item.indexOf('-');

  if(dash == -1){
    continue;
  }

  int index = item.substring(0,dash).toInt();
  char type = item.charAt(dash+1);
  /*
   S = Selected 
   M = move
   C = capture
  */
  if(type == 'S'){
    setLed(index,0,0,255);
  }

  if(type == 'M'){
    setLed(index,0,255,0);
  }

  if(type == 'C'){
    setLed(index,255,0,0);
  }
  }
pixels.show();

}

void parseBoard(String board){
  for(int i = 0; i < 64; i++){
    char p = board.charAt(i);

    if(p == '.'){
      setLed(i,0,0,0);
    }

    // För tre irad
    else if(p == 'X'){
      setLed(i,255,0,0);
    } else if(p == 'O'){
      setLed(i,0,0,255);
    }

    //För chekers
    else if(p == 'R'){
      setLed(i,255,0,0);
    } else if(p == 'B'){
      setLed(i,0,0,255);
    }

    pixels.show();
  }
}

void setTurn(char t){
  if(t == 'R'){
    pixelTurn.setPixelColor(0,pixelTurn.Color(255,0,0));
  }

  else if(t == 'B'){
    pixelTurn.setPixelColor(0,pixelTurn.Color(0,0,255));
  }

  pixelTurn.show();
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
    for (int j = 0; j < 64; j++){
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

  for(int r = 0; r < 8; r++) {
    digitalWrite(rowPins[r], LOW);
    for(int c = 0; c < 8; c++) {
      int index = r * 8 + c;
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

    if(client.connect(serverIP,serverPort)){
      client.println("SYNC");
      String response = client.readStringUntil('\n');
      response.trim();

      if(response.startsWith("SELECT:")){
        clearBoard();

        parseSelect(response.substring(7));
      }

      else if(response.startsWith("BOARD:")){
        parseBoard(response.substring(6));
      }

      else if(response.startsWith("TURN:")){
        setTurn(response.charAt(5));
      }

      client.stop();
    }
  }

  if (millis() - lastBlink > blinkInterval) {
    blinkState = !blinkState;
    lastBlink = millis();
  }

}