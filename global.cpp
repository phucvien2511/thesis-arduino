#include "global.h"

String M5_MAC_ADDR = "YOUR_MAC_ADDR_HERE";
String MQTT_SERVER = "mqtt_server_url";
String MQTT_USERNAME = "mqtt_username";
String MQTT_PASSWORD = "mqtt_password";
String PUBLISH_TOPIC = "pub_topic";
String SUBSCRIBE_TOPIC = "sub_topic";

MQTTHelper myMQTT(MQTT_SERVER, MQTT_USERNAME, MQTT_PASSWORD);

MFRC522 RFID(RFID_ADDR);

UNIT_ACMEASURE acm;

TCA9548 pahub(PAHUB_ADDR);

FingerPrint Finger;
uint8_t fingerResponse;

SHT4X sht4;

UNIT_4RELAY relay;
