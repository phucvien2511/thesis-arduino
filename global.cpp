#include "global.h"

// String MQTT_SERVER = "io.adafruit.com";
// String MQTT_USERNAME = "vienminhphuc";
// String MQTT_PASSWORD = "aio_SXbj67lOqttFFJ1BaBT9BsHhqZTs";
// String PUBLISH_TOPIC = "vienminhphuc/feeds/room-access";
// String SUBSCRIBE_TOPIC = "vienminhphuc/feeds/room-access";
String M5_MAC_ADDR = "64b70882fdc8";
String MQTT_SERVER = "mqttserver.tk";
String MQTT_USERNAME = "bksmarthotel";
String MQTT_PASSWORD = "BKSmartHotel_jen4BBXaJp";
String PUBLISH_TOPIC = "/bk/smarthotel/devicemonitoring/" + M5_MAC_ADDR + "/test";
String SUBSCRIBE_TOPIC = "/bk/smarthotel/devicemonitoring/" + M5_MAC_ADDR + "/+";

MQTTHelper myMQTT(MQTT_SERVER, MQTT_USERNAME, MQTT_PASSWORD);

MFRC522 RFID(RFID_ADDR);

UNIT_ACMEASURE acm;

TCA9548 pahub(PAHUB_ADDR);

FingerPrint Finger;
uint8_t fingerResponse;

SHT4X sht4;

UNIT_4RELAY relay;
