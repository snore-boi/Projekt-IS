#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

const int LED_PIN 5;
const int NUMPIXELS 64,

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "Ali";
const char* password = "123456789";
const char* serverIP = "10.224.33.150"; // DIN DATORS IP
const int serverPort = 8000;

WiFiClient client;
unsigned long lastSyncTime = 0;

const int rowPins[3] = {15, 17, 19};
const int colPins[3] = {22, 23, 2};
bool lastButtonState[64] = {false};

// NYTT: Variabel för att minnas om vi redan har gjort segerdansen
bool hasCelebrated = false; 

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

void sendClick(int index) {
  if (client.connect(serverIP, serverPort)) {
    client.print("CLICK:");
    client.println(index);
    client.stop();
    lastSyncTime = 0; // Tvinga omedelbar uppdatering
  }
}

// --- NY FUNKTION: SEGERDANSEN (FYRVERKERI) ---
void winDance() {
  // En lista med 6 starka färger (Röd, Grön, Blå, Gul, Lila, Cyan)
  uint32_t colors[] = {
    pixels.Color(255, 0, 0), pixels.Color(0, 255, 0), pixels.Color(0, 0, 255), 
    pixels.Color(255, 255, 0), pixels.Color(255, 0, 255), pixels.Color(0, 255, 255)
  };
  
  // Dansa i 15 snabba pulser
  for(int varv = 0; varv < 15; varv++) {
    for(int i = 0; i < NUMPIXELS; i++) {
      // Välj en slumpmässig färg (0 till 5) för varje diod
      pixels.setPixelColor(i, colors[random(0, 6)]); 
    }
    pixels.show();
    delay(80); // Vänta 80 millisekunder (ger en snabb, blinkande effekt)
  }
}
// ---------------------------------------------

void loop() {
  if(WiFi.status() != WL_CONNECTED) return;

  // 1. Scanna knappar
  for(int r = 0; r < 8; r++) {
    digitalWrite(rowPins[r], LOW);
    for(int c = 0; c < 8; c++) {
      int index = r * 8 + c;
      bool isPressed = (digitalRead(colPins[c]) == LOW);

      if(isPressed && !lastButtonState[index]) {
        sendClick(index); 
      }
      lastButtonState[index] = isPressed;
    }
    digitalWrite(rowPins[r], HIGH);
  }

  // 2. Synka med servern
  if (millis() - lastSyncTime > 100) {
    if (client.connect(serverIP, serverPort)) {
      client.println("SYNC");
      unsigned long timeout = millis();
      while (client.available() == 0 && millis() - timeout < 2000) {}

      String response = client.readStringUntil('\n');
      response.trim();

      if (response.startsWith("BOARD:")) {
        String board = response.substring(6); 

        // --- NYTT: KOLLA OM NÅGON VANN ---
        bool someoneWon = (board.indexOf('W') != -1); // Finns det ett 'W' på brädet?
        if (someoneWon && !hasCelebrated) {
          winDance(); // KÖR DANSEN!
          hasCelebrated = true; // Kom ihåg att vi dansat så vi inte fastnar i en evig loop
        } else if (!someoneWon) {
          hasCelebrated = false; // Återställ minnet när Java-spelet rensar brädet ("Spela Igen")
        }
        // ---------------------------------

        // Rita sedan ut brädet som vanligt
        for (int i = 0; i < 64; i++) {
          char p = board.charAt(i);
          if (p == 'R') pixels.setPixelColor(i, pixels.Color(255, 0, 0));
          else if (p == 'B') pixels.setPixelColor(i, pixels.Color(0, 0, 255));
          else if (p == 'W') pixels.setPixelColor(i, pixels.Color(0, 255, 0)); // Lyser fast grönt efter dansen
          else pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        }
        pixels.show();
      }
      client.stop();
    }
    lastSyncTime = millis();
  }
}