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

// CONFIGURATION PARAMETERS
const char ssid[] = SSID_NAME;
const char pass[] = WPA_PASSWORD;
const char server[] = MQTT_SERVER_ADDR;

// MQTT TOPIC
char topic[] = MQTT_TOPIC;


// VARIABLE DEFINITIONS
int status = WL_IDLE_STATUS;
unsigned int cycle_counter = 0;

float last_temp = -1;	// magic values to ensure dataChanged() passes on first
float last_hum = -1;	// cylce

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

int cycle();

bool dataChanged(float temp, float hum);


// CLASS INSTANCES
WiFiClient wifi_client;
PubSubClient mqtt_client(server, 1883, callback, wifi_client);
DHT dht22(DHTPIN, DHT22);



// FUNCTION DEFINITIONS
void wifiConnect() {
	Log.notice("[!] WiFi connection status: %d\n", status);

	while (status != WL_CONNECTED) {
		Log.verbose("[*] Attempting connection to WPA SSID: %s\n", ssid);
		status = WiFi.begin(ssid, pass);
		delay(10000); // wait 10 seconds for connection
	}
	Log.notice("[+] WiFi connection (re)established.\n");
}

void mqttConnect() {
	Log.notice("[!] Attempting connection to MQTT broker: %s\n", server);
	while (!mqtt_client.connect( (char*) cid.c_str() )) {
		Log.warning("[-] Connection failed to MQTT broker.\n");

		// edge case if WiFi DC'd before MQTT could init
		if (diagnose() == WIFI_DISCONNECTED) { wifiConnect(); }

		Log.trace("[!] Retrying in 5 seconds.\n");
		delay(5000);
		Log.trace("[!] Retrying...\n");
	}
	Log.notice("[+] Successfully connected to MQTT broker: %s\n", server);
	Log.trace("[+] Publishing to topic: %s\n", topic);
}

void generateCID() {
	byte mac[6]; WiFi.macAddress(mac);
	cid = "iot-33::" + macToString(mac);
}

bool mqttPublish(String payload) {
	Log.trace("[!] Attempting to publish on topic: %s\n", topic);
	Log.trace("[!] Payload: %s\n", payload.c_str());

	if (mqtt_client.publish(topic, (char*) payload.c_str())) {
		Log.trace("[+] Publish OK.\n");
		return true;
	} else {
		Log.warning("[-] Publish failed.\n");
		return false;
	}
}

void printInfo() {
	Log.verbose("[!] Printing device and status info: \n");

	Log.verbose("WiFi Firmware Version: %s\n", WiFi.firmwareVersion());

	Log.verbose("WiFi connection status: %d\n", status);
	if (status == WL_CONNECTED) {

		Log.verbose("SSID: %s\n", WiFi.SSID());

		byte bssid[6]; WiFi.BSSID(bssid);
		Log.verbose("BSSID: %s\n", macToString(bssid).c_str());

		IPAddress ip = WiFi.localIP();
		Log.verbose("IP Address: %s\n", String(ip).c_str());

		byte mac[6]; WiFi.macAddress(mac);
		Log.verbose("MAC Address: %s\n", macToString(mac).c_str());

		long rssi = WiFi.RSSI();
		Log.verbose("Signal strength (RSSI): %l\n", rssi);

		Log.verbose("Using MQTT ClientID: %s\n", cid.c_str());
		Log.verbose("Server IP Addr: %s\n", server);
		Log.verbose("Topic: %s\n", topic);


		if (mqtt_client.connected()) {
			Log.verbose("MQTT Status: connected\n");
		} else {
			Log.verbose("MQTT Status: disconnected\n");
		}
		Log.verbose("MQTT state int: %d\n", mqtt_client.state());


	} else {
		Log.verbose("No WiFi Connection established.\n");
	}

	Log.verbose("[!] End of device and status info.\n");
}

int diagnose() {
	// check WiFi connection
	Log.trace("[!] Running diagnostics... \n");
	if (status != WL_CONNECTED) {
		Log.trace("[-] WiFi disconnected!\n");
		return WIFI_DISCONNECTED;
	}

	if (mqtt_client.state() != 0) {
		Log.trace("[-] MQTT Broker disconnected; state int: %d\n", mqtt_client.state());
		return MQTT_BROKER_DISCONNECTED;
	}

	Log.trace("[+] WiFi and MQTT connections healthy.\n");
	return ALL_HEALTHY;
}

void fixConnections() {
	int condition = diagnose();
	switch (condition) {
		case WIFI_DISCONNECTED:
			Log.trace("[!] Running WiFi reconnection:\n");
			wifiConnect();
			break;

		case MQTT_BROKER_DISCONNECTED:
			Log.trace("[!] Running MQTT broker reconnection:\n");
			mqttConnect();
			break;

		case ALL_HEALTHY:
			// run through
			break;
		
		default:
			Log.error("[!] Unknown diagnostics integer: %s\n", condition);
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

int cycle() {
	float current_temp = dht22.readTemperature();
	float current_hum = dht22.readHumidity();

	if (dataChanged(current_temp, current_hum)) {
		last_temp = current_temp;
		last_hum = current_hum;

		return mqttPublish(
			formatData(
				current_temp,
				current_hum
			)
		);

	} else {
		last_temp = current_temp;
		last_hum = current_hum;

		return 1;
	}
}

bool dataChanged(float temp, float hum) {
	// TODO: absolute
	return (abs(temp - last_temp) > MEAS_THRESHOLD) || (abs(hum - last_hum) > MEAS_THRESHOLD);
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

	// init dht22
	dht22.begin();

	// connect
	wifiConnect();
	mqttConnect();

	generateCID();
	printInfo();
}



void loop() {
	cycle_counter++;

	if( !mqtt_client.loop() ) {
		// disconnected from MQTT
		fixConnections();
		
	} else {
		
		if (cycle_counter % 10 == 0) {
			// send data irrespective every 10 minutes
			cycle_counter = 0;

			last_temp = dht22.readTemperature();
			last_hum = dht22.readHumidity();

			mqttPublish(formatData(
				last_temp,
				last_hum
			));

		} else {
			// if significant change, send every minute
			cycle();	
		}

		delay(CYCLE_DELAY_MILLIS);
	}
}
