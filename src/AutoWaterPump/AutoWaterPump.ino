#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <EEPROM.h>

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
#define BLYNK_TEMPLATE_ID "TMPLLlamQsSx"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "AT6ieyrDj6ZecSJPgaW4iV1lSdSOlGDx"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

BlynkTimer timer;

// This function is called every time the Virtual Pin 0 state changes
BLYNK_WRITE(V0) {
  // Set incoming value from pin V0 to a variable
  int value = param.asInt();

  // Update state
  Blynk.virtualWrite(V1, value);
}

// This function is called every time the device is connected to the Blynk.Cloud
BLYNK_CONNECTED() {
  // Change Web Link Button message to "Congratulations!"
  Blynk.setProperty(V3, "offImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations.png");
  Blynk.setProperty(V3, "onImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations_pressed.png");
  Blynk.setProperty(V3, "url", "https://docs.blynk.io/en/getting-started/what-do-i-need-to-blynk/how-quickstart-device-was-made");
}

// Water Dashboard WebPage
const char index_html[] PROGMEM = R"=====( 
<!DOCTYPE html>
<html>
<head>
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.3/jquery.min.js"></script>
  <link rel="stylesheet" href="https://fonts.googleapis.com/icon?family=Material+Icons">
  <link rel="stylesheet" href="https://code.getmdl.io/1.3.0/material.indigo-pink.min.css">
  <script defer src="https://code.getmdl.io/1.3.0/material.min.js"></script>
  <script>
    $(document).ready(function () {
      const gaugeElement = document.querySelector(".gauge");
      console.log("document loaded");
      //Get all data from the IoT board
      $.ajax({
        type: "GET",
        url: "/getdata",
        success: function (data) {
          $('input[name="txtwaterLower"]')[0].parentElement.MaterialTextfield.change(data.tankLower);
          $('input[name="txtwaterUpper"]')[0].parentElement.MaterialTextfield.change(data.tankUpper);
          $('input[name="txtTinxyKey"]')[0].parentElement.MaterialTextfield.change(data.tinxyKey);
          $('input[name="txtTankHeight"]')[0].parentElement.MaterialTextfield.change(data.tankheight);
          $('input[name="txtTinxyAPIKey"]')[0].parentElement.MaterialTextfield.change(data.tinxyAPIKey);
          if (data.motorSatus == 0) {
            var myCheckbox = document.getElementById('switch-1');
            myCheckbox.parentElement.MaterialSwitch.off();
            $('.mdl-switch input[type="checkbox"]').next().text("Motor Off");
          }
          else {
            var myCheckbox = document.getElementById('switch-1');
            myCheckbox.parentElement.MaterialSwitch.on();
            $('.mdl-switch input[type="checkbox"]').next().text("Motor On");
          }
        }
      });
      //Handle the Motor toggle button change
      $('.mdl-switch input[type="checkbox"]').change(function () {
        var status = 0;
        if ($(this).is(':checked')) {
          $(this).next().text("Motor On");
          status = 1;
        }
        else {
          $(this).next().text("Motor Off");
          status = 0;
        }
        //Call the Motor Toggle API in IoT 
        $.ajax(
          {
            type: "GET",
            url: '/toggle?',
            data: "motorsatus=" + status,
            success: function (result) {
              $("#result").html(result);
            }
          });
      });
      // Gauge Display 
      setInterval(function () {
        $.ajax({
          type: "GET",
          crossDomain: true,
          url: "/level",
          success: function (data) {
            console.log(data);
            var value = (data / 100);
            if (value < 0 || value > 1) {
              return;
            }
            gaugeElement.querySelector(".gauge__fill").style.transform = `rotate(${value / 2
              }turn)`;
            gaugeElement.querySelector(".gauge__cover").textContent = `${Math.round(
              value * 100
            )}%`;
          }
        });
        // Check motor status from the IoT 
        $.ajax(
          {
            type: "GET",
            url: '/motorstatus',
            success: function (result) {
              if (result == "on") {
                var myCheckbox = document.getElementById('switch-1');
                myCheckbox.parentElement.MaterialSwitch.on();
                $('.mdl-switch input[type="checkbox"]').next().text("Motor On");
              }
              else {
                var myCheckbox = document.getElementById('switch-1');
                myCheckbox.parentElement.MaterialSwitch.off();
                $('.mdl-switch input[type="checkbox"]').next().text("Motor Off");
              }
            }
          });
      }, 1000);//time in milliseconds

      // Save the in data to IoT 
      $("#btnSave").click(function () {
        var waterLower = $('input[name="txtwaterLower"]').val();
        var waterUpper = $('input[name="txtwaterUpper"]').val();
        var tinxyKey = $('input[name="txtTinxyKey"]').val();
        var TankHeight = $('input[name="txtTankHeight"]').val();
        var tinxyAPIKey = $('input[name="txtTinxyAPIKey"]').val();

        $.ajax(
          {
            type: "GET",
            url: '/configRange?',
            data: "lower=" + waterLower + "&upper=" + waterUpper + "&Height=" + TankHeight + "&key=" + tinxyKey + "&APIKey=" + tinxyAPIKey,
            success: function (result) {
              $("#result").html(result);
              alert("Data Saved Successfuly!!!");
            }
          });
      });
    });
  </script>
  <style>
    .gauge {
      width: 100%;
      max-width: 250px;
      font-family: "Roboto", sans-serif;
      font-size: 32px;
      color: #004033;
    }

    .gauge__body {
      width: 100%;
      height: 0;
      padding-bottom: 50%;
      background: #b4c0be;
      position: relative;
      border-top-left-radius: 100% 200%;
      border-top-right-radius: 100% 200%;
      overflow: hidden;
    }

    .gauge__fill {
      position: absolute;
      top: 100%;
      left: 0;
      width: inherit;
      height: 100%;
      background: #009578;
      transform-origin: center top;
      transform: rotate(0.25turn);
      transition: transform 0.2s ease-out;
    }

    .gauge__cover {
      width: 75%;
      height: 150%;
      background: #ffffff;
      border-radius: 50%;
      position: absolute;
      top: 25%;
      left: 50%;
      transform: translateX(-50%);
      display: flex;
      align-items: center;
      justify-content: center;
      padding-bottom: 25%;
      box-sizing: border-box;
    }
  </style>
