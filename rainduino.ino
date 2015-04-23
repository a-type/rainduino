#include "rainduino_config.h"

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

#define ADAFRUIT_CC3000_IRQ 3 // interrupt pin
#define ADAFRUIT_CC3000_VBAT 5 //any pin
#define ADAFRUIT_CC3000_CS 10 //any pin

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER);

#define DHCP_RETRIES 128
#define IDLE_TIMEOUT_MS 3000
// two 3-digit temp, one 3-digit condition code
#define BUFFER_SIZE 9

void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);
	Serial.println(F("Starting Rainduino\n"));

	displayDriverMode();
	Serial.print(F("Free RAM: "));
	Serial.println(getFreeRam(), DEC);

	Serial.print(F("Forecast API is "));
	Serial.println(API_HOST);

	Serial.println(F("Initializing..."));
	if (!cc3000.begin()) {
		Serial.println(F("Unable to initialize the CC3000."));
		while(1);
	}

	uint16_t firmware = checkFirmwareVersion();
	if (firmware < 0x113) {
		Serial.println(F("Wrong firmware version."));
		while(1);
	}

	displayMACAddress();

	Serial.println(F("Deleting old connection profiles."));
	if (!cc3000.deleteProfiles()) {
		Serial.println(F("Failed to delete old profiles."));
		while(1);
	}

	char *ssid = WLAN_SSID;
	Serial.print(F("Attempting to connect to "));
	Serial.println(ssid);

	if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
		Serial.println(F("Failed to connect to AP."));
		while(1);
	}

	Serial.println(F("Connected."));

	Serial.println(F("DHCP running..."));
	byte retries = 0;
	while (!cc3000.checkDHCP()) {
		delay(100);
		if (retries++ > DHCP_RETRIES) {
			Serial.println(F("DHCP timed out."));
			while(1);
		}
	}

	while (!displayConnectionDetails()) {
		delay(1000);
	}

	getForecast();

	Serial.println(F("disconnecting."));
	cc3000.disconnect();
}

void loop() {
	// put your main code here, to run repeatedly:

}

/**************************************************************************/
/*!
	 @brief  Displays the driver mode (tiny of normal), and the buffer
				size if tiny mode is not being used

	 @note   The buffer size and driver mode are defined in cc3000_common.h
*/
/**************************************************************************/
void displayDriverMode(void)
{
	#ifdef CC3000_TINY_DRIVER
		erial.println(F("CC3000 is configure in 'Tiny' mode"));
	#else
		Serial.print(F("RX Buffer : "));
		Serial.print(CC3000_RX_BUFFER_SIZE);
		Serial.println(F(" bytes"));
		Serial.print(F("TX Buffer : "));
		Serial.print(CC3000_TX_BUFFER_SIZE);
		Serial.println(F(" bytes"));
	#endif
}

/**************************************************************************/
/*!
	 @brief  Tries to read the CC3000's internal firmware patch ID
*/
/**************************************************************************/
uint16_t checkFirmwareVersion(void)
{
	uint8_t major, minor;
	uint16_t version;
	
#ifndef CC3000_TINY_DRIVER  
	if(!cc3000.getFirmwareVersion(&major, &minor))
	{
		Serial.println(F("Unable to retrieve the firmware version!\r\n"));
		version = 0;
	}
	else
	{
		Serial.print(F("Firmware V. : "));
		Serial.print(major); Serial.print(F(".")); Serial.println(minor);
		version = major; version <<= 8; version |= minor;
	}
#endif
	return version;
}

/**************************************************************************/
/*!
	 @brief  Tries to read the 6-byte MAC address of the CC3000 module
*/
/**************************************************************************/
void displayMACAddress(void)
{
	uint8_t macAddress[6];
	
	if(!cc3000.getMacAddress(macAddress))
	{
		Serial.println(F("Unable to retrieve MAC Address!\r\n"));
	}
	else
	{
		Serial.print(F("MAC Address : "));
		cc3000.printHex((byte*)&macAddress, 6);
	}
}

/**************************************************************************/
/*!
	 @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
	uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
	
	if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
	{
		Serial.println(F("Unable to retrieve the IP Address!\r\n"));
		return false;
	}
	else
	{
		Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
		Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
		Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
		Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
		Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
		Serial.println();
		return true;
	}
}

/*******************************************************/
/*!
	@brief Connects to forecast API and retrieves forcast

	@note The forecast host, port and endpoint are specified through config

*/
/*******************************************************/
void getForecast() {
	uint32_t ip = 0;
	Serial.print(API_HOST);
	Serial.print(F(" ->"));
	while (ip == 0) {
		if (! cc3000.getHostByName(API_HOST, &ip)) {
			Serial.println(F("Couldn't resolve."));
		}
		delay(500);
	}
	cc3000.printIPdotsRev(ip);
	Serial.println(F("\r\n"));

	Adafruit_CC3000_Client www = cc3000.connectTCP(ip, API_PORT);
	if (www.connected()) {
		www.fastrprint(F("GET "));
		www.fastrprint(API_PAGE);
		www.fastrprint(F(" HTTP/1.1\r\n"));
		www.fastrprint(F("Host: "));
		www.fastrprint(API_HOST);
		www.fastrprint(F("\r\n\r\n"));
		www.println();
	}
	else {
		Serial.println(F("Connection failed"));
		return;
	}

	Serial.println(F(">---------------------------"));

	char buffer[BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));

	char c;
	while((c = www.read()) != '#') {
		Serial.print(c);
	}
	Serial.println("++++++++++++++++++++");

	if (www.read(buffer, BUFFER_SIZE)) {
		Serial.println(buffer);
	// 	// Serial.println(F("Conditions:"));
	// 	// Serial.println(conditions);
	// 	// Serial.println(F("High:"));
	// 	// Serial.println(high);
	// 	// Serial.println(F("Low:"));
	// 	// Serial.println(low);
	}
	else {
		Serial.println(F("Error reading from client."));
	}
	www.close();
	Serial.println(F("<---------------------------"));
}
