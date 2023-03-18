#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#include <HCSR04.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

const int trigPin = 12;
const int echoPin = 14;
long duration;
float distanceCm;
float distanceInch;


//define sound velocity in cm/uS
#define SOUND_VELOCITY 0.034
#define CM_TO_INCH 0.393701
#define LED D0  //Define LED pin D0

/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID           "TMPLLlamQsSx"
#define BLYNK_TEMPLATE_NAME         "Quickstart Template"
#define BLYNK_AUTH_TOKEN            "AT6ieyrDj6ZecSJPgaW4iV1lSdSOlGDx"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "";
char pass[] = "";

BlynkTimer timer;

// This function is called every time the Virtual Pin 0 state changes
BLYNK_WRITE(V0)
{
  // Set incoming value from pin V0 to a variable
  int value = param.asInt();

  // Update state
  Blynk.virtualWrite(V1, value);
}

// This function is called every time the device is connected to the Blynk.Cloud
BLYNK_CONNECTED()
{
  // Change Web Link Button message to "Congratulations!"
  Blynk.setProperty(V3, "offImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations.png");
  Blynk.setProperty(V3, "onImageUrl",  "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations_pressed.png");
  Blynk.setProperty(V3, "url", "https://docs.blynk.io/en/getting-started/what-do-i-need-to-blynk/how-quickstart-device-was-made");
}

// This function sends Arduino's uptime every second to Virtual Pin 2.
void myTimerEvent()
{
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V1, distanceCm);
  //Blynk.virtualWrite(V2, millis() / 1000);
}


// Water level WebPage
const char index_html[] PROGMEM={"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.6.3/jquery.min.js\"></script>\n"
"<script>\n"
"$( document ).ready(function() {\n"
"       const gaugeElement = document.querySelector(\".gauge\");\n"
"        console.log( \"document loaded\" );\n"
"        setGaugeValue(gaugeElement, 0.0);\n"
"        setInterval(function()\n"
"{           $.ajax({ \n"
"                type: \"GET\",\n"
"                crossDomain: true,\n"
"                url: \"/level\",\n"
"                success: function(data){  \n"
"                    console.log(data);      \n"
"                    setGaugeValue(gaugeElement, (data/100));\n"
"                }\n"
"            });\n"
"        }, 1000);//time in milliseconds\n"
"    });\n"
 
"function setGaugeValue(gauge, value) {\n"
"  if (value < 0 || value > 1) {\n"
"    return;\n"
"  }\n"
"  gauge.querySelector(\".gauge__fill\").style.transform = `rotate(${\n"
"    value / 2\n"
"  }turn)`;\n"
"  gauge.querySelector(\".gauge__cover\").textContent = `${Math.round(\n"
"    value * 100\n"
"  )}%`;\n"
"}\n"
"</script>\n"
"<style>\n"
".gauge {width: 100%;max-width: 250px;font-family: \"Roboto\", sans-serif;font-size: 32px;color: #004033;} \n"
".gauge__body {width: 100%;height: 0;padding-bottom: 50%;background: #b4c0be;position: relative;border-top-left-radius: 100% 200%;border-top-right-radius: 100% 200%;overflow: hidden;}\n"
".gauge__fill {position: absolute;top: 100%;left: 0;width: inherit;height: 100%;background: #009578;transform-origin: center top;transform: rotate(0.25turn);transition: transform 0.2s ease-out;}\n"
".gauge__cover {width: 75%;height: 150%;background: #ffffff;border-radius: 50%;position: absolute;top: 25%;left: 50%;transform: translateX(-50%);display: flex;align-items: center;justify-content: center;padding-bottom: 25%;box-sizing: border-box;} \n"
"</style>\n"
"</head>\n"
"<body>\n"

"<div class=\"gauge\">\n"
"    <div class=\"gauge__body\">\n"
"      <div class=\"gauge__fill\"></div>\n"
"      <div class=\"gauge__cover\"></div>\n"
"    </div>\n"
"  </div>\n"

"</body>\n"
"</html>\n"};

#define MAX_HEIGHT 27  // tank height 27 cm manually enter here for automatic empty tank
#define MOTOR_CONTROL_PIN D4 

UltraSonicDistanceSensor distanceSensor(D6,D5);  //D1 trig, D2=echo

int waterLevelLowerThreshold = 15;
int waterLevelUpperThreshold = 90;
float volume = 0;
float liters = 0;
WiFiClient client;
String inputString = "";  // a string to hold incoming data
String dataToSend = "";
int waterLevelDownCount = 0, waterLevelUpCount = 0;
ESP8266WebServer server(80);

