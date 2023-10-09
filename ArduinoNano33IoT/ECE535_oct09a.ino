#include "arduino_secrets.h"
#include "thingProperties.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void parseUbloxMessage(const char* message) {
  char time[10] = "";
  char latitude[10] = "";
  char lat_dir;  // To store N/S
  char longitude[11] = "";
  char long_dir;  // To store E/W
  char altitude[10] = "";

  sscanf(message, "%6s-%9s%c-%10s%c-%4s", time, latitude, &lat_dir, longitude, &long_dir, altitude);

  // Convert the latitude and longitude to float
  float latitudeFloat = atof(latitude);
  float longitudeFloat = atof(longitude);
  
  // Convert from DDDMM.MMMM to DD.DDDDDD
  float newLatitudeFloat = trunc(latitudeFloat / 100) + (latitudeFloat - trunc(latitudeFloat / 100) * 100) / 60;
  float newLongitudeFloat = trunc(longitudeFloat / 100) + (longitudeFloat - trunc(longitudeFloat / 100) * 100) / 60;
  
  // Add negative sign for S and W
  if (lat_dir == 'S') {
    newLatitudeFloat = -newLatitudeFloat;
  }
  
  if (long_dir == 'W') {
    newLongitudeFloat = -newLongitudeFloat;
  }
  
  // Convert float back to string with 7 decimal places for latitude and 8 for longitude
  char newLatitude[15], newLongitude[15];
  snprintf(newLatitude, 15, "%.7f", newLatitudeFloat);
  snprintf(newLongitude, 15, "%.7f", newLongitudeFloat);

  ubloxLat = String(newLatitude);  // Already a String
  ubloxLong = String(newLongitude);  // Already a String
  ubloxTime = String(time);
  ubloxAlt = String(altitude);
  coordMAP = {newLatitudeFloat, newLongitudeFloat};

  // Debug output
  Serial.println("Parsed message:");
  Serial.print("Time: "); Serial.println(ubloxTime);
  Serial.print("Latitude: "); Serial.println(ubloxLat);
  Serial.print("Longitude: "); Serial.println(ubloxLong);
  Serial.print("Altitude: "); Serial.println(ubloxAlt);
}


void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  delay(1500);
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  while (ArduinoCloud.connected() != 1) {
    ArduinoCloud.update();
    delay(500);
  }

  timeClient.begin();
  timeClient.update();
}

void loop() {
  ArduinoCloud.update();
  timeClient.update();
  unsigned long currentTime = timeClient.getEpochTime();
  
  int modulo;
  
  static String receivedMessageSerial1 = "";

  while (Serial1.available()) {
    char incomingByte = Serial1.read();
    receivedMessageSerial1 += incomingByte;

    if (incomingByte == '\n' || incomingByte == '\r') {
      receivedMessageSerial1.trim();
      
      if (receivedMessageSerial1.length() > 0) {
        char buf[50];
        receivedMessageSerial1.toCharArray(buf, 50);
        parseUbloxMessage(buf);
        rAW = receivedMessageSerial1;
      }
      
      receivedMessageSerial1 = "";
    }
  }
  Serial.println(currentTime);
  modulo = currentTime % 15;
  Serial1.println(String(modulo));
  timeUTC = currentTime;
  delay(1000);
}