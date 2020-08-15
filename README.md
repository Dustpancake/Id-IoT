# Id IoT
Arduino Nano 33 IoT script for connecting to WPA secured WiFi, and subsequently connecting to an MQTT Broker.

Features include

- automatic disconnect recovery for both WiFi and MQTT
- configurable login defined in a `secrets.h` header file
- debug mode, which prints verbose status updates across the serial connection
- send temperature and humidity data from a DHT22 device

In the current configuration, will probe the data every `CYCLE_DELAY_MILIS` milliseconds, and if change is greater than `MEAS_THRESHOLD`, will send to the MQTT broker. Otherwise, will wait 10 times `CYCLE_DELAY_MILIS` before sending.

## Setup
To setup, create the file `include/secrets.h` and populate the contents with the following configuration parameters
```c
#ifndef SECRETS_H
#define SECRETS_H 

#define SSID_NAME "SSID_OF_YOUR_WIFI"
#define WPA_PASSWORD "WIFI_PASSWORD"

#define MQTT_SERVER_ADDR "SERVER_ADDRESS"
#define MQTT_TOPIC "TOPIC_NAME"

#define LOG_LEVEL 6	// set to 0 for production
#define DHTPIN 7

#define CYCLE_DELAY_MILLIS 60000 // one minute
#define MEAS_THRESHOLD 0.2

#endif
```

## Running
Developed using PlatformIO, the entire code can be executed and uploaded to a IoT device by running
```
pio run -t upload
```

If debug mode is enabled, code execution is halted until a serial connection at baud rate 9600 is established. Using PlatformIO, this is as simple as
```
pio device monitor
```
