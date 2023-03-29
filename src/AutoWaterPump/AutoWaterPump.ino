#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <Espalexa.h>
#include <UrlEncode.h>
#include <string>


const int trigPin = 12;
const int echoPin = 14;
long duration;
float distanceCm;
float distanceInch;

//call back function
void firstLightChanged(uint8_t brightness);
// device names
String Device_1_Name = "Water Pump";



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
  <title>Auto Water Pump</title>
  <link rel="icon" type="image/x-icon" href="https://img.icons8.com/external-flat-wichaiwi/256/external-iot-internet-of-things-flat-wichaiwi-11.png">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.3/jquery.min.js"></script>
  <script src="https://storage.googleapis.com/code.getmdl.io/1.0.6/material.min.js"></script>
  <link rel="stylesheet" href="https://storage.googleapis.com/code.getmdl.io/1.0.6/material.indigo-pink.min.css">
  <link rel="stylesheet" href="https://fonts.googleapis.com/icon?family=Material+Icons">
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
          $('input[name="txtOverflow"]')[0].parentElement.MaterialTextfield.change(data.overFlow);
          $('input[name="txtTime"]')[0].parentElement.MaterialTextfield.change(data.timervalue);
          $('input[name="txtMobileNumber"]')[0].parentElement.MaterialTextfield.change(data.mobNum);
          $('input[name="txtWhatsAPIKey"]')[0].parentElement.MaterialTextfield.change(data.whatsAPIkey);
          if (data.motorSatus == 0) {
            $('#switch-1')[0].parentElement.MaterialSwitch.off();
            $('#switch-1').next().text("Motor Off");
          }
          else {
            $('#switch-1')[0].parentElement.MaterialSwitch.on();
            $('#switch-1').next().text("Motor On");
          }
          console.log(data.timerSatus);
          if (data.timerSatus == 0) {
            $('#chkTimer')[0].parentElement.MaterialCheckbox.uncheck();
            $('#chkTimer').next().text("Timer Off");
          }
          else {
            $('#chkTimer')[0].parentElement.MaterialCheckbox.check();
            $('#chkTimer').next().text("Timer On");
          }
        }
      });
      //Handle the Motor toggle button change
      $('#switch-1').change(function () {
        var status;
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
            url: "/toggle?",
            data: "motorsatus=" + status,
            success: function (result) {
              $("#result").html(result);
            }
          });
      });
      // Timer Checkbox event enable
      $('#chkTimer').change(function () {
        if ($(this).is(':checked')) {
          $(this).next().text("Timmer On");
        }
        else {
          $(this).next().text("Timmer Off");
        }
      });
      // Gauge Display every 3 mints 
      setInterval(function () {
        $.ajax({
          type: "GET",
          url: "/level",
          success: function (data) {
            console.log(data.level);
            console.log(data.motorstatus);
            var value = (data.level / 100);
            if (value < 0 || value > 1) {
              return;
            }
            gaugeElement.querySelector(".gauge__fill").style.transform = `rotate(${value / 2}turn)`;
            gaugeElement.querySelector(".gauge__cover").textContent = `${Math.round(value * 100)}%`;
            if (data.motorstatus == 1) {
              $('#switch-1')[0].parentElement.MaterialSwitch.on();
              $('#switch-1').next().text("Motor On");
            }
            else {
              $('#switch-1')[0].parentElement.MaterialSwitch.off();
              $('#switch-1').next().text("Motor Off");
            }
          }
        });
      }, 3000);//time in 3 milliseconds

      // Save the in data to IoT 
      $("#btnSave").click(function () {
        var waterLower = $('input[name="txtwaterLower"]').val();
        var waterUpper = $('input[name="txtwaterUpper"]').val();
        var tinxyKey = $('input[name="txtTinxyKey"]').val();
        var TankHeight = $('input[name="txtTankHeight"]').val();
        var tinxyAPIKey = $('input[name="txtTinxyAPIKey"]').val();
        var waterOverflow = $('input[name="txtOverflow"]').val();
        var timerValue = $('input[name="txtTime"]').val(); 
        var mobNum = $('input[name="txtMobileNumber"]').val(); 
        var whatsAPIkey = $('input[name="txtWhatsAPIKey"]').val(); 
        var chkvalue = $('#chkTimer').is(':checked');
        var timerSatus = chkvalue ? 1 : 0;
        $.ajax(
          {
            type: "GET",
            url: "/configRange?",
            data: "wlow=" + waterLower + "&whi=" + waterUpper + "&hght=" + TankHeight + "&tKey=" + tinxyKey + "&tAPI=" + tinxyAPIKey + "&Ovflow=" + waterOverflow + "&time=" + timerValue + "&tSat=" + timerSatus + "&mob="+ mobNum + "&whtAPI=" + whatsAPIkey,
            success: function (result) {
              $("#result").html(result);
              alert("Data Saved Successfuly !!!");
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

    .mdl-textfield--floating-label.is-focused .mdl-textfield__label,
    .mdl-textfield--floating-label.is-dirty .mdl-textfield__label {
      font-size: 14px !important;
      top: 1px !important;
      font-weight: bold;
    }

    .mdl-switch__label {
      font-size: 14px !important;
      color: rgb(63, 81, 181) !important;
      font-weight: bold !important;
    }

    .mdl-checkbox__label {
      font-size: 14px !important;
      color: rgb(63, 81, 181) !important;
      font-weight: bold !important;
    }
  </style>
</head>

<body>
  <br>
  <table style="margin-left:auto; margin-right:auto; padding: 20px;">
    <tr>
      <td colspan="2">
        <label class="mdl-switch mdl-js-switch mdl-js-ripple-effect" for="switch-1">
          <input type="checkbox" name="switch-1" id="switch-1" class="mdl-switch__input">
          <span class="mdl-switch__label">Motor Off</span>
        </label>
      </td>
    </tr>
    <tr>
      <td colspan="2" align="center" style="padding-bottom:10px">
        <label class="mdl-layout-title" style="font-weight: bolder;">Water Level </label>
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
      <td colspan="2"><span class="mdl-layout-title"
          style="padding-top: 5px; padding-bottom: 5px; font-weight: bolder;">Settings :</span></td>
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
          <span class="mdl-textfield__error">Number required!</span>
        </div>
      </td>
      <td>
        <div class="mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
          <input class="mdl-textfield__input" type="text" name="txtOverflow">
          <label class="mdl-textfield__label" for="txtOverflow"> Overflow Level (CM) : </label>
          <span class="mdl-textfield__error">Number required!</span>
        </div>
      </td>
    </tr>
    <tr>
      <td>
        <div class="mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
          <input class="mdl-textfield__input" type="text" name="txtTinxyKey">
          <label class="mdl-textfield__label" for="txtTinxyKey"> Tinxy Device Key : </label>
        </div>
      </td>
      <td>
        <div class="mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
          <input class="mdl-textfield__input" type="text" name="txtTinxyAPIKey">
          <label class="mdl-textfield__label" for="txtTinxyAPIKey"> Tinxy API Key : </label>
        </div>
      </td>
    </tr>
    <tr>
      <td>
        <div class="mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
          <input class="mdl-textfield__input" type="text" name="txtMobileNumber">
          <label class="mdl-textfield__label" for="txtMobileNumber"> WhatsApp Number : </label>
        </div>
      </td>
      <td>
        <div class="mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
          <input class="mdl-textfield__input" type="text" name="txtWhatsAPIKey">
          <label class="mdl-textfield__label" for="txtWhatsAPIKey"> WhatsApp API Key : </label>
        </div>
      </td>
    </tr>
    <tr>
      <td>
        <div>
          <label class="mdl-checkbox mdl-js-checkbox mdl-js-ripple-effect" for="chkTimer">
            <input type="checkbox" name="chkTimer" id="chkTimer" class="mdl-checkbox__input">
            <span class="mdl-checkbox__label">Timmer Off</span>
        </div>
        <div class="mdl-textfield mdl-js-textfield mdl-textfield--floating-label" style="padding: 0px !important;">
          <input class="mdl-textfield__input" type="time" name="txtTime" style="width: 100px;">
        </div>
        </label>
      </td>
      <td align="right">
        <button id="btnSave"
        class="mdl-button mdl-js-button mdl-button--raised mdl-js-ripple-effect mdl-button--accent">
        Save Data
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
String tinxyAPIKey = "498cfc0432856ed534c6c8cde9e8af3c64f721e7";
int overFlow = 10;
float volume = 0;
float liters = 0;
WiFiClient client;
String inputString = "";  // a string to hold incoming data
String dataToSend = "";
int waterLevelDownCount = 0, waterLevelUpCount = 0;
ESP8266WebServer server(80);
DateTime currentTime;
RTC_DS1307 DS1307_RTC;
char Week_days[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
int timerSatus;
String timeValue;
Espalexa espalexa;
String phoneNumber = "0";
String whatsAppApiKey = "0";
String curDateTime;
int timerRetry = 0;

void setCrossOrigin() {
  server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
  server.sendHeader(F("Access-Control-Max-Age"), F("600"));
  server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
  server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));
};

void sendMessage(String message) {
  if (phoneNumber.length() >= 10) {
    if (whatsAppApiKey.length() >= 7) {
      // Data to send with HTTP POST
      String url = "http://api.callmebot.com/whatsapp.php?phone=" + String(phoneNumber) + "&apikey=" + String(whatsAppApiKey) + "&text=" + urlEncode(message);
      //Serial.println(url);
      WiFiClient client;
      HTTPClient http;
      http.begin(client, url);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Send HTTP POST request
      int httpResponseCode = http.POST(url);
      if (httpResponseCode == 200) {
        Serial.print("WhatsApp Message sent successfully");
      } else {
        Serial.println("Error sending the WhatsApp message");
        Serial.print("HTTP response code: ");
        Serial.println(httpResponseCode);
      }

      // Free resources
      http.end();
    }
  }
}

void motorOn() {
  //If we used the Tinxy Relay Module
  //if (tinxyKey != NULL && tinxyAPIKey != NULL) {
  Serial.println("MotorON event");
  if (MotorStatus == 0) {
    if (liters < waterLevelUpperThreshold) {
      WiFiClientSecure client;
      client.setInsecure();
      HTTPClient http;
      String serverPath = "https://backend.tinxy.in/v2/devices/" + tinxyKey + "/toggle";
      // Your Domain name with URL path or IP address with path
      //client.connect("backend.tinxy.in", 443);
      http.begin(client, serverPath);
      String token = "Bearer " + tinxyAPIKey;
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Authorization", token);
      int httpResponseCode = http.POST("{\"request\":{\"state\":1},\"deviceNumber\":1}");
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      if (httpResponseCode == 200) {
        MotorStatus = 1;
      }
      // Free resources
      http.end();
    }
  }
  //}
}

void motorOff() {
  //If we used the Tinxy Relay Module
  //if (tinxyKey != NULL && tinxyAPIKey != NULL) {
  if (MotorStatus == 1) {
    Serial.println("MotorOFF event");
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    String serverPath = "https://backend.tinxy.in/v2/devices/" + tinxyKey + "/toggle";
    // Your Domain name with URL path or IP address with path
    //client.connect("backend.tinxy.in", 443);
    http.begin(client, serverPath);
    String token = "Bearer " + tinxyAPIKey;
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", token);
    int httpResponseCode = http.POST("{\"request\":{\"state\":0},\"deviceNumber\":1}");
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode == 200) {
      MotorStatus = 0;
    }
    // Free resources
    http.end();
  }
  //}
}
// This function sends Arduino's uptime every second to Virtual Pin 2.
void myTimerEvent() {
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V1, liters);
  //Blynk Swich Status
  Blynk.virtualWrite(V0, MotorStatus);
  //Blynk.virtualWrite(V2, millis() / 1000);
}

int writeStringToEEPROM(int addrOffset, const String &strToWrite) {
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++) {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
  return addrOffset + 1 + len;
}
int readStringFromEEPROM(int addrOffset, String *strToRead) {
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++) {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0';  // !!! NOTE !!! Remove the space between the slash "/" and "0" (I've added a space because otherwise there is a display bug)
  *strToRead = String(data);
  return addrOffset + 1 + newStrLen;
}

void saveEEPROM() {
  EEPROM.write(0, 1);
  EEPROM.write(5, tankHeight);
  EEPROM.write(10, waterLevelLowerThreshold);
  EEPROM.write(15, waterLevelUpperThreshold);
  EEPROM.write(20, overFlow);
  EEPROM.write(25, timerSatus);
  int addr1 = writeStringToEEPROM(50, tinxyKey);
  int addr2 = writeStringToEEPROM(addr1, tinxyAPIKey);
  int addr3 = writeStringToEEPROM(addr2, timeValue);
  int addr4 = writeStringToEEPROM(addr3, phoneNumber);
  int addr5 = writeStringToEEPROM(addr4, whatsAppApiKey);
  EEPROM.commit();
}

void readEEPROM() {
  Serial.println("EEPROM Get values");
  tankHeight = EEPROM.read(5);
  waterLevelLowerThreshold = EEPROM.read(10);
  waterLevelUpperThreshold = EEPROM.read(15);
  overFlow = EEPROM.read(20);
  timerSatus = EEPROM.read(25);
  int addr1 = readStringFromEEPROM(50, &tinxyKey);
  int addr2 = readStringFromEEPROM(addr1, &tinxyAPIKey);
  int addr3 = readStringFromEEPROM(addr2, &timeValue);
  int addr4 = readStringFromEEPROM(addr3, &phoneNumber);
  int addr5 = readStringFromEEPROM(addr4, &whatsAppApiKey);
}

int checkValueinEEPROM() {
  int check = 0;
  Serial.println("EEPROM check values");
  check = EEPROM.read(0);
  Serial.print("Check Value - ");
  Serial.println(check);
  return check;
}


void handleRoot() {
  setCrossOrigin();
  server.send_P(200, "text/html;charset=UTF-8", index_html);
}


void handleLevelRequest() {
  setCrossOrigin();
  DynamicJsonDocument doc(512);
  doc["level"] = String(liters);
  doc["motorstatus"] = MotorStatus;
  String buf;
  serializeJson(doc, buf);
  server.send(200, "application/json", buf);
}


void handleNotFound() {
  setCrossOrigin();
  String message = "File Not Found\n\n";
  server.send(404, "text/plain", message);
}

void handleToggle() {
  setCrossOrigin();
  int mStatus = (server.arg(0)).toInt();
  if (mStatus == 1) {
    if (liters < waterLevelUpperThreshold) {
      if (MotorStatus == 0) {
        motorOn();
        sendMessage("Motor Started by API on " + curDateTime);
      }
    }
  } else {
    if (MotorStatus == 1) {
      motorOff();
      sendMessage("Motor Stopped by API on " + curDateTime);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleGetData() {
  setCrossOrigin();
  DynamicJsonDocument doc(512);
  doc["tankheight"] = tankHeight;
  doc["tankLower"] = waterLevelLowerThreshold;
  doc["tankUpper"] = waterLevelUpperThreshold;
  doc["motorSatus"] = MotorStatus;
  doc["tinxyKey"] = tinxyKey;
  doc["tinxyAPIKey"] = tinxyAPIKey;
  doc["overFlow"] = overFlow;
  doc["timervalue"] = timeValue;
  doc["timerSatus"] = timerSatus;
  doc["mobNum"] = phoneNumber;
  doc["whatsAPIkey"] = whatsAppApiKey;
  String buf;
  serializeJson(doc, buf);
  server.send(200, "application/json", buf);
}

void handleRangeSetting() {
  setCrossOrigin();
  Serial.println("Save Data API");
  waterLevelLowerThreshold = (server.arg(0)).toInt();
  waterLevelUpperThreshold = (server.arg(1)).toInt();
  tankHeight = (server.arg(2)).toInt();
  tinxyKey = (server.arg(3));
  tinxyAPIKey = (server.arg(4));
  overFlow = (server.arg(5)).toInt();
  timeValue = (server.arg(6));
  timerSatus = (server.arg(7)).toInt();
  phoneNumber = (server.arg(8));
  whatsAppApiKey = (server.arg(9));
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
  float sensorValueCM = duration * SOUND_VELOCITY / 2;
  Serial.println("*****************************");
  Serial.print("Sensor Value (CM): ");
  Serial.println(sensorValueCM);
  Serial.print("distanceCm (CM): ");
  Serial.println(distanceCm);
  if (distanceCm == 0.00) {
    distanceCm = sensorValueCM;
  }
  float differance = sensorValueCM - distanceCm;
  Serial.print("differance (CM): ");
  Serial.println(differance);
  if (differance < 5.00) {
    if (differance > -5.00) {
      float currentWaterLevel = sensorValueCM - overFlow;
      if (currentWaterLevel > 0.0) {
        distanceCm = sensorValueCM;
        float maxWaterLevel = tankHeight - overFlow;
        /*Serial.print("currentWaterLevel (CM): ");
        Serial.println(currentWaterLevel);
        Serial.print("maxWaterLevel (CM): ");
        Serial.println(maxWaterLevel);*/
        // Convert to inches
        //distanceInch = distanceCm * CM_TO_INCH;
        float waterheight = maxWaterLevel - currentWaterLevel;
        //Serial.print("waterheight (CM): ");
        //Serial.println(waterheight);
        volume = (waterheight / maxWaterLevel);
        if (volume > 0.0) {
          //Serial.print("volume (CM): ");
          //Serial.println(volume);
          liters = volume * 100;  // for percentage
          Serial.print("Litters : ");
          Serial.println(liters);

          if (liters <= waterLevelLowerThreshold)
            waterLevelDownCount++;
          else waterLevelDownCount = 0;
          if (liters >= waterLevelUpperThreshold)
            waterLevelUpCount++;
          else waterLevelUpCount = 0;
          if (waterLevelDownCount == 3) {  //TURN ON RELAY
            // MotorOn Tinxy API HTTP Call
            if (MotorStatus == 0) {
              motorOn();
              Serial.println("Motor turned on");
              sendMessage("Low Water level - Motor Started on " + curDateTime);
            }
          }
          if (waterLevelUpCount == 3) {  //TURN OFF RELAY
            // MotorOff Tinxy API HTTP Call
            if (MotorStatus == 1) {
              motorOff();
              Serial.println("Motor turned off");
              sendMessage("High Water Level - Motor Stopped on " + curDateTime);
            }
          }
          if (liters > 99.0) {
            if (MotorStatus == 1) {
              motorOff();
              Serial.println("Motor turned off");
              sendMessage("Motor Forcefully Stopped on " + curDateTime);
            }
          }
        }
      }
    }
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

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void Timer() {
  currentTime = DS1307_RTC.now();
  String H = getValue(timeValue, ':', 0);
  String M = getValue(timeValue, ':', 1);
  int timerhour = H.toInt();
  int timermint = M.toInt();
  int curhour = currentTime.hour();
  int curmint = currentTime.minute();
  int cursec = currentTime.second();
  curDateTime = currentTime.timestamp();
  Serial.print("Timer Time - ");
  Serial.print(timerhour);
  Serial.print(':');
  Serial.println(timermint);
  Serial.print("Current Time - ");
  Serial.print(curhour);
  Serial.print(':');
  Serial.println(curmint);
  if (timerSatus == 1) {
    if (timerhour == curhour) {
      if (timermint == curmint) {
        if (cursec == 0) {
          if (MotorStatus == 0) {
            motorOn();
            Serial.println("Motor turned on");
            sendMessage("Motor Started by Timer on " + curDateTime);
          }
        }
      }
    }
  }
}

//our callback functions
void firstLightChanged(uint8_t brightness) {
  //Control the device
  EspalexaDevice *d1 = espalexa.getDevice(0);
  if (brightness == 255) {
    if (MotorStatus == 0) {
      motorOn();
      Serial.println("Motor Turned ON");
      d1->setPercent(100);
      sendMessage("Motor Started by Alexa on " + curDateTime);
    }
  } else {
    if (MotorStatus == 1) {
      motorOff();
      Serial.println("Motor Turned OFF");
      d1->setPercent(0);
      sendMessage("Motor Stopped by Alexa on " + curDateTime);
    }
  }
}

void addDevices() {
  // Define your devices here.
  espalexa.addDevice(Device_1_Name, firstLightChanged);  //simplest definition, default state off
  espalexa.begin(&server);
  Serial.println("Alaxa Begin");
}

BLYNK_WRITE(V0) {
  // Set incoming value from pin V0 to a variable
  int pinValue = param.asInt();
  Serial.print("Blynk Value : ");
  Serial.println(pinValue);
  if (pinValue == 1) {
    if (MotorStatus == 0) {
      Serial.println("Blynk Motor On");
      motorOn();
    }
  } else {
    if (MotorStatus == 1) {
      Serial.println("Blynk Motor Off");
      motorOff();
    }
  }
}

void setup() {
  Serial.begin(115200);      // Starts the serial communication
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
  //add divice to Alexa
  addDevices();
  //WebServer Setup
  Serial.print("Local IP Address:");
  Serial.println(WiFi.localIP());
  server.on("/", HTTP_GET, handleRoot);
  server.on("/level", HTTP_GET, handleLevelRequest);
  server.on("/configRange", HTTP_GET, handleRangeSetting);
  server.on("/getdata", HTTP_GET, handleGetData);
  server.on("/toggle", HTTP_GET, handleToggle);
  server.onNotFound([]() {
    if (!espalexa.handleAlexaApiCall(server.uri(), server.arg(0))) {
      server.send(404, "text/plain", "Not found");
    }
  });
  // WebService Enabling. Disable due to ESPALEXA is running
  //server.onNotFound(handleNotFound);
  //server.begin();
  Serial.println("HTTP server started");
  Serial.print("WIFI Settings : SSID - ");
  Serial.print(WiFi.SSID().c_str());
  Serial.print(" PWD - ");
  Serial.println(WiFi.psk().c_str());
  //Blynk Setup
  Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID().c_str(), WiFi.psk().c_str());
  Serial.println("Blynk started");
  // Setup a Blynk function to be called every second
  timer.setInterval(3000L, myTimerEvent);

  //EEPROM Setup
  EEPROM.begin(512);
  int vcheck = checkValueinEEPROM();
  if (vcheck != 1)
    saveEEPROM();
  else
    readEEPROM();

  if (!DS1307_RTC.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }

  // Comment this code. Run this 1st time only for set the time to RTC timmer
  //DS1307_RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
}


void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED, LOW);  // LED ON
    while (WiFi.status() == WL_CONNECTED) {
      runPeriodicFunc();
      //WebServer haddle request. Disable due to ESPALEXA is running
      //server.handleClient();
      //LED Function
      digitalWrite(LED, HIGH);
      delay(500);
      digitalWrite(LED, LOW);
      delay(200);
      //Blynk Sync
      Blynk.run();
      timer.run();
      // Timmer Module Tigger Event
      Timer();
      //loop the Alexa
      espalexa.loop();
      delay(1);
    }
  } else {
    digitalWrite(LED, HIGH);  // LED OFF
  }
}