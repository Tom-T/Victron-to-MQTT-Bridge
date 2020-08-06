#ifndef settings_h
#define settings_h

#define VICTRON_RX_PIN D1
#define VICTRON_TX_PIN D8     // TX can be disconnected.

#define WIFI_SSID "MyWifi"
#define WIFI_PASSWORD "myWifipassword"

#define MQTT_HOST IPAddress(192, 168, 100, 1)
#define MQTT_PORT 1883
#define MQTT_TOPIC "Victron"
#define MQTT_USER "user"
#define MQTT_PASSWORD "password"

#endif