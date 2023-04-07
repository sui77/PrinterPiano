#include "Adafruit_Keypad.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>


// ============================

// Your WiFi credentials
const char* wifi_ssid = "SSID";
const char* wifi_pass = "PASSWORD";

// Your octoprint api settings
const char* octoprint_api_url  = "https://HOSTNAME/api/printer/command";
const char* octoprint_host     = "HOSTNAME";
const char* octoprint_api_key  = "API_KEY";

// Range to move the axis
String positionX[2] = { "10", "50" };
String positionY[2] = { "10", "50" };


// ============================


int MODE_INTERNAL       = 0;
int MODE_OCTOPRINT_BEEP = 1;
int MODE_BOTH           = 2;
int MODE_OCTOPRINT_X    = 3;
int MODE_OCTOPRINT_Y    = 4;

// define the internal buzzer pin
byte internalBuzzerPin = D0;

// define the keypad
char keys[4][4] = {
  {0x0, 0x4, 0x8, 0xC},
  {0x1, 0x5, 0x9, 0xD},
  {0x2, 0x6, 0xA, 0xE},
  {0x3, 0x7, 0xB, 0xF}
};
byte colPins[4] = {D8, D7, D6, D5};
byte rowPins[4] = {D1, D2, D3, D4};
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, 4, 4);

float frequencies[13] = {
   65.4064,  // C
   73.4162,  // D
   82.4069,  // E
   87.3071,  // F
   97.9989,  // G
  110.0000,  // A
  123.4708,  // B
  130.8128,  // C
   69.2957,  // C#
   77.7818,  // D#
   92.4986,  // F#
  103.8262,  // G#
  116.5409   // A#
};


int mode = MODE_OCTOPRINT_X;
int octave = 2;
int currentPos = 0;

WiFiClientSecure client;
HTTPClient http;


void setup() {
    Serial.begin(74880);

    // WiFi Connection
    WiFi.begin(wifi_ssid, wifi_pass);
    Serial.println("Connecting to WiFi ");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Keypad Init
    customKeypad.begin();


    client.setInsecure();
}

void loop() {

  customKeypad.tick();
  while(customKeypad.available()){
      keypadEvent e = customKeypad.read();
      
      if(e.bit.EVENT == KEY_JUST_PRESSED) {
          Serial.print("Key pressed: ");
          Serial.println(e.bit.KEY);
          Serial.print(octave); Serial.print("-"); Serial.println(mode);
          if (e.bit.KEY >=0 && e.bit.KEY <= 0xC) {

              // play a tone
              playTone(e.bit.KEY);

          } else if (e.bit.KEY == 0xD) {

              // octave down
              if (octave > 0) {
                  octave--;
              }

          } else if (e.bit.KEY == 0xE) {
              
              // octave down
              if (octave < 6) {
                  octave++;
              }


          } else if (e.bit.KEY == 0xF) {
              
              // change mode
              mode++;
              if (mode > MODE_OCTOPRINT_Y) {
                  mode = 0;
              }

              for (int i = 0; i<= mode; i++) {
                tone(internalBuzzerPin, 800, 100);
                delay(150);
              }

          }
      } 
  }
}

void playTone(byte number) {
  
  unsigned int f = round(frequencies[number]*pow(2,octave));
  Serial.print("playing ");
  Serial.print(f);
  Serial.println("Hz");

  if (mode == MODE_INTERNAL || mode == MODE_BOTH) {
      tone(internalBuzzerPin, f, 500);
  }
  if (mode == MODE_OCTOPRINT_BEEP || mode == MODE_BOTH) {
      String cmd = "{\"commands\": [\"M300 P500 S" + String(f) + "\"]}";
      sendCommand(cmd);
  }
  if (mode == MODE_OCTOPRINT_X) {
      String cmd = "{\"commands\": [\"G0 X" + positionX[currentPos] + " F" + String(f) + "" + "\"]}";
      if (currentPos == 1) { currentPos = 0; } else { currentPos = 1; }
      sendCommand(cmd);
  }
  if (mode == MODE_OCTOPRINT_Y) {
      String cmd = "{\"commands\": [\"G0 Y" + positionX[currentPos] + " F" + String(f) + "" + "\"]}";
      if (currentPos == 1) { currentPos = 0; } else { currentPos = 1; }
      sendCommand(cmd);
  }  
}

void sendCommand(String command) {
  if ((WiFi.status() == WL_CONNECTED)) {
    Serial.print("[HTTP] begin...\n");
    http.begin(client, octoprint_api_url);
    http.addHeader("Host", octoprint_host);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Api-Key", octoprint_api_key);

    int httpCode = http.POST(command);

    if (httpCode > 0) {
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }

}