</head>

<body>
  <br>
  <table>
    <tr>
      <td colspan="2">
        <label class="mdl-switch mdl-js-switch mdl-js-ripple-effect" for="switch-1">
          <input type="checkbox" id="switch-1" class="mdl-switch__input">
          <span class="mdl-switch__label">Motor Off</span>
        </label>
      </td>
    </tr>
    <tr>
      <td colspan="2" align="center">
        <label class="mdl-layout-title">Water Level </label>
        <br>
        <div class="gauge">
          <div class="gauge__body">
            <div class="gauge__fill"></div>
            <div class="gauge__cover"></div>
          </div>
        </div>
      </td>
    </tr>
    <tr>
      <td colspan="2"><span class="mdl-layout-title">Settings :</span></td>
    </tr>
    <tr>
      <td>
        <div class="mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
          <input class="mdl-textfield__input" type="text" pattern="-?[0-9]*(\.[0-9]+)?" name="txtwaterLower">
          <label class="mdl-textfield__label" for="txtwaterLower">Water Level Lower (%): </label>
          <span class="mdl-textfield__error">Number required!</span>
        </div>
      </td>

      <td>
        <div class="mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
          <input class="mdl-textfield__input" type="text" pattern="-?[0-9]*(\.[0-9]+)?" name="txtwaterUpper">
          <label class="mdl-textfield__label" for="txtwaterUpper"> Water Level Upper (%): </label>
          <span class="mdl-textfield__error">Number required!</span>
        </div>
      </td>

    </tr>
    <tr>
      <td>
        <div class="mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
          <input class="mdl-textfield__input" type="text" name="txtTankHeight">
          <label class="mdl-textfield__label" for="txtTankHeight"> Water Tank Height (CM) : </label>
        </div>
      </td>
      <td>
        <div class="mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
          <input class="mdl-textfield__input" type="text" name="txtTinxyKey">
          <label class="mdl-textfield__label" for="txtTinxyKey"> Tinxy Device Key : </label>
        </div>
      </td>
    </tr>
    <tr>
      <td>
        <div class="mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
          <input class="mdl-textfield__input" type="text" name="txtTinxyAPIKey">
          <label class="mdl-textfield__label" for="txtTinxyAPIKey"> Tinxy API Key : </label>
        </div>
      </td>
    </tr>
    <tr>
      <td colspan="2" align="right">
        <button id="btnSave"
          class="mdl-button mdl-js-button mdl-button--raised mdl-js-ripple-effect mdl-button--accent">
          Save
        </button>
      </td>
    </tr>
  </table>
