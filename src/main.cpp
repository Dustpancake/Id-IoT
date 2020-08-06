#include "Arduino.h"

#include <WiFiNINA.h>
#include <PubSubClient.h>

#include "secrets.h"



// MACROS
#if DEBUG == 1

#define DEBUG_PRINT_1(str1) Serial.println(str1);
#define DEBUG_PRINT_2(str1, str2) Serial.print(str1); Serial.println(str2);

#else

#define DEBUG_PRINT_1(str1) ;
#define DEBUG_PRINT_2(str1, str2) ;

#endif

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
char topic[] = "test_topic";


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


// CLASS INSTANCES
WiFiClient wifi_client;
PubSubClient mqtt_client(server, 1883, callback, wifi_client);



// FUNCTION DEFINITIONS
void wifiConnect() {
	DEBUG_PRINT_1("") // newline
	DEBUG_PRINT_2("[!] WiFi connection status: ", status)

	while (status != WL_CONNECTED) {
		DEBUG_PRINT_2("[*] Attempting connection to WPA SSID: ", ssid)
		status = WiFi.begin(ssid, pass);
		delay(10000); // wait 10 seconds for connection
	}
	DEBUG_PRINT_1("[+] WiFi connection (re)established.")
}

void mqttConnect() {
	DEBUG_PRINT_2("[!] Attempting connection to MQTT broker: ", server)
	while (!mqtt_client.connect( (char*) cid.c_str() )) {
		DEBUG_PRINT_1("[-] Connection failed to MQTT broker.")

		// edge case if WiFi DC'd before MQTT could init
		if (diagnose() == WIFI_DISCONNECTED) { wifiConnect(); }

		DEBUG_PRINT_1("[!] Retrying in 5 seconds.")
		delay(5000);
		DEBUG_PRINT_1("[!] Retrying...")
	}
	DEBUG_PRINT_2("[+] Successfully connected to MQTT broker: ", server)
	DEBUG_PRINT_2("[+] Publishing to topic: ", topic)
}

void generateCID() {
	byte mac[6]; WiFi.macAddress(mac);
	cid = "iot-33::" + macToString(mac);
}

bool mqttPublish(String payload) {
	DEBUG_PRINT_2("[!] Attempting to publish on topic: ", topic)
	DEBUG_PRINT_2("[!] Payload: ", payload)

	if (mqtt_client.publish(topic, (char*) payload.c_str())) {
		DEBUG_PRINT_1("[+] Publish OK.")
		return true;
	} else {
		DEBUG_PRINT_1("[-] Publish failed.")
		return false;
	}
}

void printInfo() {
	DEBUG_PRINT_1("[!] Printing device and status info: ")
	DEBUG_PRINT_1("")	// newline

	DEBUG_PRINT_2("WiFi Firmware Version: ", WiFi.firmwareVersion())

	DEBUG_PRINT_2("WiFi connection status: ", status)
	if (status == WL_CONNECTED) {
		DEBUG_PRINT_1("")	// newline

		DEBUG_PRINT_2("SSID: ", WiFi.SSID())

		byte bssid[6]; WiFi.BSSID(bssid);
		DEBUG_PRINT_2("BSSID: ", macToString(bssid))

		IPAddress ip = WiFi.localIP();
		DEBUG_PRINT_2("IP Address: ", ip)

		byte mac[6]; WiFi.macAddress(mac);
		DEBUG_PRINT_2("MAC Address: ", macToString(mac))

		long rssi = WiFi.RSSI();
		DEBUG_PRINT_2("Signal strength (RSSI): ", rssi)

		DEBUG_PRINT_1("") // newline
		DEBUG_PRINT_2("Using MQTT ClientID: ", cid)
		DEBUG_PRINT_2("Server IP Addr: ", server)
		DEBUG_PRINT_2("Topic: ", topic)


		if (mqtt_client.connected()) {
			DEBUG_PRINT_1("MQTT Status: connected")
		} else {
			DEBUG_PRINT_1("MQTT Status: disconnected")
		}
		DEBUG_PRINT_2("MQTT state int: ", mqtt_client.state())


	} else {
		DEBUG_PRINT_1("No WiFi Connection established.")
	}

	DEBUG_PRINT_1("")
	DEBUG_PRINT_1("[!] End of device and status info.")
}

int diagnose() {
	// check WiFi connection
	DEBUG_PRINT_1("[!] Running diagnostics... ")
	if (status != WL_CONNECTED) {
		DEBUG_PRINT_1("[-] WiFi disconnected!")
		return WIFI_DISCONNECTED;
	}

	if (mqtt_client.state() != 0) {
		DEBUG_PRINT_2("[-] MQTT Broker disconnected; state int: ", mqtt_client.state())
		return MQTT_BROKER_DISCONNECTED;
	}

	DEBUG_PRINT_1("[+] WiFi and MQTT connections healthy.")
	return ALL_HEALTHY;
}

void fixConnections() {
	int condition = diagnose();
	switch (condition) {
		case WIFI_DISCONNECTED:
			DEBUG_PRINT_1("[!] Running WiFi reconnection:")
			wifiConnect();
			break;

		case MQTT_BROKER_DISCONNECTED:
			DEBUG_PRINT_1("[!] Running MQTT broker reconnection:")
			mqttConnect();
			break;

		case ALL_HEALTHY:
			// run through
			break;
		
		default:
			DEBUG_PRINT_2("[!] Unknown diagnostics integer: ", condition)
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


// ARDUINO SETUP AND LOOP

void setup() {

	#if DEBUG == 1

	Serial.begin(9600);
	while (!Serial) {
		; // wait for serial connection
	}

	#endif

	// connect
	wifiConnect();
	mqttConnect();

	generateCID();
	printInfo();
}


int i = 0;
void loop() {
	if( !mqtt_client.loop() ) {
		// disconnected from MQTT
		fixConnections();
		
	} else {
		// connected to MQTT
		if ( mqttPublish("Hello world " + String(i)) ) {
			// successfully published
			i++;
		} else {
			// retry same message
		}
		delay(MESSAGE_DELAY_MILLIS);
	}
}
