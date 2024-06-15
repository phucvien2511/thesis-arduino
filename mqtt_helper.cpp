#include "mqtt_helper.h"
char subscribeMessage[256];
void MQTTHelper::connect() {
    client.setServer(mqttServer.c_str(), 1883);
    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
        //          Serial.print("Current subscribeMessage - in callback: ");
        //          Serial.println(subscribeMessage);
    });
    reconnect();
}

bool MQTTHelper::publish(String topic, String payload) {
#ifdef ADAFRUIT
    String topic = user + "/feeds/" + topic;
#endif
    //    Serial.print("Publishing to topic: ");
    //    Serial.println(topic);
    //    Serial.print("Status: ");
    if (client.publish(topic.c_str(), payload.c_str(), 1)) {
        Serial.println("Publish success!");
        return 1;
    }
    Serial.println("Publish failed!");
    return 0;
}

void MQTTHelper::subscribe(String topic) {
#ifdef ADAFRUIT
    String topic = user + "/feeds/" + topic;
#endif
    if (client.subscribe(topic.c_str())) {
        Serial.println("Subscribed!");
    }
}

void MQTTHelper::checkConnection() {
    if (!client.connected()) {
        Serial.println("MQTT Connection lost!");
        reconnect();
    }
    client.loop();
}

void MQTTHelper::callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
    strncpy(subscribeMessage, (char*)payload, length);
}

void MQTTHelper::reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("client", username.c_str(), password.c_str())) {
            Serial.println("Connected!");
            MQTTHelper::subscribe("/bk/smarthotel/devicemonitoring/64b70882fdc8/+");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}
