#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

TinyGPSPlus gps;

HardwareSerial GPS(1)
;
HardwareSerial SIM800(2);

#define GPS_RX 4
#define GPS_TX 5

#define SIM_RX 16
#define SIM_TX 17

#define VIBRATION_PIN 27

const char OWNER_NUMBER[] = "YOUR_PHONE_NUMBER";

bool alertSent = false;
unsigned long lastAlert = 0;
const unsigned long ALERT_COOLDOWN = 30000;

void setup() {
  Serial.begin(115200);

  GPS.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  SIM800.begin(9600, SERIAL_8N1, SIM_RX, SIM_TX);

  pinMode(VIBRATION_PIN, INPUT);

  delay(3000);

  Serial.println("================================");
  Serial.println("BIKE ANTI-THEFT SYSTEM");
  Serial.println("================================");

  sendAT("AT", "OK", 2000);
  sendAT("ATE0", "OK", 2000);
  sendAT("AT+CMGF=1", "OK", 2000);
  sendAT("AT+CNMI=1,2,0,0,0", "OK", 2000);

  Serial.println("System Ready");
}

void loop() {

  while (GPS.available()) {
    gps.encode(GPS.read());
  }

  readSIM800();

  int vibration = digitalRead(VIBRATION_PIN);

  Serial.print("Vibration: ");
  Serial.println(vibration);

  if (gps.location.isValid()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);

    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println("Waiting for GPS fix...");
  }

  if (vibration == HIGH) {

    if (!alertSent || millis() - lastAlert > ALERT_COOLDOWN) {

      Serial.println("!!! VIBRATION DETECTED !!!");

      sendLocationSMS();

      alertSent = true;
      lastAlert = millis();
    }
  }

  if (vibration == LOW) {
    alertSent = false;
  }

  Serial.println("----------------------------");

  delay(500);
}

bool sendAT(const char* command, const char* expected, unsigned long timeout) {

  SIM800.println(command);

  unsigned long start = millis();
  String response = "";

  while (millis() - start < timeout) {

    while (SIM800.available()) {
      char c = SIM800.read();
      response += c;
      Serial.write(c);
    }

    if (response.indexOf(expected) != -1) {
      return true;
    }
  }

  Serial.print("Command failed: ");
  Serial.println(command);

  return false;
}

void sendLocationSMS() {

  Serial.println("Sending location SMS...");

  SIM800.println("AT+CMGF=1");
  delay(500);

  SIM800.print("AT+CMGS=\"");
  SIM800.print(OWNER_NUMBER);
  SIM800.println("\"");

  delay(1000);

  String response = "";

  unsigned long start = millis();

  while (millis() - start < 3000) {

    while (SIM800.available()) {
      char c = SIM800.read();
      response += c;
      Serial.write(c);
    }

    if (response.indexOf(">") != -1) {
      break;
    }
  }

  if (response.indexOf(">") == -1) {
    Serial.println("SMS prompt not received.");
    return;
  }

  SIM800.println("ALERT! Bike vibration detected.");

  if (gps.location.isValid()) {

    SIM800.print("Latitude: ");
    SIM800.println(gps.location.lat(), 6);

    SIM800.print("Longitude: ");
    SIM800.println(gps.location.lng(), 6);

    SIM800.print("Google Maps: https://maps.google.com/?q=");
    SIM800.print(gps.location.lat(), 6);
    SIM800.print(",");
    SIM800.println(gps.location.lng(), 6);

  } else {

    SIM800.println("GPS location not available.");

  }

  SIM800.write(26);

  delay(5000);

  Serial.println("SMS command completed.");
}

void readSIM800() {

  if (!SIM800.available()) {
    return;
  }

  String message = "";

  while (SIM800.available()) {

    message += (char)SIM800.read();

    delay(2);
  }

  Serial.print("SIM800: ");
  Serial.println(message);

  message.toUpperCase();

  if (message.indexOf("LOCATION") >= 0) {

    Serial.println("LOCATION request received.");

    sendLocationSMS();
  }
}