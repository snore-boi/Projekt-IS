// TEst demo
// TEst demo TEST

#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

const int LED_PIN = 5;
const int TURN_PIN = 4;

const char* ssid = "Ali";
const char* password = "123456789";
const char* serverIP = "10.224.33.11";
const int serverPort = 9500;

WiFiClient client;

// ========== VARIABLER ==========
Adafruit_NeoPixel* pixels = nullptr;      // Dynamisk NeoPixel, pekare som kan ändras
Adafruit_NeoPixel pixelTurn(1, TURN_PIN, NEO_GRB + NEO_KHZ800);

String currentGame = "";   // "TICTACTOE", "CHESS", eller "CHECKERS"
int boardSize = 0;          // 3 eller 8 rutor
int numPixels = 0;          // 9 eller 64 lampor

// Pekare för pins (dessa ska peka på rätt array)
const int* rowPins = nullptr;
const int* colPins = nullptr;

// Dynamiska arrayer för knappstate
bool* lastButtonState = nullptr;
unsigned long* lastDebounceTime = nullptr;

const unsigned long updateInterval = 100;
unsigned long lastUpdate = 0;

const unsigned long blinkInterval = 750;
unsigned long lastBlink = 0;
bool blinkState = false;
bool started = true;

// Pins för 8x8 spel (för schack/checkers)
const int rowPins8x8[8] = {13, 14, 15, 16, 17, 18, 19, 21};
const int colPins8x8[8] = {25, 26, 27, 32, 33, 34, 35, 36};

// Pins för 3x3 (används för tic tac toe)
const int rowPins3x3[3] = {13, 14, 15};
const int colPins3x3[3] = {25, 26, 27};

const unsigned long debounceDelay = 50;

// ========== FUNKTIONER ==========

String readResponse() {
  String response = "";
  unsigned long timeout = millis() + 1000;
  while (millis() < timeout) {
    if (client.available()) {
      response = client.readStringUntil('\n');
      break;
    }
  }
  response.trim();
  return response;
}

void clearBoard() {
  for (int i = 0; i < numPixels; i++) {
    pixels->setPixelColor(i, 0);
  }
}

void setLed(int index, int r, int g, int b) {
  if (index >= 0 && index < numPixels) {
    pixels->setPixelColor(index, pixels->Color(r, g, b));
  }
}

void setupPinsForGame() { //Konfigurera om hårdvaran
  if (currentGame == "TICTACTOE") {
    boardSize = 3;
    numPixels = 9;
    rowPins = rowPins3x3;
    colPins = colPins3x3;
    
    // Konfigurera 3x3 pins
    for (int i = 0; i < 3; i++) {
      pinMode(rowPins[i], OUTPUT);
      digitalWrite(rowPins[i], HIGH);
      pinMode(colPins[i], INPUT_PULLUP);
    }
  } 
  else {  // CHESS eller CHECKERS
    boardSize = 8;
    numPixels = 64;
    rowPins = rowPins8x8;
    colPins = colPins8x8;
    
    // Konfigurera 8x8 pins
    for (int i = 0; i < 8; i++) {
      pinMode(rowPins[i], OUTPUT);
      digitalWrite(rowPins[i], HIGH);
      pinMode(colPins[i], INPUT_PULLUP);
    }
  }

  // Allokera minne för knapparrayer
  int totalButtons = boardSize * boardSize;
  //Om vi redan har reserverat minne för knapparna, frigör det först
  if (lastButtonState != nullptr) delete[] lastButtonState; 
  if (lastDebounceTime != nullptr) delete[] lastDebounceTime;
  
  //Reservera sedan nytt minne för rätt antal knappar
  lastButtonState = new bool[totalButtons]();
  lastDebounceTime = new unsigned long[totalButtons]();
}

//återskapa displayen
void initDisplay() { 
  // Ta bort gammal NeoPixel om den finns
  if (pixels != nullptr) {
    delete pixels;
  }
  
  // Skapa ny NeoPixel med rätt storlek
  pixels = new Adafruit_NeoPixel(numPixels, LED_PIN, NEO_GRB + NEO_KHZ800);
  pixels->begin();
  clearBoard();
  pixels->show();
}

void parseSelect(String msg) {
  int sep = msg.indexOf('|');
  if (sep == -1) {
    return;
  }

  String data = msg.substring(sep + 1);
  int start = 0;

  while (start < data.length()) {
    int comma = data.indexOf(',', start);
    String item;

    if (comma == -1) {
      item = data.substring(start);
      start = data.length();
    } else {
      item = data.substring(start, comma);
      start = comma + 1;
    }

    int dash = item.indexOf('-');

    if (dash == -1) {
      continue;
    }

    int index = item.substring(0, dash).toInt();
    char type = item.charAt(dash + 1);
    /*
     S = Selected 
     M = move
     C = capture
    */
    if (type == 'S') setLed(index, 0, 0, 255);      // Selected - blå
    else if (type == 'M') setLed(index, 0, 255, 0); // Move - grön
    else if (type == 'C') setLed(index, 255, 0, 0); // Capture - röd
  }
  pixels->show(); // engång efter alla ändringar
}