</body>
</html>
)=====";

int waterLevelLowerThreshold = 20;
int waterLevelUpperThreshold = 80;
int tankHeight = 100;
int MotorStatus = 0;
String tinxyKey = "62568039a0011179488b852a";
String tinxyAPIKey = "Bearer 498cfc0432856ed534c6c8cde9e8af3c64f721e7";
float volume = 0;
float liters = 0;
WiFiClient client;
String inputString = "";  // a string to hold incoming data
String dataToSend = "";
int waterLevelDownCount = 0, waterLevelUpCount = 0;
ESP8266WebServer server(80);

void motorOn() {
  //If we used the Tinxy Relay Module
  //if (tinxyKey != NULL && tinxyAPIKey != NULL) {
  Serial.println("MotorON event");
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String serverPath = "https://backend.tinxy.in/v2/devices/" + tinxyKey + "/toggle";
  // Your Domain name with URL path or IP address with path
  client.connect("backend.tinxy.in", 443);
  http.begin(client, serverPath);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", tinxyAPIKey);
  int httpResponseCode = http.POST("{\"request\":{\"state\":1},\"deviceNumber\":1}");
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  if (httpResponseCode == 200)
    MotorStatus = 1;
  // Free resources
  http.end();
  //}
}

void motorOff() {
  //If we used the Tinxy Relay Module
  //if (tinxyKey != NULL && tinxyAPIKey != NULL) {
  Serial.println("MotorOFF event");
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String serverPath = "https://backend.tinxy.in/v2/devices/" + tinxyKey + "/toggle";
  // Your Domain name with URL path or IP address with path
  client.connect("backend.tinxy.in", 443);
  http.begin(client, serverPath);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", tinxyAPIKey);
  int httpResponseCode = http.POST("{\"request\":{\"state\":0},\"deviceNumber\":1}");
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  if (httpResponseCode == 200)
    MotorStatus = 0;
  // Free resources
  http.end();
  //}
}
// This function sends Arduino's uptime every second to Virtual Pin 2.
void myTimerEvent() {
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V1, liters);
  //Blynk.virtualWrite(V2, millis() / 1000);
}

int writeStringToEEPROM(int addrOffset, const String &strToWrite)
{
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
  return addrOffset + 1 + len;
}
int readStringFromEEPROM(int addrOffset, String *strToRead)
{
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0'; // !!! NOTE !!! Remove the space between the slash "/" and "0" (I've added a space because otherwise there is a display bug)
  *strToRead = String(data);
  return addrOffset + 1 + newStrLen;
}

void saveEEPROM() {
  Serial.println("EEPROM Saving values");
  EEPROM.write(0, 1);
  EEPROM.write(5, tankHeight);
  EEPROM.write(10, waterLevelLowerThreshold);
  EEPROM.write(15, waterLevelUpperThreshold);
  EEPROM.write(20, MotorStatus);
  int addr1 = writeStringToEEPROM(50, tinxyKey);
  int addr2 = writeStringToEEPROM(addr1, tinxyAPIKey);
  EEPROM.commit();
}

void readEEPROM() {
  Serial.println("EEPROM Get values");
  tankHeight = EEPROM.read(5);
  waterLevelLowerThreshold = EEPROM.read(10);
  waterLevelUpperThreshold = EEPROM.read(15);
  MotorStatus = EEPROM.read(20);
  int addr1 = readStringFromEEPROM(50, &tinxyKey);
  int addr2 = readStringFromEEPROM(addr1, &tinxyAPIKey);
}

int checkValueinEEPROM(){
  int check = 0;
  Serial.println("EEPROM check values");
  check = EEPROM.read(0);
  Serial.print("Check Value - ");
  Serial.println(check);
  return  check;
}


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
  if (MotorStatus == 0)  //MOTOR ON
    server.send(200, "text/plain", "off");
  else server.send(200, "text/plain", "on");
}

