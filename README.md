# Victron to MQTT bridge

While there seemed to be a few projects with similar interest nothing really hit the mark for me. I wanted something that was adaptable and easy to use. Most notably I wanted a reliable way to ensure good data from the Victron Charge controllers. I suspect this can work with other Victron devices as well, if you test this please let me know!

## Credits:
This project was inspired by Pim Rutgers and his project: https://github.com/physee/Victron.Arduino-ESP8266 However out of the box that implementation didn't work for me and I had another project with ideas I wanted to incorporate and so this repo was born. 

## Getting started
Edit src/settings.h to match your environment. 

```
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
```