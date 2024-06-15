#ifndef MQTT_HELPER_H
#define MQTT_HELPER_H

#include <PubSubClient.h>
#include <WiFi.h>

class MQTTHelper {
   private:
    String username;
    String password;
    String mqttServer;
    WiFiClient wifiClient;
    PubSubClient client;

   public:
    MQTTHelper(String mqttServer, String username, String password) : mqttServer(mqttServer), username(username), password(password), client(wifiClient) {
        client.setBufferSize(2048);
    }

    void connect();
    bool publish(String, String);  // topic, payload
    void subscribe(String);        // topic
    void checkConnection();

   private:
    void reconnect();                           // check connection eventually
    void callback(char*, byte*, unsigned int);  // check if message is received
};

#endif  // MQTT_HELPER_H
