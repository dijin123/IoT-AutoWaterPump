#include <ArduinoJson.h>
#include <BlynkSimpleEsp8266.h>
#include <AceButton.h>
#include <Espalexa.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
using namespace ace_button;

ESP8266WebServer server(80);
Espalexa espalexa;

#define BLYNK_TEMPLATE_ID "TMPLLlamQsSx"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "AT6ieyrDj6ZecSJPgaW4iV1lSdSOlGDx"
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
BlynkTimer timer;

// define the GPIO connected with Relays and switches
#define RelayPin1 0   //GPIO0
#define SwitchPin1 1  //TX
//Change the virtual pins according the rooms
#define VPIN_BUTTON_1 V2



int toggleState_1 = 0;  //Define integer to remember the toggle state for relay 1

ButtonConfig config1;
AceButton button1(&config1);

void handleEvent1(AceButton *, uint8_t, uint8_t);

// device names
String Device_1_Name = "Water Pump Switch";

//callback functions
void firstLightChanged(uint8_t brightness);

void relayOnOff() {
  if (toggleState_1 == 0) {
    digitalWrite(RelayPin1, LOW);  // turn on relay 1
    toggleState_1 = 1;
    Serial.println("Device1 ON");
  } else {
    digitalWrite(RelayPin1, HIGH);  // turn off relay 1
    toggleState_1 = 0;
    Serial.println("Device1 OFF");
  }
  delay(100);
}

BLYNK_CONNECTED() {
  // Request the latest state from the server
  Blynk.syncVirtual(VPIN_BUTTON_1);
}

// When App button is pushed - switch the state

BLYNK_WRITE(VPIN_BUTTON_1) {
  toggleState_1 = param.asInt();
  if (toggleState_1 == 1) {
    digitalWrite(RelayPin1, LOW);
  } else {
    digitalWrite(RelayPin1, HIGH);
  }
}

void setCrossOrigin() {
  server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
  server.sendHeader(F("Access-Control-Max-Age"), F("600"));
  server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
  server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));
};

void handleRoot() {
  setCrossOrigin();
  server.send_P(200, "text/html;charset=UTF-8", "<html><body><b>Switch is Working</b></body></html>");
}

void handleGetData() {
  setCrossOrigin();
  DynamicJsonDocument doc(512);
  doc["status"] = toggleState_1;
  String buf;
  serializeJson(doc, buf);
  server.send(200, "application/json", buf);
}

void handleRemoteValue() {
  setCrossOrigin();
  Serial.println("Call Motor API");
  int value = (server.arg(0)).toInt();
  toggleState_1 = value;
  relayOnOff();
  Blynk.virtualWrite(VPIN_BUTTON_1, toggleState_1);  // Update Button Widget
  server.send(200, "text/plain", "OK");
}

void firstLightChanged(uint8_t brightness) {
  Serial.println("ALEXA EVENT 1");
  relayOnOff();
  Blynk.virtualWrite(VPIN_BUTTON_1, toggleState_1);  // Update Button Widget
}


void addDevices() {
  // Define your devices here.//simplest definition, default state off
  espalexa.addDevice(Device_1_Name, firstLightChanged);
  espalexa.begin(&server);
  Serial.println("Alaxa Begin");
}

void button1Handler(AceButton *button, uint8_t eventType, uint8_t buttonState) {
  Serial.println("EVENT1");
  relayOnOff();
  Blynk.virtualWrite(VPIN_BUTTON_1, toggleState_1);  // Update Button Widget
}

void myTimerEvent() {
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(VPIN_BUTTON_1, toggleState_1);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RelayPin1, OUTPUT);
  pinMode(SwitchPin1, INPUT_PULLUP);
  //During Starting all Relays should TURN OFF
  digitalWrite(RelayPin1, HIGH);
  config1.setEventHandler(button1Handler);
  button1.init(SwitchPin1);

  WiFiManager wifiManager;
  //clear the value in EEPROM for already save SSID and Password
  //wifiManager.resetSettings();
  wifiManager.autoConnect("Water Pump Switch Wifi");
  Serial.println("Wifi Connected");
  Serial.print("Local IP Address:");
  Serial.println(WiFi.localIP());
  //add alexa device
  addDevices();
  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_GET, handleRemoteValue);
  server.on("/status", HTTP_GET, handleGetData);
  server.onNotFound([]() {
    if (!espalexa.handleAlexaApiCall(server.uri(), server.arg(0))) {
      server.send(404, "text/plain", "Not found");
    }
  });
  Serial.println("Alexa started");
  //Blynk Setup
  Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID().c_str(), WiFi.psk().c_str());
  Serial.println("Blynk started");
  // Setup a Blynk function to be called every second
  timer.setInterval(3000L, myTimerEvent);
}
void loop() {
  // put your main code here, to run repeatedly:
  if (WiFi.status() == WL_CONNECTED) {
    while (WiFi.status() == WL_CONNECTED) {
      digitalWrite(LED_BUILTIN, LOW);  // turn the LED on (HIGH is the voltage level)
      //Blynk Sync
      Blynk.run();
      timer.run();
      //Manual Switch Control
      button1.check();
      //loop the Alexa
      espalexa.loop();
    }
  } else {
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED off by making the voltage LOW
    delay(200);
  }
}