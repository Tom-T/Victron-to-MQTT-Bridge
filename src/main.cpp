/*
  main.cpp - Victron to MQTT Bridge

  Created by Tom Tijerina Aug 6, 2020.
  Released into the public domain.
*/

#include <SoftwareSerial.h>
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include <Ticker.h>
#include <settings.h>

// Include Circularbuffer, and limit it to 255.
#define CIRCULAR_BUFFER_XS
#include <CircularBuffer.h>

#define MAXIMUM_BUFFER_SIZE 155


bool discardNext = true; //Used to discard all results the first time. Ensuring we get a complete set.

void BufferResponse();
void SendResponseIfValid();
uint16_t postMQTT(String name, String value);


CircularBuffer<String, 55> dataLine; // buffer capacity is 55
char bufferedChars[MAXIMUM_BUFFER_SIZE]; // an array to store the received data
int bufferedCharsIndex;


// Serial variables
SoftwareSerial VictronSerial(VICTRON_RX_PIN, VICTRON_TX_PIN);         // RX, TX Using Software Serial so we can use the hardware serial to check the ouput

//Wifi Settings
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

void connectToWifi();
void connectToMqtt();
void onWifiConnect(const WiFiEventStationModeGotIP &event);
void onWifiDisconnect(const WiFiEventStationModeDisconnected &event);


//MQTT Broker
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
bool mqttConnected = false; //Used to stop sending messages if we are not connected.


void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttPublish(uint16_t packetId);


void setup()
{
    // Open serial communications and wait for port to open:
    Serial.begin(19200);
    VictronSerial.begin(19200);
    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onPublish(onMqttPublish);
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    connectToWifi();
}

void loop() {
    BufferResponse();
}

void connectToWifi()
{
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt()
{
    Serial.println("Connecting to MQTT...");
    mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
    Serial.println("Connected to Wi-Fi.");
    connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
    Serial.println("Disconnected from Wi-Fi.");
    mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    wifiReconnectTimer.once(2, connectToWifi);
    mqttConnected = false;
}

void onMqttConnect(bool sessionPresent)
{
    Serial.println("Connected to MQTT.");
    Serial.print("Session present: ");
    Serial.println(sessionPresent);
    mqttConnected = true;
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    Serial.println("Disconnected from MQTT.");

    if (WiFi.isConnected())
    {
        mqttReconnectTimer.once(2, connectToMqtt);
    }
    mqttConnected = false;
}

void onMqttPublish(uint16_t packetId)
{
    Serial.printf("PacketId: %d, Publish acknowledged.", packetId);
    return;
}

uint16_t postMQTT(String name, String value)
{
    //Remove invalid topics.
    name.replace("+", "");
    name.replace("#", "");
    char TopicString[50];
    sprintf(TopicString, "%s/%s", MQTT_TOPIC, name.c_str());

    if (strcmp(name.c_str(), "Checksum") == 0) {
        return 0;
    }
    if (name.length() && value.length()) {
        int packetID = mqttClient.publish(TopicString, 0, true, value.c_str());
        // Serial.printf("Sending [%s] to [%s]\r\n", TopicString, value.c_str());
        return packetID;
    }

    return 0;
}

void BufferResponse()
{
    char rc;
    while (VictronSerial.available() > 0)
    {
        rc = VictronSerial.read();

        bufferedChars[bufferedCharsIndex] = rc;
        bufferedCharsIndex++;

        if (bufferedCharsIndex >= MAXIMUM_BUFFER_SIZE)
        {
            bufferedCharsIndex = MAXIMUM_BUFFER_SIZE - 1; //Ensure we don't go over our maximum size.
        }
        if (rc == '\n')
        {
            bufferedChars[bufferedCharsIndex] = 0; //End the string.
            dataLine.push(bufferedChars);

            //Reset buffers used.
            bufferedCharsIndex = 0;

            // The last field of a block is always the Checksum
            if (strcmp(strtok(bufferedChars, "\t"), "Checksum") == 0) {
                if (discardNext)
                {
                    dataLine.clear();
                    discardNext = false;
                }
                SendResponseIfValid();
            }
            return;
        }
        yield();
    }
}

void SendResponseIfValid()
{
    // Sum off all received bytes should be 0
    uint8_t checksum = 0;

    for (int x = 0; x < dataLine.size(); x++) {
        for (int i = 0; i < dataLine[x].length(); i++) {
            checksum += dataLine[x].charAt(i);
        }
    }

    //Did we get valid data?
    if (checksum == 0) {
        char *name;
        char *value;
        while (dataLine.isEmpty() == false) {
            String thisline = dataLine.pop();

            char receivedChars[MAXIMUM_BUFFER_SIZE];
            memcpy(receivedChars, thisline.c_str(), thisline.length());

            name = strtok(receivedChars, "\t"); // get the first value
            value = strtok(NULL, "\r"); // Get the next value

            if ((strcmp(name, "Checksum") != 0)  && mqttConnected) postMQTT(name, value);
        }
        Serial.println("Send data to mqtt server");
    }
    else
    {
        Serial.printf("Checksum failed. Got %d insead of 0.\r\n", checksum);
        dataLine.clear();
    }
}