void handleToggle() {
  server.enableCORS(true);
  MotorStatus = (server.arg(0)).toInt();
  if (MotorStatus == 1) {
    motorOn();
  } else {
    motorOff();
  }
  server.send(200, "text/plain", "OK");
}

void handleGetData() {
  server.enableCORS(true);
  DynamicJsonDocument doc(512);
  doc["tankheight"] = tankHeight;
  doc["tankLower"] = waterLevelLowerThreshold;
  doc["tankUpper"] = waterLevelUpperThreshold;
  doc["motorSatus"] = MotorStatus;
  doc["tinxyKey"] = tinxyKey;
  doc["tinxyAPIKey"] = tinxyAPIKey;
  String buf;
  serializeJson(doc, buf);
  server.send(200, "application/json", buf);
}

void handleRangeSetting() {
  Serial.println("Save Data API");
  waterLevelLowerThreshold = (server.arg(0)).toInt();
  waterLevelUpperThreshold = (server.arg(1)).toInt();
  tankHeight = (server.arg(2)).toInt();
  tinxyKey = (server.arg(3));
  tinxyAPIKey = (server.arg(4));
  server.enableCORS(true);
  saveEEPROM();
  server.send(200, "text/plain", "OK");
}


void measure_Volume() {
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

  float waterHeightCM = distanceCm;
  //Serial.print("Water Height in Tank CM : ");
  //Serial.println(waterHeightCM);
  volume = (tankHeight - waterHeightCM) / tankHeight;
  liters = volume * 100;  // for percentage
  //Serial.println(liters);

  if (liters <= waterLevelLowerThreshold)
    waterLevelDownCount++;
  else waterLevelDownCount = 0;
  if (liters >= waterLevelUpperThreshold)
    waterLevelUpCount++;
  else waterLevelUpCount = 0;
  if (waterLevelDownCount == 3) {  //TURN ON RELAY
    Serial.println("motor turned on");
    //digitalWrite(MOTOR_CONTROL_PIN, LOW);  //If we use Relay then active LOW (ON)
    // Motor On Tinxy API HTTP Call
    motorOn();
  }
  if (waterLevelUpCount == 3) {  //TURN OFF RELAY
    Serial.println("motor turned off");
    //digitalWrite(MOTOR_CONTROL_PIN, HIGH);  //If we use Relay then active HIGH (OFF)
    // Motor Off Tinxy API HTTP Call
    motorOff();
  }
}



void runPeriodicFunc() {
  static const unsigned long REFRESH_INTERVAL1 = 1000;  // 2.1sec
  static unsigned long lastRefreshTime1 = 0;
  if (millis() - lastRefreshTime1 >= REFRESH_INTERVAL1) {
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
  //clear the value in EEPROM for already save SSID and Password
  //wifiManager.resetSettings();
  wifiManager.autoConnect("AutoWaterPump Wifi");
  Serial.println("Wifi Connected");

  //WebServer Setup
  Serial.print("Local IP Address:");
  Serial.println(WiFi.localIP());
  server.on("/", handleRoot);
  server.on("/level", handleLevelRequest);
  server.on("/configRange", handleRangeSetting);
  server.on("/motorstatus", handleStatus);
  server.on("/getdata", handleGetData);
  server.on("/toggle", handleToggle);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  Serial.print("WIFI Settings : SSID - ");
  Serial.print(WiFi.SSID().c_str());
  Serial.print(" PWD - ");
  Serial.println(WiFi.psk().c_str());
  //Blynk Setup
  Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID().c_str(), WiFi.psk().c_str());
  Serial.println("Blynk started");
  // Setup a Blynk function to be called every second
  timer.setInterval(1000L, myTimerEvent);

  //EEPROM Setup
  EEPROM.begin(512);
  int vcheck = checkValueinEEPROM();
  if(vcheck != 1)
    saveEEPROM();
  else
    readEEPROM();
}


void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED, LOW);  // LED ON    
    while (WiFi.status() == WL_CONNECTED) {
      runPeriodicFunc();
      server.handleClient();
      //LED Function
      digitalWrite(LED, HIGH);
      delay(500);
      digitalWrite(LED, LOW);
      delay(200);
      //Blynk Sync
      Blynk.run();
      timer.run();
    }
  } else {
    digitalWrite(LED, HIGH);  // LED OFF
  }
}