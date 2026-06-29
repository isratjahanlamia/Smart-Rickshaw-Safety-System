#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <RTClib.h>

// 1. PIN CONFIGURATION
LiquidCrystal lcd(A0, A1, A2, A3, 5, 2); 
SoftwareSerial gsmSerial(10, 11); 
SoftwareSerial gpsSerial(4, 3);   

TinyGPSPlus gps;
RTC_DS3231 rtc;

const int buttonPin = 9;   
const int buzzerPin = 13;  
const int redLedPin = 12;  
const int trigPin = 6;     
const int echoPin = 7;     

// 2. VARIABLES & SETTINGS
float simSpeed = 0; 
int violationCount = 0;
const int SPEED_LIMIT = 25;
const int PARKING_THRESHOLD = 10; 
String inputString = ""; 

void setup() {
  Serial.begin(9600);
  gsmSerial.begin(9600);
  gpsSerial.begin(9600);
  
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  lcd.begin(16, 2);
  lcd.print(F("SYSTEM STARTING"));
  
  if (!rtc.begin()) { 
    Serial.println(F("RTC Not Found!")); 
  }

  // --- CRITICAL FIX: SYNC TIME ---
  // This line tells the RTC to take the time from your laptop RIGHT NOW.
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); 

  delay(2000);
  lcd.clear();

  Serial.println(F("--- Rickshaw Simulation Active ---"));
  Serial.println(F("Commands: Numbers (Speed), Type 'P' to check parking status."));
}

void loop() {
  while (gpsSerial.available() > 0) { gps.encode(gpsSerial.read()); }

  if (Serial.available() > 0) {
    inputString = Serial.readStringUntil('\n');
    inputString.trim();

    if (inputString == "P" || inputString == "p") {
      checkSonarParking();
    }
    else if (inputString.length() > 0) {
      simSpeed = inputString.toFloat();
      Serial.print(F("SPEED INPUT: ")); Serial.print(simSpeed); Serial.println(F(" km/h"));
      
      if (simSpeed > SPEED_LIMIT) {
        handleOverspeed();
      } else {
        digitalWrite(redLedPin, LOW);
        noTone(buzzerPin);
      }
    }
  }

  // FEATURE 4: EMERGENCY SOS
  if (digitalRead(buttonPin) == LOW) {
    DateTime now = rtc.now();
    // Padding zeros for a clean look (e.g., 09:05 instead of 9:5)
    String timeStr = (now.hour() < 10 ? "0" : "") + String(now.hour()) + ":" + 
                     (now.minute() < 10 ? "0" : "") + String(now.minute());
    
    Serial.print(F("\n*** EMERGENCY BUTTON PRESSED AT ")); Serial.print(timeStr); Serial.println(F(" ***"));
    
    digitalWrite(redLedPin, HIGH);
    tone(buzzerPin, 1500);
    
    sendPoliceSMS(F("EMERGENCY! Passenger in danger. Save them!"), timeStr);
    
    noTone(buzzerPin);
    digitalWrite(redLedPin, LOW);
    while(digitalRead(buttonPin) == LOW) { delay(10); } 
  }

  lcd.setCursor(0, 0);
  lcd.print(F("SPD: ")); lcd.print(simSpeed, 1);
  lcd.print(F(" km/h    "));
  lcd.setCursor(0, 1);
  lcd.print(F("Violations: ")); lcd.print(violationCount);
}

void checkSonarParking() {
  Serial.println(F("Scanning Sonar..."));
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;

  Serial.print(F("Distance: ")); Serial.print(distance); Serial.println(F(" cm"));

  if (distance > 0 && distance < PARKING_THRESHOLD) {
    DateTime now = rtc.now();
    String timeStr = (now.hour() < 10 ? "0" : "") + String(now.hour()) + ":" + 
                     (now.minute() < 10 ? "0" : "") + String(now.minute());
    
    Serial.println(F("ILLEGAL PARKING DETECTED!"));
    digitalWrite(redLedPin, HIGH);
    tone(buzzerPin, 1500);
    
    sendPoliceSMS(F("ALERT: Rickshaw parked illegally."), timeStr);
    
    noTone(buzzerPin);
    digitalWrite(redLedPin, LOW);
  } else {
    Serial.println(F("PARKING IS LEGAL."));
  }
}

void handleOverspeed() {
  violationCount++;
  Serial.print(F("Violation Count: ")); Serial.println(violationCount);

  if (violationCount < 3) {
    digitalWrite(redLedPin, HIGH);
    tone(buzzerPin, 1000); 
    delay(1000); 
    noTone(buzzerPin);
    digitalWrite(redLedPin, LOW);
  } 
  else {
    DateTime now = rtc.now();
    String timeStr = (now.hour() < 10 ? "0" : "") + String(now.hour()) + ":" + 
                     (now.minute() < 10 ? "0" : "") + String(now.minute());
    
    Serial.print(F("3rd Violation reached at ")); Serial.println(timeStr);
    digitalWrite(redLedPin, HIGH);
    tone(buzzerPin, 1000); 
    delay(3000); 
    noTone(buzzerPin);
    digitalWrite(redLedPin, LOW);
    
    sendPoliceSMS(F("ALERT: Speed limit violated 3 times."), timeStr);
    violationCount = 0; 
  }
  simSpeed = 0;
}

void sendPoliceSMS(const __FlashStringHelper* text, String timeVal) {
  lcd.clear();
  lcd.print(F("SMS SENDING..."));

  Serial.println(F("----------------------------------------"));
  Serial.println(F("SMS TO POLICE: 01300374489"));
  Serial.print(F("MSG: ")); Serial.print(text); Serial.print(F(" | Time: ")); Serial.println(timeVal);
  Serial.print(F("GPS: http://googleusercontent.com/maps.google.com/"));
  Serial.print(gps.location.lat(), 6);
  Serial.print(F(","));
  Serial.println(gps.location.lng(), 6);
  Serial.println(F("----------------------------------------"));

  gsmSerial.println(F("AT+CMGF=1"));
  delay(200);
  gsmSerial.print(F("AT+CMGS=\"+8801300374489\"\r"));
  delay(200);
  gsmSerial.print(text);
  gsmSerial.print(F(" Time: "));
  gsmSerial.print(timeVal);
  gsmSerial.write(26);
  
  delay(2000);
  lcd.clear();
}