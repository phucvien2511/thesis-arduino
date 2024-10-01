#ifndef GLOBAL_H
#define GLOBAL_H

#define M5_CORE2

#pragma once
#include <Wire.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> // Documentation: https://arduinojson.org/
#include "M5_FPC1020A.h" // Fingerprint Unit: http://docs.m5stack.com/en/unit/finger
#include "MFRC522_I2C.h" // RFID Unit: https://docs.m5stack.com/en/unit/rfid
#include "UNIT_ACMEASURE.h" //AC Measure Unit: https://docs.m5stack.com/en/unit/AC%20Measure%20Unit
#include "mqtt_helper.h"
#include "TCA9548.h" //Reference: https://github.com/RobTillaart/TCA9548/blob/master/examples/tca9548_search_device/tca9548_search_device.ino
#include "M5UnitENV.h"
#include "UNIT_4RELAY.h"

// Define variable to store subscribe message
extern char subscribeMessage[256];

// Define addresses/variables for units
#define PAHUB_ADDR              0x70
#define PAHUB_MAX_CHANNELS      6
#define RFID_ADDR               0x28
#define ACMEASURE_ADDR          0x42
#define ENV4_ADDR               0x44
#define RELAY_ADDR              0x26

//#define I2C_SDA                 32
//#define I2C_SCL                 33
#ifdef M5_CORE2
  #define I2C_SDA                 32
  #define I2C_SCL                 33
#else 
  #define I2C_SDA                 26
  #define I2C_SCL                 32
#endif

// Define WiFi parameters 
#define WIFI_SSID               "YOUR_WIFI_NAME"
#define WIFI_PASSWORD           "YOUR_WIFI_PASSWORD"

#define LAB_WIFI_SSID           "ACLAB"
#define LAB_WIFI_PASSWORD       "ACLAB2023"

// Define available unit connection
#define RFID_UNIT_CONNECTED     true
#define ACM_UNIT_CONNECTED      true
#define PAHUB_UNIT_CONNECTED    true
#define ENV4_UNIT_CONNECTED     true
#define RELAY_UNIT_CONNECTED    true
#define FINGER_UNIT_CONNECTED   false

// Init MQTT variables
extern String M5_MAC_ADDR;
extern String MQTT_SERVER;
extern String MQTT_USERNAME;
extern String MQTT_PASSWORD;
extern String PUBLISH_TOPIC;
extern String SUBSCRIBE_TOPIC;
extern MQTTHelper myMQTT;

// Init RFID instance
extern MFRC522 RFID;

// Init ACMEASURE instance
extern UNIT_ACMEASURE acm;

// Init PaHub instance
extern TCA9548 pahub;

// Init Fingerprint instance
extern FingerPrint Finger;
extern uint8_t fingerResponse;

// Init SHT4X instance
extern SHT4X sht4;

// Init 4relay instance
extern UNIT_4RELAY relay;

#endif  // GLOBAL_H
