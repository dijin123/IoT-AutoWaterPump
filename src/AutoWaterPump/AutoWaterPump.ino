#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#include <HCSR04.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>


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
const char index_html[] PROGMEM = R"=====( 
<!DOCTYPE html>
<html>
<head>
<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.3/jquery.min.js"></script>
<link rel="stylesheet" href="https://fonts.googleapis.com/icon?family=Material+Icons">
<link rel="stylesheet" href="https://code.getmdl.io/1.3.0/material.indigo-pink.min.css">
<script defer src="https://code.getmdl.io/1.3.0/material.min.js"></script>
<script>
$( document ).ready(function() {
        const gaugeElement = document.querySelector(".gauge");
        console.log( "document loaded" );
        $.ajax({ 
                      type: "GET",
                      url: "/getdata",
                      success: function(data){ 
                        $('input[name="txtwaterLower"]')[0].parentElement.MaterialTextfield.change(data.tankLower);
                        $('input[name="txtwaterUpper"]')[0].parentElement.MaterialTextfield.change(data.tankUpper);
                        $('input[name="txtTinxyKey"]')[0].parentElement.MaterialTextfield.change(data.tinxyKey);
                        $('input[name="txtTankHeight"]')[0].parentElement.MaterialTextfield.change(data.tankheight);
                        if(data.motorSatus == 0) {
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

            $('.mdl-switch input[type="checkbox"]').change(function(){
                var status = 0;
                if($(this).is(':checked')){
                    $(this).next().text("Motor On");
                    status = 1;
                }
                else
                {
                    $(this).next().text("Motor Off");
                    status = 0;
                }
                $.ajax(
                {
                  type: "GET",
                  url: '/toggle?',
                  data: "motorsatus=" + status,
                  success: function(result)
                  {
                      $("#result").html(result);
                  }
                });
            });
          // Gauge Display 
        setInterval(function()
                {
                  $.ajax({ 
                      type: "GET",
                      crossDomain: true,
                      url: "/level",
                      success: function(data){  
                          console.log(data);
                          var value = (data/100);
                          if (value < 0 || value > 1) {
                            return;
                          }
                          gaugeElement.querySelector(".gauge__fill").style.transform = `rotate(${
                            value / 2
                          }turn)`;
                          gaugeElement.querySelector(".gauge__cover").textContent = `${Math.round(
                            value * 100
                          )}%`;
                }
            });
        }, 1000);//time in milliseconds

        $("#btnSave").click(function()
        {
            var waterLower = $('input[name="txtwaterLower"]').val();
            var waterUpper = $('input[name="txtwaterUpper"]').val();
            var tinxyKey = $('input[name="txtTinxyKey"]').val();
            var TankHeight = $('input[name="txtTankHeight"]').val();
            console.log("post value: lower: " + waterLower +" upper : " + waterUpper +" TankHeight :" +TankHeight);  
            $.ajax(
            {
                type: "GET",
                url: '/configRange?',
                data: "lower=" + waterLower + "&upper=" + waterUpper + "&Height=" + TankHeight + "&key="+tinxyKey,
                success: function(result)
                {
                    $("#result").html(result);
                }
            });
            alert("Data Saved Successfuly!!!")
        });
    });
</script>
<style>
.gauge {width: 100%;max-width: 250px;font-family: "Roboto", sans-serif;font-size: 32px;color: #004033;}
.gauge__body {width: 100%;height: 0;padding-bottom: 50%;background: #b4c0be;position: relative;border-top-left-radius: 100% 200%;border-top-right-radius: 100% 200%;overflow: hidden;}
.gauge__fill {position: absolute;top: 100%;left: 0;width: inherit;height: 100%;background: #009578;transform-origin: center top;transform: rotate(0.25turn);transition: transform 0.2s ease-out;}
.gauge__cover {width: 75%;height: 150%;background: #ffffff;border-radius: 50%;position: absolute;top: 25%;left: 50%;transform: translateX(-50%);display: flex;align-items: center;justify-content: center;padding-bottom: 25%;box-sizing: border-box;}
</style>
</head>
<body>

<br>
<table>
  <tr>
    <td colspan="2" >
      <label class="mdl-switch mdl-js-switch mdl-js-ripple-effect" for="switch-1">
        <input type="checkbox" id="switch-1" class="mdl-switch__input">
        <span class="mdl-switch__label">Motor Off</span>
      </label>
    </td>
  </tr>
  <tr>
    <td colspan="2" align="center">
      <label class = "mdl-layout-title" >Water Level </label>
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
           <div class = "mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
              <input class = "mdl-textfield__input" type = "text" pattern = "-?[0-9]*(\.[0-9]+)?" name= "txtwaterLower">
              <label class = "mdl-textfield__label" for = "txtwaterLower">Water Level Lower : </label>
              <span class = "mdl-textfield__error">Number required!</span>
           </div>
     </td>
     
     <td>
           <div class = "mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
              <input class = "mdl-textfield__input" type = "text" pattern = "-?[0-9]*(\.[0-9]+)?" name= "txtwaterUpper">
              <label class = "mdl-textfield__label" for = "txtwaterUpper"> Water Level Upper : </label>
              <span class = "mdl-textfield__error">Number required!</span>
           </div>
     </td>
    
  </tr>
  <tr>
    <td>
      <div class = "mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
        <input class = "mdl-textfield__input" type = "text" name= "txtTankHeight">
        <label class = "mdl-textfield__label" for = "txtTankHeight"> Water Tank Height : </label>
     </div>
    </td>
    <td>
      <div class = "mdl-textfield mdl-js-textfield mdl-textfield--floating-label">
        <input class = "mdl-textfield__input" type = "text" name= "txtTinxyKey">
        <label class = "mdl-textfield__label" for = "txtTinxyKey"> Tinxy API Key : </label>
     </div>
    </td>
  </tr>
  <tr>
    <td colspan="2" align="right">
      <button id="btnSave" class="mdl-button mdl-js-button mdl-button--raised mdl-js-ripple-effect mdl-button--accent">
      Save
    </button></td>
  </tr>
</table>   
</body>
</html>
)=====";

#define MAX_HEIGHT  27  // tank height 27 cm manually enter here for automatic empty tank
#define MOTOR_CONTROL_PIN D4

//UltraSonicDistanceSensor distanceSensor(D6,D5);  //D1 trig, D2=echo

int waterLevelLowerThreshold = 15;
int waterLevelUpperThreshold = 80;
int tankHeight = 100;
int MotorStatus = 0;
String tinxyKey = "62568039a0011179488b852a";
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
  if (MotorStatus == 0)  //MOTOR ON
  server.send(200, "text/plain", "on");
  else server.send(200, "text/plain", "off");
}

void handleToggle() {
  server.enableCORS(true);
  MotorStatus = (server.arg(0)).toInt();
  server.send(200, "text/plain", "OK");
}

void handleGetData(){
  server.enableCORS(true);
  DynamicJsonDocument doc(512);
  doc["tankheight"] = tankHeight;
  doc["tankLower"] = waterLevelLowerThreshold;
  doc["tankUpper"] = waterLevelUpperThreshold;
  doc["motorSatus"] = MotorStatus;
  doc["tinxyKey"] = tinxyKey;
  String buf;
  serializeJson(doc, buf);
  server.send(200, "application/json", buf);
}

void handleRangeSetting() {
  waterLevelLowerThreshold = (server.arg(0)).toInt();
  waterLevelUpperThreshold = (server.arg(1)).toInt();
  tankHeight = (server.arg(2)).toInt();
  //BLYNK_AUTH_TOKEN = (server.arg(3));
  tinxyKey = (server.arg(3));
  Serial.print(waterLevelLowerThreshold);
  Serial.print(":");
  Serial.print(waterLevelUpperThreshold);
  Serial.print(":");
  Serial.print(MAX_HEIGHT);
  Serial.print(":");
  Serial.print(BLYNK_AUTH_TOKEN);
  Serial.print(":");
  Serial.println(tinxyKey);
  server.enableCORS(true);
  server.send(200, "text/plain", "OK");
}


void measure_Volume()
{
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
      Serial.print("Distance (cm): ");
      Serial.println(distanceCm);
      //Serial.print("Distance (inch): ");
      //Serial.println(distanceInch);
  //float heightInch = 1 * distanceSensor.measureDistanceCm();
  float waterHeightCM = distanceCm;
  Serial.print("Water Height in CM : ");
  Serial.println(waterHeightCM);
  volume = (tankHeight - waterHeightCM) / tankHeight;  //MAX_HEIGHT-distance will give actual height, 1 cm for   // offset adjustment
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
  wifiManager.autoConnect("AutoWaterPump WiFi");
  Serial.println("Connected :)");

  //WebServer Setup
  Serial.print("IP address:");
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

      
      Blynk.run();
      timer.run();
    }
  } else {
    digitalWrite(LED, HIGH);  // LED OFF
  }
}