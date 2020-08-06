### Create `secrets.h` here
The format is
```c
#ifndef SECRETS_H
#define SECRETS_H 

#define SSID_NAME "SSID_OF_YOUR_WIFI"
#define WPA_PASSWORD "WIFI_PASSWORD"

#define MQTT_SERVER_ADDR "SERVER_ADDRESS"

#define DEBUG 1	// set to 0 for production

#endif
```