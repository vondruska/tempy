# 🌡️ Tempy

![Tempy Screenshot](https://i.imgur.com/QPWzRAQm.png)

Push readings from my DS18B20 temperature probes into MQTT. Runs on a ESP8266 board. Has a web interface to monitor current temperatures and supports OTA updates. Originally based on [fridgy-temp](https://github.com/fklement/fridgy-temp).

## Why build this?

There are things like [ESPHome](https://esphome.io/) that does this.  But I wanted to tinker with ESP8266 boards plus learn to see how far everything has evolved since my automated pet feeder 10+ years ago.

## Options
Currently, configuration / options like Wifi and MQTT connection settings are housed in an included header file `secrets.h`. An example file is available `secrets_example.h`.

| Key | Description |
| --- | ----------- |
| mqtt_server | Hostname of the MQTT server |
| mqtt_user | Username for the MQTT connection |
| mqtt_pass | Password for the MQTT connection |
| topic_root | Topic root for tempy with an ending slash. `tempy/` is what I use |
| wifi_SSID | SSID of the Wifi Network |
| wifi_password | Wifi network key / password |
| update_username | For OTA updates, the username |
| update_password | For OTA updates, the password |

## MQTT Topics
If using Home Assistant, Tempy publishes MQTT Discovery data upon boot. If enabled in Home Assistant, this will create temperature sensors in Home Assistant with the name `sensor.{probe id}`. From there, you can rename and change the entity id to something more meaningful.

If manually configuring:

| Purpose | Topic | Notes |
| ------- | ----- | ----- |
| State Topic | `{topic_root}/{sensor_id}` | Raw readings will be in Celsius
| Last Will and Testament | `{topic_root}/{chip_id}/lwt` | `ONLINE` / `OFFLINE` for values
| Home Assistant Discovery | `homeassistant/sensor/{sensor_id}/config` |

## HTTP Server
Two routes exist:
* `/` - Web interface to view currently detected probes
* `/update` - OTA updates. Will require HTTP authentication using the credentials from the options above.

## Security
This is a hobby project. The OTA updater does not validate the firmware is from a trusted source. The HTTP server does not support TLS potentially exposing the OTA credentials. MQTT connection does not support TLS either.

Ideally, put these on an isolated network with very limited connectivity.
