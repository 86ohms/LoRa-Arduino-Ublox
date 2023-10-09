#include <Arduino.h>
#include <string.h>


bool shouldSendMessage = false;


// Modify the function to return the parsed message
void parseUbloxMessage(const char* message, char* parsedMessage) {
  char* token;
  char buffer[128];
  strncpy(buffer, message, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0'; // Ensure null-termination
  
  token = strtok(buffer, ",");
  
  int field_count = 1;
  char time[10] = "";
  char latitude[10] = "";
  char longitude[11] = "";
  char altitude[10] = "";
  char lat_dir[2] = "";
  char lon_dir[2] = "";
  
  while (token != NULL) {
    if (field_count == 3) {
      strncpy(time, token, 6);
      time[6] = '\0';
    }
    if (field_count == 4) {
      strncpy(latitude, token, 9);
      latitude[9] = '\0';
    }
    if (field_count == 5) {
      strncpy(lat_dir, token, 1);
      lat_dir[1] = '\0';
    }
    if (field_count == 6) {
      strncpy(longitude, token, 10);
      longitude[10] = '\0';
    }
    if (field_count == 7) {
      strncpy(lon_dir, token, 1);
      lon_dir[1] = '\0';
    }
    if (field_count == 8) {
      strncpy(altitude, token, 4);
      altitude[4] = '\0';
    }

    token = strtok(NULL, ",");
    field_count++;
  }
  
  // Concatenate the parsed fields into one string
  snprintf(parsedMessage, 50, "%s-%s%s-%s%s-%s", time, latitude, lat_dir, longitude, lon_dir, altitude);
}
void readSerial3() {
  String receivedData = "";
  while (Serial3.available()) {
    char incomingByte = Serial3.read();
    receivedData += incomingByte;
    if (incomingByte == '\n' || incomingByte == '\r') {
      receivedData.trim();
      Serial.println("Received from Serial3: " + receivedData);
      break; // Exit the loop once we receive a line break
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);       
  Serial2.begin(9600);    
  Serial3.begin(9600);

  //// LORA-E5 SETUP ////
  Serial.println("Starting LORA Setup...");
  delay(1000);
  Serial3.println("AT+DR=US915");  
  delay(3000);
  readSerial3();
  Serial3.println("AT+DR=3"); //SF7 @ 125kHz
  delay(3000);
  readSerial3();
  Serial3.println("AT+CH=NUM,8-15");
  delay(3000);
  readSerial3();
  Serial3.println("AT+MODE=LWOTAA");
  delay(3000);
  readSerial3();
  Serial3.println("AT+UART=TIMEOUT,1000");
  delay(3000);
  readSerial3();
  Serial3.println("AT+JOIN");
  delay(500);
  readSerial3();

  
  delay(5000);
}

void loop() {
  static String receivedMessageUBlox = "";
  static String receivedMessageSerial1 = "";  
  static String receivedMessageSerial3 = "";
  static String receivedMessageSerial2 = "";
  static unsigned long lastUbloxTransmitTime = 0;
  unsigned long currentMillis = millis();
  String lastParsedMessage = "";  // A new local variable to hold the last parsed message

  // Reading from Serial1
  while (Serial1.available()) {
    char incomingByte = Serial1.read();
    receivedMessageSerial1 += incomingByte;
    if (incomingByte == '\n' || incomingByte == '\r') {
      receivedMessageSerial1.trim();
      Serial.println(receivedMessageSerial1);

      if (receivedMessageSerial1.length() > 0 && atoi(receivedMessageSerial1.c_str()) == 0) {
        shouldSendMessage = true;
      }

      receivedMessageSerial1 = "";
    }
  }

  // Reading from Serial3
  while (Serial3.available()) {
    char incomingByte = Serial3.read();
    receivedMessageSerial3 += incomingByte;
    if (incomingByte == '\n' || incomingByte == '\r') {
      receivedMessageSerial3.trim();
      Serial.println(receivedMessageSerial3);
      receivedMessageSerial3 = "";
    }
  }

  // Reading from Serial2
  while (Serial2.available()) {
    char incomingByte = Serial2.read();
    receivedMessageSerial2 += incomingByte;
    if (incomingByte == '\n' || incomingByte == '\r') {
      receivedMessageSerial2.trim();
      char parsedMessageSerial2[50];
      parseUbloxMessage(receivedMessageSerial2.c_str(), parsedMessageSerial2);
      
      // Check if parsed message is not just dashes
      if (strcmp(parsedMessageSerial2, "---") != 0) {
        Serial.println(parsedMessageSerial2);
        Serial1.println(parsedMessageSerial2);  
        
        lastParsedMessage = parsedMessageSerial2; // Store the parsed message
      }
      
      receivedMessageSerial2 = "";
    }
  }

  // Checking for shouldSendMessage flag and last parsed message
  if (shouldSendMessage && !lastParsedMessage.equals("")) {
    Serial3.println("AT+MSG=" + lastParsedMessage);
    // Serial1.println("AT+MSG=" + lastParsedMessage);
    Serial.println("Sent message: " + lastParsedMessage);  // Debug message
    
    lastParsedMessage = "";  // Reset for next read
    shouldSendMessage = false;  // Reset flag
  }
}
