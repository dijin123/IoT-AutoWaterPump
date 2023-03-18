#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

const int trigPin = 12;
const int echoPin = 14;


//define sound velocity in cm/uS
#define SOUND_VELOCITY 0.034
#define CM_TO_INCH 0.393701
#define LED D0 //Define LED pin D0

long duration;
float distanceCm;
float distanceInch;

void setup() {
  Serial.begin(9600); // Starts the serial communication
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  pinMode(LED, OUTPUT); //Onbard LED
  WiFiManager wifiManager;
    wifiManager.resetSettings();
    wifiManager.autoConnect("DNA WiFi Manager");
    Serial.println("connected :)");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED)
        {
          digitalWrite(LED,LOW); // LED ON
          while(WiFi.status() == WL_CONNECTED){
            digitalWrite(LED,HIGH);
            delay(500);
            digitalWrite(LED,LOW);
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
            distanceCm = duration * SOUND_VELOCITY/2;
            
            // Convert to inches
            distanceInch = distanceCm * CM_TO_INCH;
            
            // Prints the distance on the Serial Monitor
            Serial.print("Distance (cm): ");
            Serial.println(distanceCm);
            Serial.print("Distance (inch): ");
            Serial.println(distanceInch);
            
            delay(1000); 
          }
        }
        else {
          digitalWrite(LED,HIGH); // LED OFF
        }
  
}