void handleRoot() {
  server.send_P(200, "text/html;charset=UTF-8", index_html);
}


void handleLevelRequest() {
  server.enableCORS(true);
  server.send(200, "text", String(liters));
}


void handleNotFound() {
  String message = "File Not Found\n\n";
  server.send(404, "text/plain", message);
}

void handleStatus() {
  server.enableCORS(true);
  if (digitalRead(MOTOR_CONTROL_PIN) == 0)  //MOTOR ON
  server.send(200, "text/plain", "on");
  else server.send(200, "text/plain", "off");
}

void handleRangeSetting() {
  waterLevelLowerThreshold = (server.arg(0)).toInt();
  waterLevelUpperThreshold = (server.arg(1)).toInt();
  Serial.print(waterLevelLowerThreshold);
  Serial.print(":");
  Serial.println(waterLevelUpperThreshold);
  server.enableCORS(true);
  server.send(200, "text/plain", "");
}


void measure_Volume()
{
  float heightInch = 1 * distanceSensor.measureDistanceCm();
  //float heightInch = 1 * distanceCm; 
  Serial.print("Height in Inch HTML : ");
  Serial.println(heightInch);
  volume = (MAX_HEIGHT - heightInch) / 28;  //MAX_HEIGHT-distance will give actual height, 1 cm for   // offset adjustment
  liters = volume * 100;  // for percentage
  Serial.println(liters);
  if (liters <= waterLevelLowerThreshold)
    waterLevelDownCount++;
  else waterLevelDownCount = 0;
  if (liters >= waterLevelUpperThreshold)
    waterLevelUpCount++;
  else waterLevelUpCount = 0;
  if (waterLevelDownCount == 3)
  {  //TURN ON RELAY
    Serial.println("motor turned on");
    //digitalWrite(MOTOR_CONTROL_PIN, LOW);  //Relay is active LOW
  }
  if (waterLevelUpCount == 3)
  {  //TURN OFF RELAY
    Serial.println("motor turned off");
    //digitalWrite(MOTOR_CONTROL_PIN, HIGH);  //Relay is active LOW
  }
}

void runPeriodicFunc()
{
  static const unsigned long REFRESH_INTERVAL1 = 1000;  // 2.1sec
  static unsigned long lastRefreshTime1 = 0;
  if (millis() - lastRefreshTime1 >= REFRESH_INTERVAL1)
  {
    measure_Volume();
    lastRefreshTime1 = millis();
  }
}


void setup() {
  Serial.begin(9600);        // Starts the serial communication
  pinMode(trigPin, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);   // Sets the echoPin as an Input

  // Onboard LED Setup
  pinMode(LED, OUTPUT);  //Onbard LED

  //Wifi Manager Setup
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  wifiManager.autoConnect("DNA WiFi Manager");
  Serial.println("connected :)");

  //WebServer Setup
  Serial.print("IP address:");
  Serial.println(WiFi.localIP());
  server.on("/", handleRoot);
  server.on("/level", handleLevelRequest);
  server.on("/configRange", handleRangeSetting);
  server.on("/motor_status", handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  Serial.print("WIFI Settings : ");  
  Serial.print(WiFi.SSID().c_str()); 
  Serial.println(WiFi.psk().c_str()); 
  Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID().c_str(), WiFi.psk().c_str());
  Serial.println("Blynk started...");  
  // Setup a function to be called every second
  timer.setInterval(1000L, myTimerEvent);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {

    digitalWrite(LED, LOW);  // LED ON
    while (WiFi.status() == WL_CONNECTED) {

      //WebServer Start
      runPeriodicFunc();
      server.handleClient();

      //LED Function
      digitalWrite(LED, HIGH);
      delay(500);
      digitalWrite(LED, LOW);
      delay(200);

      // Ultrsonic Sensor Code
      // Clears the trigPin
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      // Sets the trigPin on HIGH state for 10 micro seconds
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);

      // Reads the echoPin, returns the sound wave travel time in microseconds
      duration = pulseIn(echoPin, HIGH);

      // Calculate the distance
      distanceCm = duration * SOUND_VELOCITY / 2;

      // Convert to inches
      distanceInch = distanceCm * CM_TO_INCH;

      // Prints the distance on the Serial Monitor
      //Serial.print("Distance (cm): ");
      //Serial.println(distanceCm);
      Serial.print("Distance (inch): ");
      Serial.println(distanceInch);
      Blynk.run();
      timer.run();
      

      delay(1000);
    }
  } else {
    digitalWrite(LED, HIGH);  // LED OFF
  }
}