void parseBoard(String board) {
  clearBoard();

  if (currentGame == "TICTACTOE") {
    // 3x3 bräde: förväntar sig 9 tecken som "XOXOXOXO."
    for (int i = 0; i < 9 && i < board.length(); i++) {
      char p = board.charAt(i);
      if (p == 'X') setLed(i, 255, 0, 0);      // Röd för X
      else if (p == 'O') setLed(i, 0, 0, 255); // Blå för O
      else setLed(i, 0, 0, 0);                 // Tom
    }
  } 
  else if (currentGame == "CHECKERS") {
    // Checkers (8x8)  
    for (int i = 0; i < 64 && i < board.length(); i++) {
      char p = board.charAt(i);
      if (p == 'R' || p == 'X') setLed(i, 255, 0, 0);    // Röd pjäs
      else if (p == 'B' || p == 'O') setLed(i, 0, 0, 255); // Blå pjäs
      else if (p == 'K') setLed(i, 255, 255, 0);         // Kung (gul)
      else setLed(i, 0, 0, 0);
    }
  }
  else { // CHESS
    for (int i = 0; i < 64 && i < board.length(); i++) {
      char p = board.charAt(i);
      if (p == '.') setLed(i, 0, 0, 0);
      else if (p == 'R' || p == 'N' || p == 'B' || p == 'Q' || p == 'K' || p == 'P') {
        setLed(i, 255, 0, 0);  // Vita pjäser (röd)
      }
      else if (p == 'r' || p == 'n' || p == 'b' || p == 'q' || p == 'k' || p == 'p') {
        setLed(i, 0, 0, 255);  // Svarta pjäser (blå)
      }
    }
  }
  pixels->show();
}

void setTurn(char t) {  
  if (t == 'R' || t == 'W' || t == 'X') {
    pixelTurn.setPixelColor(0, pixelTurn.Color(255, 0, 0));  // Röd tur
  }
  else if (t == 'B' || t == 'O') {
    pixelTurn.setPixelColor(0, pixelTurn.Color(0, 0, 255));  // Blå tur
  }
  pixelTurn.show();
}

void sendClick(String message) {
  if (client.connect(serverIP, serverPort)) {
    client.println(message);
    if (client.available()) {
      client.readStringUntil('\n');
    }
    client.stop();
  }
}

void startShow() {
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < numPixels; j++) {
      pixels->setPixelColor(j, pixels->Color(random(255), random(255), random(255)));
    }
    pixels->show();
    delay(500);
  }
  started = false;
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  pixelTurn.begin();
  
  // Anslut till WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(300); 
  }
  Serial.println("Ansluten till WiFi!");
  
  // Fråga servern vilket spel som spelas
  if (client.connect(serverIP, serverPort)) {
    client.println("GET_GAME");
    currentGame = readResponse();
    client.stop();
    Serial.print("Spel: ");
    Serial.println(currentGame);
  } else {
    currentGame = "CHESS";  // Default
  }
  
  // Sätt upp pins baserat på spelet
  setupPinsForGame();
  
  // Initiera displayen
  initDisplay();
}

// ========== LOOP ==========
void loop() {
  if (started) {
    startShow();
  }

  if (WiFi.status() != WL_CONNECTED) return;

  // Knappavläsning (anpassad efter brädstorlek)
  for (int r = 0; r < boardSize; r++) {  // ÄNDRAT: använd boardSize, inte 8!
    digitalWrite(rowPins[r], LOW);
    delayMicroseconds(50);

    for (int c = 0; c < boardSize; c++) {  // ÄNDRAT: använd boardSize
      int index = r * boardSize + c;
      bool isPressed = (digitalRead(colPins[c]) == LOW);

      if (isPressed && !lastButtonState[index]) {
        if (millis() - lastDebounceTime[index] > debounceDelay) {
          String button = String(r) + ":" + String(c);
          sendClick(button);
          lastDebounceTime[index] = millis();
        }
      }
      lastButtonState[index] = isPressed;
    }
    digitalWrite(rowPins[r], HIGH);
    delayMicroseconds(50);
  }
  
  // Synkronisering med server
  if (millis() - lastUpdate > updateInterval) {//frågar servern 10 ggr/sekund
    lastUpdate = millis();

    if (client.connect(serverIP, serverPort)) {
      client.println("SYNC");
      String response = readResponse();

      if (response.startsWith("GAME:")) {
        String newGame = response.substring(5);
        if (newGame != currentGame) {
          currentGame = newGame;
          setupPinsForGame();
          initDisplay();
        }
      }
      else if (response.startsWith("SELECT:")) {
        parseSelect(response.substring(7)); //Markera möjliga drag
      }
      else if (response.startsWith("BOARD:")) {
        parseBoard(response.substring(6)); // visa hela brädet
      }
      else if (response.startsWith("TURN:")) {
        setTurn(response.charAt(5)); //visa vems tur
      }

      client.stop();
    }
  }

  // Blinking 
  if (millis() - lastBlink > blinkInterval) {
    blinkState = !blinkState;
    lastBlink = millis();
  }
}
