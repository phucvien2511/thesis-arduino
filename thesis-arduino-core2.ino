// Import libraries

#define M5_CORE2

#ifdef M5_CORE2
  #include "M5Core2.h"
#else
  #include "M5Atom.h"
#endif
#include "global.h"

/////////////////////////////////////////////////////////////////////////////////////
// Setup WiFi
/////////////////////////////////////////////////////////////////////////////////////

// Start WiFi connection
void connect_wifi() {
    Serial.print("Connecting to Wi-Fi...");
    #ifdef M5_CORE2
      WiFi.begin(LAB_WIFI_SSID, LAB_WIFI_PASSWORD);
    #else 
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    #endif

    int counter = 0;
    while (WiFi.status() != WL_CONNECTED) {
        if (counter >= 60) { // 60*500ms = 30s
            Serial.println("\nWi-Fi connection timed out.");
            return;
        }
        delay(500);
        Serial.print(".");
        counter++;
    }
    Serial.println("\nConnected to Wi-Fi!");
}



/////////////////////////////////////////////////////////////////////////////////////
// Setup HTTP Client
/////////////////////////////////////////////////////////////////////////////////////

const char* serverName = "http://172.28.182.60:8000";
int port = 8000;
const char* baseApiPath = "/api";

// Init ArduinoJson instance
StaticJsonDocument<128> jsonDoc;
DynamicJsonDocument dJsonDoc(1024);
String jsonString;
String HTTP_Get(const char* path) {
    WiFiClient client;
    HTTPClient http;

    // Your Domain name with URL path or IP address with path
    http.begin(path);

    // If you need Node-RED/server authentication, insert user and password below
    // http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    String payload = "{}";

    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        payload = http.getString();
    } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();

    return payload;
}

void HTTP_Post(const char* path, const String& requestBody) {
    WiFiClient client;
    HTTPClient http;

    // Your Domain name with URL path or IP address with path
    http.begin(path);

    // Specify content type and length
    http.addHeader("Content-Type", "application/json");

    // If you need Node-RED/server authentication, insert user and password below
    // http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

    // Send HTTP POST request
    int httpResponseCode = http.POST(requestBody);

//    String payload = "{}";
//
//    if (httpResponseCode > 0) {
//        Serial.print("HTTP Response code: ");
//        Serial.println(httpResponseCode);
//        payload = http.getString();
//    } else {
//        Serial.print("Error code: ");
//        Serial.println(httpResponseCode);
//    }
    // Free resources
    http.end();
 
}
//void TestGET() {
//    jsonString = httpGETRequest(serverName);
//    Serial.println(jsonString);
//}

/////////////////////////////////////////////////////////////////////////////////////
// Setup Serial Monitor (SM) for debugging & sending command
/////////////////////////////////////////////////////////////////////////////////////

// Define variables
char sm_received[34];       // Store received characters (length + 2 for prefix & suffix)
char SM_PREFIX = '!';       // Prefix of command
char SM_SUFFIX = '#';       // Suffix of command
boolean isNewLine = false;  // Tracking the end of command

// Functions
void SM_Init(int baudRate) {
    Serial.begin(baudRate);
}

