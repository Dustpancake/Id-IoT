#include "Arduino.h"

#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoLog.h>

#include "secrets.h"


// MACRO VARIABLES
#define WIFI_DISCONNECTED -3
#define MQTT_BROKER_DISCONNECTED -2
#define ALL_HEALTHY 0

#define MESSAGE_DELAY_MILLIS 30000

// CONFIGURATION PARAMETERS
const char ssid[] = SSID_NAME;
const char pass[] = WPA_PASSWORD;
const char server[] = MQTT_SERVER_ADDR;

// MQTT TOPIC
char topic[] = MQTT_TOPIC;


// VARIABLE DEFINITIONS
int status = WL_IDLE_STATUS;
String cid;

// FUNCTION DECLERATIONS
void wifiConnect();
void mqttConnect();

void generateCID();

bool mqttPublish(char* payload);

void printInfo();

int diagnose();

void fixConnections();

void callback(char* topic, byte* payload, unsigned int length);

String macToString(byte* mac);

String formatData(float temp, float humidity);


// CLASS INSTANCES
WiFiClient wifi_client;
PubSubClient mqtt_client(server, 1883, callback, wifi_client);
DHT dht22(DHTPIN, DHT22);



// FUNCTION DEFINITIONS
void wifiConnect() {
	Log.notice("[!] WiFi connection status: %d", status);

	while (status != WL_CONNECTED) {
		Log.verbose("[*] Attempting connection to WPA SSID: %s", ssid);
		status = WiFi.begin(ssid, pass);
		delay(10000); // wait 10 seconds for connection
	}
	Log.notice("[+] WiFi connection (re)established.");
}

void mqttConnect() {
	Log.notice("[!] Attempting connection to MQTT broker: %s", server);
	while (!mqtt_client.connect( (char*) cid.c_str() )) {
		Log.warning("[-] Connection failed to MQTT broker.");

		// edge case if WiFi DC'd before MQTT could init
		if (diagnose() == WIFI_DISCONNECTED) { wifiConnect(); }

		Log.trace("[!] Retrying in 5 seconds.");
		delay(5000);
		Log.trace("[!] Retrying...");
	}
	Log.notice("[+] Successfully connected to MQTT broker: %s", server);
	Log.trace("[+] Publishing to topic: %s", topic);
}

void generateCID() {
	byte mac[6]; WiFi.macAddress(mac);
	cid = "iot-33::" + macToString(mac);
}

bool mqttPublish(String payload) {
	Log.trace("[!] Attempting to publish on topic: %s", topic);
	Log.trace("[!] Payload: %s", payload);

	if (mqtt_client.publish(topic, (char*) payload.c_str())) {
		Log.trace("[+] Publish OK.");
		return true;
	} else {
		Log.warning("[-] Publish failed.");
		return false;
	}
}

void printInfo() {
	Log.verbose("[!] Printing device and status info: ");

	Log.verbose("WiFi Firmware Version: %s", WiFi.firmwareVersion());

	Log.verbose("WiFi connection status: %d", status);
	if (status == WL_CONNECTED) {

		Log.verbose("SSID: %s", WiFi.SSID());

		byte bssid[6]; WiFi.BSSID(bssid);
		Log.verbose("BSSID: %s", macToString(bssid));

		IPAddress ip = WiFi.localIP();
		Log.verbose("IP Address: %s", ip);

		byte mac[6]; WiFi.macAddress(mac);
		Log.verbose("MAC Address: %s", macToString(mac));

		long rssi = WiFi.RSSI();
		Log.verbose("Signal strength (RSSI): %l", rssi);

		Log.verbose("Using MQTT ClientID: %s", cid);
		Log.verbose("Server IP Addr: %s", server);
		Log.verbose("Topic: %s", topic);


		if (mqtt_client.connected()) {
			Log.verbose("MQTT Status: connected");
		} else {
			Log.verbose("MQTT Status: disconnected");
		}
		Log.verbose("MQTT state int: %d", mqtt_client.state());


	} else {
		Log.verbose("No WiFi Connection established.");
	}

	Log.verbose("[!] End of device and status info.");
}

int diagnose() {
	// check WiFi connection
	Log.trace("[!] Running diagnostics... ");
	if (status != WL_CONNECTED) {
		Log.trace("[-] WiFi disconnected!");
		return WIFI_DISCONNECTED;
	}

	if (mqtt_client.state() != 0) {
		Log.trace("[-] MQTT Broker disconnected; state int: ", mqtt_client.state());
		return MQTT_BROKER_DISCONNECTED;
	}

	Log.trace("[+] WiFi and MQTT connections healthy.");
	return ALL_HEALTHY;
}

void fixConnections() {
	int condition = diagnose();
	switch (condition) {
		case WIFI_DISCONNECTED:
			Log.trace("[!] Running WiFi reconnection:");
			wifiConnect();
			break;

		case MQTT_BROKER_DISCONNECTED:
			Log.trace("[!] Running MQTT broker reconnection:");
			mqttConnect();
			break;

		case ALL_HEALTHY:
			// run through
			break;
		
		default:
			Log.error("[!] Unknown diagnostics integer: ", condition);
			delay(10000);
	}
}

void callback(char* topic, byte* payload, unsigned int length) {
	// stub
}

String macToString(byte* mac) {
	String result;
	for (int i = 0; i < 6; ++i) {

		result += String(mac[i], 16);

		if (i < 5) result += '-';
	}
	return result;
}

String formatData(float temp, float humidity) {
	return String(String(temp, 3) + "," + String(humidity, 3));
}


// ARDUINO SETUP AND LOOP

void setup() {

	#if LOG_LEVEL != 0

	Serial.begin(9600);
	while (!Serial) {
		; // wait for serial connection
	}

	Log.begin(LOG_LEVEL, &Serial);

	#endif


	// connect
	wifiConnect();
	mqttConnect();

	generateCID();
	printInfo();
}



void loop() {
	if( !mqtt_client.loop() ) {
		// disconnected from MQTT
		fixConnections();
		
	} else {
		// connected to MQTT
		if ( mqttPublish(
				formatData(
					dht22.readTemperature(),
					dht22.readHumidity()
				)
			)
		) {
			// successfully published
		} else {
			// retry same message
		}
		delay(MESSAGE_DELAY_MILLIS);
	}
}
