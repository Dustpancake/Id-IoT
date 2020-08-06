# Id IoT
Arduino Nano 33 IoT script for connecting to WPA secured WiFi, and subsequently connecting to an MQTT Broker.

Currently will continuously publish `"Hello World {i}"` with incrementing `i` on a `test_topic` MQTT topic. Features include

- automatic disconnect recovery for both WiFi and MQTT
- configurable login defined in a `secrets.h` header file
- debug mode, which prints verbose status updates across the serial connection

## Setup
To setup, create the file `include/secrets.h` and populate the contents with the following configuration parameters
```c
#ifndef SECRETS_H
#define SECRETS_H 

#define SSID_NAME "SSID_OF_YOUR_WIFI"
#define WPA_PASSWORD "WIFI_PASSWORD"

#define MQTT_SERVER_ADDR "SERVER_ADDRESS"

#define DEBUG 1	// set to 0 for production

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