void SM_Reading() {
    boolean isReading = false;
    int index = 0;
    char currentChar;
    // Clear the previous buffer
    memset(sm_received, 0, sizeof(sm_received));
    while (Serial.available() > 0 && !isNewLine) {
        currentChar = Serial.read();
        if (isReading) {
            if (currentChar != SM_SUFFIX) {
                sm_received[index] = currentChar;
                index++;
                if (index >= 33) {
                    index = 32;
                }
            } else {
                sm_received[index] = '\0';
                isReading = false;
                isNewLine = true;
                index = 0;
            }
        } else if (currentChar == SM_PREFIX) {
            isReading = true;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// Setup Fingerprint Unit
/////////////////////////////////////////////////////////////////////////////////////

bool Finger_Compare() {
    Serial.println("Comparing fingerprint...");
    fingerResponse = Finger.fpm_compareFinger();
    return (fingerResponse == ACK_SUCCESS);
}

bool Finger_Register(int userId, int authLevel) {
    Serial.println("Registering fingerprint for new user...");
    fingerResponse = Finger.fpm_addUser(userId, authLevel);
    return (fingerResponse == ACK_SUCCESS);
}

bool Finger_DeleteAll() {
    Serial.println("Deleting all fingerprint data...");
    fingerResponse = Finger.fpm_deleteAllUser();
    return (fingerResponse == ACK_SUCCESS);
}

/////////////////////////////////////////////////////////////////////////////////////
// Setup RFID Unit
/////////////////////////////////////////////////////////////////////////////////////

boolean isGetUID = false; // Status of reading process
String uidString;         // Store UID from a read card
// Functions
void RFID_GetVersion() {
    if (!RFID_UNIT_CONNECTED) {
      return;
    }
    byte v = RFID.PCD_ReadRegister(RFID.VersionReg);
    Serial.print(F("MFRC522 Software Version: 0x"));
    Serial.print(v, HEX);
    if (v == 0x91) {
        Serial.print(F(" = v1.0"));
    } else if (v == 0x92) {
        Serial.print(F(" = v2.0"));
    } else {
        Serial.print(F(" unknown"));
    }
    Serial.println("");
    // When 0x00 or 0xFF is returned, communication probably failed
    if ((v == 0x00) || (v == 0xFF)) {
        Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
    }
}

// Configure dynamic buffers
String RFID_WRITE_BUFFER_STRING = "";  // Get from MQTT message arrived - Using to write to card
int RFID_WRITE_BUFFER_STRING_LENGTH; // Length of the message
String RFID_READ_BUFFER_STRING = "";   // Using to display the result
MFRC522::MIFARE_Key MY_MIFARE_KEY = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; // The key to auth
byte RFID_READ_WRITE_STATUS; // Read/Write progress status
int RFID_BLOCK_NUM = 4;  //Start block
byte RFID_READ_BUFFER_SIZE = 18;  // Read buffer size
byte RFID_READ_BUFFER[18]; // Read buffer array

void RFID_RegisterUID(int status, String roomId) { // Send UID to back-end
  if (!RFID_UNIT_CONNECTED) {
    return;
  }
  String myJsonString = "";
  DynamicJsonDocument myDJsonDoc(1024);
  myDJsonDoc["type"] = "response";
  JsonArray jsonArr = myDJsonDoc.createNestedArray("data");
  JsonObject uidObject = jsonArr.createNestedObject();
  uidObject["to"] = "room_access_key";
  uidObject["roomId"] = roomId;
  uidObject["status"] = status;
  serializeJson(myDJsonDoc, myJsonString);
  //Serial.println(testStr.c_str());
  myMQTT.publish(PUBLISH_TOPIC, myJsonString.c_str());
}

void RFID_AuthAccess(int status, String roomId) {
   if (!RFID_UNIT_CONNECTED) {
    return;
  }
  String myJsonString = "";
  DynamicJsonDocument myDJsonDoc(1024);
  myDJsonDoc["type"] = "response";
  JsonArray jsonArr = myDJsonDoc.createNestedArray("data");
  JsonObject uidObject = jsonArr.createNestedObject();
  uidObject["to"] = "auth_access";
  uidObject["roomId"] = roomId;
  uidObject["status"] = status;
  serializeJson(myDJsonDoc, myJsonString);
  //Serial.println(testStr.c_str());
  myMQTT.publish(PUBLISH_TOPIC, myJsonString.c_str());  
}

void WriteDataToBlock(int RFID_BLOCK_NUM, byte RFID_WRITE_BUFFER[]) 
{
  /* Authenticating the desired data block for write access using Key A */
  RFID_READ_WRITE_STATUS = RFID.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, RFID_BLOCK_NUM, &MY_MIFARE_KEY, &(RFID.uid));
  if (RFID_READ_WRITE_STATUS != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Write: ");
    Serial.println(RFID.GetStatusCodeName(RFID_READ_WRITE_STATUS));
    RFID_RegisterUID(0, "1");
    return;
  }
  else {
    Serial.println("Authentication success");
  }
  RFID_BLOCK_NUM = 4;
  /* Write data to the block */
  //RFID_READ_WRITE_STATUS = RFID.MIFARE_Write(RFID_BLOCK_NUM, RFID_WRITE_BUFFER, 16);
  for (int i = 0; i < RFID_WRITE_BUFFER_STRING_LENGTH; i += 16) {
    if (RFID_BLOCK_NUM % 4 != 3) {
       byte temp[16];
       memcpy(temp, &RFID_WRITE_BUFFER[i], min(16, RFID_WRITE_BUFFER_STRING_LENGTH - i)); // Split data to different blocks (if the data is longer than 16 chars)
       RFID_READ_WRITE_STATUS = RFID.MIFARE_Write(RFID_BLOCK_NUM, temp, 16);
       if (RFID_READ_WRITE_STATUS != MFRC522::STATUS_OK) {
          Serial.print("Writing to Block failed: ");
          Serial.println(RFID.GetStatusCodeName(RFID_READ_WRITE_STATUS));
          RFID_RegisterUID(0, "1");
          return;
      }
      else {
        Serial.print("Written to block ");
        Serial.println(RFID_BLOCK_NUM);
        
      }
    }
    RFID_BLOCK_NUM++;
  }
  RFID_RegisterUID(1, "1");
  RFID_BLOCK_NUM = 4;
   RFID.PICC_HaltA();
   RFID.PCD_StopCrypto1();
  
}

void ReadDataFromBlock(int blockNum, int stringLength) 
{
  /* Authenticating the desired data block for Read access using Key A */
  byte status = RFID.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (byte)RFID_BLOCK_NUM, &MY_MIFARE_KEY, &(RFID.uid));

  if (status != MFRC522::STATUS_OK) {
     Serial.print("Authentication failed for Read: ");
     Serial.println(RFID.GetStatusCodeName(status));
     return;
  }
  else {
    Serial.println("Authentication success");
  }

  /* Reading data from the Block */
  byte readBuffer[18];
  byte readBufferLen = 18;
  for (int i = 0; i < stringLength; i += 16) {
    //status = RFID.MIFARE_Read(RFID_BLOCK_NUM, RFID_READ_BUFFER, &RFID_READ_BUFFER_SIZE);
    if (blockNum % 4 != 3) {
      status = RFID.MIFARE_Read(blockNum, readBuffer, &readBufferLen);
      if (status != MFRC522::STATUS_OK) {
        Serial.print("Reading failed: ");
        Serial.println(RFID.GetStatusCodeName(status));
        return;
      }
      Serial.print("Read from block #"); 
      Serial.println(blockNum);
      for (uint8_t b = 0; b < min(16, stringLength - i); b++) {
        RFID_READ_BUFFER_STRING += (char)readBuffer[b]; 
        Serial.print((char)readBuffer[b]);
      }
      Serial.println();
    }
    blockNum++;
  }
  RFID_READ_BUFFER_STRING.trim(); // Remove the null byte
  Serial.println("Compare 2 strings: ");
  Serial.print("Input: ");
  Serial.print(RFID_WRITE_BUFFER_STRING);
  Serial.print(" - Length: ");
  Serial.println(RFID_WRITE_BUFFER_STRING.length());
  Serial.print("Output: ");
  Serial.print(RFID_READ_BUFFER_STRING.substring(0, RFID_WRITE_BUFFER_STRING.length()));
  Serial.print(" - Length: ");
  Serial.println(RFID_READ_BUFFER_STRING.substring(0, RFID_WRITE_BUFFER_STRING.length()).length());
  if (RFID_WRITE_BUFFER_STRING.length() !=
      RFID_READ_BUFFER_STRING.substring(0, RFID_WRITE_BUFFER_STRING.length()).length()) {
    RFID_AuthAccess(0, "1");
    
  }
  else {
    if (RFID_READ_BUFFER_STRING.substring(0, RFID_WRITE_BUFFER_STRING.length()) == RFID_WRITE_BUFFER_STRING) {
      RFID_AuthAccess(1, "1");
    }
    else {
      RFID_AuthAccess(0, "1");
    } 
  }

    RFID.PICC_HaltA();
    RFID.PCD_StopCrypto1();

  
}


long rfid_delayTimer = 0;
void RFID_ReadCardUID() {
    if (!RFID_UNIT_CONNECTED) {
      return;
    }
    Serial.println("---READ RFID CARD ID-----");
    // Cannot read card's UID
    if (!RFID.PICC_IsNewCardPresent() || !RFID.PICC_ReadCardSerial()) {
        delay(200);
        rfid_delayTimer++;
        return;
    }
    // Now a card is selected. The UID and SAK is in RFID.uid.
    // Dump UID
    Serial.print("Card UID:");
    for (byte i = 0; i < RFID.uid.size; i++) {
        Serial.print(RFID.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(RFID.uid.uidByte[i], HEX);
        uidString += String(RFID.uid.uidByte[i], HEX);
    }
    Serial.println();
     Serial.print(F("PICC type: "));
     byte piccType = RFID.PICC_GetType(RFID.uid.sak);
     Serial.println(RFID.PICC_GetTypeName(piccType));
     // Identify if your card cannot R/W
//     if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI
//        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
//        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
//        Serial.println(F("This sample only works with MIFARE Classic cards."));
//        return;
//    }
    //RFID.PICC_DumpToSerial(&(RFID.uid));
    isGetUID = true;
}

void RFID_ClearData() {
  if (!RFID_UNIT_CONNECTED) {
    return;
  }
  
  byte RFID_CLEAR_BUFFER[16] = { 0 };
  for (int i = 0; i < 63; i++) {
    if (i % 4 != 3) {
      byte status = RFID.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (byte)i, &MY_MIFARE_KEY, &(RFID.uid));

      if (status != MFRC522::STATUS_OK)
      {
         Serial.print("Authentication failed for Clear Data: ");
         Serial.println(RFID.GetStatusCodeName(status));
         return;
      }
      else
      {
        Serial.println("Authentication success");
      }
      RFID.MIFARE_Write(i, RFID_CLEAR_BUFFER, 16);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////
// Setup AC Measure unit
/////////////////////////////////////////////////////////////////////////////////////

int PRICE_PER_KWH = 1806;
float ELECTRIC_BILL = 0;
float TOTAL_POWER_CONSUMED = 0;
int MEASURE_TIME = 0;
void AC_Init() {
    if (!ACM_UNIT_CONNECTED) {
      return;
    }
    while (!acm.begin(&Wire, UNIT_ACMEASURE_ADDR, 21, 22, 100000UL)) {
        Serial.println("ACMeasure module not found!");  // Start AC Measure
    }
    Serial.print("AC Measure Firmware version: ");
    Serial.println(int(acm.getFirmwareVersion()));
}

void AC_ReadVoltage() {
    if (!ACM_UNIT_CONNECTED) {
      return;
    }
    if (acm.getReady()) {
        Serial.print("Voltage: ");
        float voltageVal = float(acm.getVoltage()) / 100.0;
        Serial.println(voltageVal);
        #ifdef M5_CORE2
          M5.Lcd.fillRect(160, 180, 100, 20, BLACK);
          M5.Lcd.setCursor(160, 180, 4);
          M5.Lcd.printf("%.2f V", voltageVal);
        #endif
    }
}

void AC_ReadCurrent() {
    if (!ACM_UNIT_CONNECTED) {
      return;
    }
    if (acm.getReady()) {
        Serial.print("Current: ");
        Serial.println(float(acm.getCurrent()) / 100.0);
    }
}

void AC_ReadPower() {
    if (!ACM_UNIT_CONNECTED) {
      return;
    }
    if (acm.getReady()) {
        Serial.print("Power: ");
        float powerValue = float(acm.getPower()) / 100.0;
        Serial.println(powerValue);
        TOTAL_POWER_CONSUMED += powerValue;
        MEASURE_TIME++;

//        jsonString = "";
//        jsonDoc["topic"] = "power-usage";
//        jsonDoc["value"] = powerValue;
//        serializeJson(jsonDoc, jsonString);
//        myMQTT.publish(PUBLISH_TOPIC, jsonString.c_str());
    } 
//    else {
//        jsonString = "";
//        jsonDoc["topic"] = "power-usage";
//        jsonDoc["value"] = 0;
//        serializeJson(jsonDoc, jsonString);
//        myMQTT.publish(PUBLISH_TOPIC, jsonString.c_str());
//    }
}

void AC_ReadPowerFactor() {
    if (!ACM_UNIT_CONNECTED) {
      return;
    }
    if (acm.getReady()) {
        Serial.print("Power factor: ");
        Serial.println(float(acm.getPowerFactor()) / 100.0);
    }
}

void AC_ReadApparentPower() {  // Cong suat bieu kien
    if (acm.getReady()) {
        Serial.print("Apparent power: ");
        Serial.println(float(acm.getApparentPower()) / 100.0);
    }
}
float kwhInitValue = 0.0;
void AC_ReadKWH() {
    if (!ACM_UNIT_CONNECTED) {
      return;
    }
    if (acm.getReady()) {
        Serial.print("kWh: ");
        float kwhValue = float(acm.getKWH()) / 100.0;
        Serial.println(kwhValue);
        TOTAL_POWER_CONSUMED = kwhValue + kwhInitValue;
        #ifdef M5_CORE2
          M5.Lcd.fillRect(170, 120, 200, 20, BLACK);
          M5.Lcd.setCursor(170, 120, 4);
          M5.Lcd.printf("%.2f kWh", TOTAL_POWER_CONSUMED);
        #endif
//        #ifdef M5_CORE2
//          M5.Lcd.fillRect(160, 180, 100, 20, BLACK);
//          M5.Lcd.setCursor(160, 180, 4);
//          M5.Lcd.printf("%.2f KWh", kwhValue);
//        #endif
//        jsonString = "";
//        jsonDoc["topic"] = "power-usage";
//        jsonDoc["value"] = kwhValue;
//        serializeJson(jsonDoc, jsonString);
//        myMQTT.publish(PUBLISH_TOPIC, jsonString.c_str());
        String myJsonString = "";
        DynamicJsonDocument myDJsonDoc(1024);
        myDJsonDoc["type"] = "publish";
        JsonArray jsonArr = myDJsonDoc.createNestedArray("data");
        JsonObject tempObject = jsonArr.createNestedObject();
        tempObject["topic"] = "power-usage";
        tempObject["value"] = round(TOTAL_POWER_CONSUMED * 100.0) / 100.0; 
        serializeJson(myDJsonDoc, myJsonString);
        myMQTT.publish(PUBLISH_TOPIC, myJsonString.c_str());
    }
    else {
        Serial.println("ACM is not ready yet!");
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// PaHUB functions
/////////////////////////////////////////////////////////////////////////////////////

void PaHub_Init() {
  if (!PAHUB_UNIT_CONNECTED) {
    return;
  }
    Serial.print("TCA9548_LIB_VERSION: ");
    Serial.println(TCA9548_LIB_VERSION);
    bool isChannelConnected[6] = {false};
    if (!pahub.begin()) {
        Serial.println("Could not connect to TCA9548 multiplexer.");
        return;
    }

    Serial.println("Scan the channels of the PaHub");
    for (int chan = 0; chan < PAHUB_MAX_CHANNELS; chan++) {
        pahub.selectChannel(chan);
        bool b = pahub.isConnected(RFID_ADDR);      // Check RFID connected
        if (!b) {
            b = pahub.isConnected(ACMEASURE_ADDR);  // Check ACM connected
        }
        if (!b) {
            b = pahub.isConnected(ENV4_ADDR);       // Check ENVIV connected
        }
        if (!b) {
            b = pahub.isConnected(RELAY_ADDR);      // Check 4-RELAY connected
        }
        if (b) {
            isChannelConnected[chan] = true;
        }
        Serial.print("CH");
        Serial.print(chan);
        Serial.print(": ");
        Serial.print(b ? "found!\t" : "x\t");
    }
    Serial.println();

    Serial.println("Scanning done!");
    for (int i = 0; i < PAHUB_MAX_CHANNELS; i++) {
        if (isChannelConnected[i]) {
            pahub.enableChannel(i);
            Serial.print("Enabled channel ");
            Serial.println(i);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////
// Setup ENVIV Unit
/////////////////////////////////////////////////////////////////////////////////////

void ENV_Init() {
  if (!ENV4_UNIT_CONNECTED) {
    return;
  }
  if (!sht4.begin(&Wire, ENV4_ADDR, I2C_SDA, I2C_SCL, 400000U)) {
        Serial.println("Couldn't find SHT4x");
        return;
  }
  else {
    Serial.println("SHT4X found!");
  }
  sht4.setPrecision(SHT4X_LOW_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

}

void ENV_ReadValue() {
  if (!ENV4_UNIT_CONNECTED) {
    return;
  }
  if (sht4.update()) {
    #ifdef M5_CORE2
      M5.Lcd.fillRect(170, 50, 150, 20, BLACK);
      M5.Lcd.setCursor(170, 50, 4);
      M5.Lcd.printf("%.2f oC", round(sht4.cTemp * 100.0) / 100.0);
      M5.Lcd.fillRect(170, 85, 150, 20, BLACK);
      M5.Lcd.setCursor(170, 85, 4);
      M5.Lcd.printf("%.2f %%", round(sht4.humidity * 100.0) / 100.0);
    #endif
    String myJsonString = "";
    DynamicJsonDocument myDJsonDoc(1024);
    myDJsonDoc["type"] = "publish";
    JsonArray jsonArr = myDJsonDoc.createNestedArray("data");
    JsonObject tempObject = jsonArr.createNestedObject();
    tempObject["topic"] = "temperature";
    tempObject["value"] = round(sht4.cTemp * 100.0) / 100.0; 
    JsonObject humidityObject = jsonArr.createNestedObject();
    humidityObject["topic"] = "humidity";
    humidityObject["value"] = round(sht4.humidity* 100.0) / 100.0;
    serializeJson(myDJsonDoc, myJsonString);
    myMQTT.publish(PUBLISH_TOPIC, myJsonString.c_str());
  }
}
/////////////////////////////////////////////////////////////////////////////////////
// Setup 4Relay Unit
/////////////////////////////////////////////////////////////////////////////////////

void Relay_Init() {
  if (!RELAY_UNIT_CONNECTED) {
    return;
  }
  if (relay.begin(&Wire, I2C_SDA, I2C_SCL)) {
    Serial.println("4-Relay initialized!");
    relay.Init(1); // Init relay in async mode
    return;
  }
  Serial.println("4-Relay started failed!");

}

bool relayFlag = false;
void Relay_Control(int index, int status) {
#ifdef M5_CORE2
  if (!RELAY_UNIT_CONNECTED) {
    return;
  }
  relay.relayWrite(index, status);
  if (status == 0) {
    if (index == 0) {
       M5.Lcd.fillCircle(175, 165, 10, RED);
    }
    else if (index == 1) {
      M5.Lcd.fillCircle(200, 165, 10, RED);
    }
    else if (index == 2) {
      M5.Lcd.fillCircle(225, 165, 10, RED);
    }
    else if (index == 3) {
      M5.Lcd.fillCircle(250, 165, 10, RED);
    }
  }
  else {
    if (index == 0) {
       M5.Lcd.fillCircle(175, 165, 10, BLUE);
    }
    else if (index == 1) {
      M5.Lcd.fillCircle(200, 165, 10, BLUE);
    }
    else if (index == 2) {
      M5.Lcd.fillCircle(225, 165, 10, BLUE);
    }
    else if (index == 3) {
      M5.Lcd.fillCircle(250, 165, 10, BLUE);
    } 
  }
#endif
}
/////////////////////////////////////////////////////////////////////////////////////
// Main functions
/////////////////////////////////////////////////////////////////////////////////////

void processSubscribeMessage() {
    deserializeJson(dJsonDoc, subscribeMessage);
    String msgType = dJsonDoc["type"].as<String>();
    JsonArray msgData = dJsonDoc["data"].as<JsonArray>();
    
    if (msgType == "command") {
      for (JsonVariant v : msgData) {
        JsonObject msgObj = v.as<JsonObject>();
        String msgDeviceName = msgObj["deviceName"].as<String>();
        if (msgDeviceName == "RFID") {
          String msgAction = msgObj["action"].as<String>();
          if (msgAction == "REGISTER_RFID_CARD") {
            if (RFID_UNIT_CONNECTED) {
              while (!isGetUID) {
                RFID_ReadCardUID();
              }
              String msgRoomId = msgObj["roomId"].as<String>();
              //RFID_RegisterUID(uidString, msgRoomId);
              isGetUID = false;
              uidString = "";
            }
          }
          else if (msgAction == "WRITE_RFID_CARD") {
            if (RFID_UNIT_CONNECTED) {

              while (!isGetUID) {
                if (rfid_delayTimer >= 50) {
                  isGetUID = true;
                  break;
                }
                RFID_ReadCardUID();
                
              }
              isGetUID = false;
              rfid_delayTimer = 0;
              uidString = "";
              //RFID_ClearData();
              RFID_READ_BUFFER_STRING = "";
              RFID_WRITE_BUFFER_STRING = msgObj["value"].as<String>();
              RFID_WRITE_BUFFER_STRING_LENGTH = (int)RFID_WRITE_BUFFER_STRING.length() + 1;
              byte RFID_WRITE_BUFFER[RFID_WRITE_BUFFER_STRING_LENGTH];
              Serial.print("Write buffer length: ");
              Serial.println(RFID_WRITE_BUFFER_STRING_LENGTH);
              RFID_WRITE_BUFFER_STRING.getBytes(RFID_WRITE_BUFFER, RFID_WRITE_BUFFER_STRING_LENGTH);
              WriteDataToBlock(RFID_BLOCK_NUM, RFID_WRITE_BUFFER);
            }
          }
          else if (msgAction == "READ_RFID_CARD") {
            if (RFID_UNIT_CONNECTED) {

              while (!isGetUID) {
                if (rfid_delayTimer >= 50) {
                  isGetUID = true;
                  break;
                }
                RFID_ReadCardUID();
                
              }
              isGetUID = false;
              rfid_delayTimer = 0;
              uidString = "";
              //RFID_ClearData();
              RFID_READ_BUFFER_STRING = "";
              RFID_WRITE_BUFFER_STRING = msgObj["value"].as<String>();
              RFID_WRITE_BUFFER_STRING_LENGTH = (int)RFID_WRITE_BUFFER_STRING.length() + 1;
              byte RFID_WRITE_BUFFER[RFID_WRITE_BUFFER_STRING_LENGTH];
              Serial.print("Read buffer length: ");
              Serial.println(RFID_WRITE_BUFFER_STRING_LENGTH);
              //RFID_WRITE_BUFFER_STRING.getBytes(RFID_WRITE_BUFFER, RFID_WRITE_BUFFER_STRING_LENGTH);
           
              //ReadDataFromBlock(RFID_BLOCK_NUM, RFID_READ_BUFFER);
              ReadDataFromBlock(RFID_BLOCK_NUM, RFID_WRITE_BUFFER_STRING_LENGTH);
              Serial.print("Read result: ");
              Serial.println(RFID_READ_BUFFER_STRING);

            }
          }
        }  
        else if (msgDeviceName == "RELAY") {
          String msgAction_str = msgObj["action"].as<String>();
          String msgIndex_str = msgObj["index"].as<String>();
          int msgAction = msgAction_str == "OFF" ? 0 : 1;
          int msgIndex = msgIndex_str.toInt();
          Serial.print("Relay control: ");
          Serial.print(msgAction);
          Serial.print(" - ");
          Serial.println(msgIndex);
          Relay_Control(msgIndex, msgAction);
        }
      }
    }

//    if (msgType == "SCAN-RFID") {
//        Serial.print("Please move your RFID card near the reader");
//        int retryAttempt = 0;
//        while (!isGetUID) { // Have 10 seconds to read
//            if (retryAttempt >= 10) {
//                break;
//            }
//            RFID_ReadCardUID();
//            Serial.print(".");
//            delay(1000);
//            retryAttempt++;
//        }
//        if (msgValue == uidString) {
//            jsonString = "";
//            jsonDoc["topic"] = "room-access";
//            jsonDoc["value"] = 1;
//            serializeJson(jsonDoc, jsonString);
//            myMQTT.publish(PUBLISH_TOPIC, jsonString.c_str());
//        } else {
//            jsonString = "";
//            jsonDoc["topic"] = "room-access";
//            jsonDoc["value"] = 0;
//            serializeJson(jsonDoc, jsonString);
//            myMQTT.publish(PUBLISH_TOPIC, jsonString.c_str());
//        }
//        isGetUID = false;
//        uidString = "";
//    } else if (msgType == "LIGHT-CONTROL") {
//        if (msgValue == "0") {
//            M5.dis.fillpix(0x000000);
//        } else {
//            M5.dis.fillpix(0xff0000);
//        }
//    }
    memset(subscribeMessage, 0, sizeof(subscribeMessage));
    //  delay(5000);
}

void setup() {
    // put your setup code here, to run once:
    #ifdef M5_CORE2
      M5.begin(true, false, true, true); // Start M5Core2 
    #else
      M5.begin(true, false, true); // Start M5Atom
    #endif
    Wire.begin();                 // Start Wire library
    Serial.begin(115200);         // Start Serial Monitor
    PaHub_Init();
    if (RFID_UNIT_CONNECTED) {
      RFID.PCD_Init();  // Start RFID Unit
      RFID_GetVersion();
    }
    AC_Init();
    ENV_Init();
    Relay_Init();
   
    #ifdef M5_CORE2
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(0, 220, 2);
      M5.Lcd.print("    On/Off relay    Close door");
      M5.Lcd.setCursor(0, 0);  // Set the cursor position
      //M5.Lcd.print("SMART HOMESTAY\n\n");
      M5.Lcd.setTextColor(BLUE);
      M5.Lcd.drawCentreString("SMART HOMESTAY", 160, 0, 4);
      M5.Lcd.drawFastHLine(0, 30, 320, BLUE);
      M5.Lcd.setCursor(0, 50, 4);
      M5.Lcd.print("Temperature");
      M5.Lcd.setCursor(0, 85, 4);
      M5.Lcd.print("Humidity");
      M5.Lcd.setCursor(0, 120, 4);
      M5.Lcd.print("Power usage");
      M5.Lcd.setCursor(0, 155, 4);
      M5.Lcd.print("RELAYS");
//      M5.Lcd.print("   Lamp: \n   Temp: \n   Humi: \n   \n   \n   Bill: \n");
    #endif
    connect_wifi();
    myMQTT.connect();
    if (ACM_UNIT_CONNECTED) {
      String getKwhStr = HTTP_Get("http://172.28.182.60:8000/api/data/power-usage/latest");
      DynamicJsonDocument kwhDoc(1024);
      deserializeJson(kwhDoc, getKwhStr);
      kwhInitValue = kwhDoc["data"]["value"].as<String>().toFloat();
      Serial.print("kwh string: ");
      Serial.println(getKwhStr);
      Serial.print("kwh Value: ");
      Serial.println(kwhInitValue);
      //acm.setKWH(kwhInitValue * 100.0); 

      //AC_ReadKWH();
      TOTAL_POWER_CONSUMED = kwhInitValue;
      #ifdef M5_CORE2
        M5.Lcd.fillRect(170, 120, 200, 20, BLACK);
        M5.Lcd.setCursor(170, 120, 4);
        M5.Lcd.printf("%.2f kWh", TOTAL_POWER_CONSUMED);
      #endif
    }
    //ENV_ReadValue();
    //myMQTT.subscribe(SUBSCRIBE_TOPIC);
//    Serial.print("Subscribed to topic: ");
//    Serial.println(SUBSCRIBE_TOPIC);
}

long env4_delayTimer = 0, acm_delayTimer = 0;
bool isSetKWH = false;
void loop() {
    // put your main code here, to run repeatedly:
    M5.update();
    myMQTT.checkConnection();
    processSubscribeMessage();
    //RFID_ReadCardUID();
    //delayCnt++;
    if (ENV4_UNIT_CONNECTED) {
      if (millis() - env4_delayTimer >= 30000) {
        ENV_ReadValue();
        env4_delayTimer = millis();
      }
    }
    if (ACM_UNIT_CONNECTED) {
      if (millis() - acm_delayTimer >= 30000) {
        //AC_ReadPower();
        acm_delayTimer = millis();
        AC_ReadKWH();
      }
    }
    //Relay_Control();

#ifdef M5_CORE2
  if (RELAY_UNIT_CONNECTED) {
    if (M5.BtnA.wasPressed()) {
      relayFlag = !relayFlag;
      if (relayFlag) {
        M5.Lcd.fillRect(170, 155, 150, 20, BLACK);
        M5.Lcd.setCursor(170, 155, 4);
//        M5.Lcd.printf("ON");

        for (int i = 0; i < 4; i++) {
          Relay_Control(i, 1);
        }
        HTTP_Post("http://172.28.182.60:8000/api/data/socket-1/create", "{\"value\":\"1\"}");
        HTTP_Post("http://172.28.182.60:8000/api/data/socket-2/create", "{\"value\":\"1\"}");
        HTTP_Post("http://172.28.182.60:8000/api/data/socket-3/create", "{\"value\":\"1\"}");
        HTTP_Post("http://172.28.182.60:8000/api/data/socket-4/create", "{\"value\":\"1\"}");
      }
      else {
        M5.Lcd.fillRect(170, 155, 150, 20, BLACK);
        M5.Lcd.setCursor(170, 155, 4);
        //M5.Lcd.printf("OFF");
        for (int i = 0; i < 4; i++) {
          Relay_Control(i, 0);
        }
        HTTP_Post("http://172.28.182.60:8000/api/data/socket-1/create", "{\"value\":\"0\"}");
        HTTP_Post("http://172.28.182.60:8000/api/data/socket-2/create", "{\"value\":\"0\"}");
        HTTP_Post("http://172.28.182.60:8000/api/data/socket-3/create", "{\"value\":\"0\"}");
        HTTP_Post("http://172.28.182.60:8000/api/data/socket-4/create", "{\"value\":\"0\"}");
      }
    }
  }
  if (RFID_UNIT_CONNECTED && M5.BtnA.wasPressed()) {
    
  }
  if (M5.BtnB.wasPressed()) {
    if (RFID_UNIT_CONNECTED) {
      String myJsonString = "";
      DynamicJsonDocument myDJsonDoc(1024);
      myDJsonDoc["type"] = "publish";
      JsonArray jsonArr = myDJsonDoc.createNestedArray("data");
      JsonObject tempObject = jsonArr.createNestedObject();
      tempObject["topic"] = "room-access";
      tempObject["value"] = 0; 
      serializeJson(myDJsonDoc, myJsonString);
      myMQTT.publish(PUBLISH_TOPIC, myJsonString.c_str());
    }
  }
#endif
//    delay(500);
}
