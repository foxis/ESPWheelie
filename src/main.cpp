#include <Arduino.h>
#include <EasyOTA.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <math.h>
#include <SPI.h>
#include "wificonfig.h"
#include "DiffDrive.h"

#define MOTOR_A0  5
#define MOTOR_A1  4
#define MOTOR_B0  0
#define MOTOR_B1  2
#define WHEEL_BASE 1.0


EasyOTA OTA(ARDUINO_HOSTNAME);
DiffDrive ddrive(MOTOR_A0, MOTOR_B0, MOTOR_A1, MOTOR_B1, WHEEL_BASE);
void S_printf(const char * format, ...) {
	char buffer[512];
	va_list args;
	va_start (args, format);
	vsnprintf (buffer, sizeof(buffer), format, args);
	Serial.println(buffer);
	va_end (args);
}
void setup() {
	Serial.begin(115200);
	OTA.addAP(WIFI_SSID, WIFI_PASSWORD);
	OTA.onConnect([](const String& ssid, EasyOTA::STATE state) {
		S_printf("Connected %s, state: %s", ssid.c_str(), state == EasyOTA::EOS_STA ? "Station" : "Access Point");
	});

	OTA.onMessage([](const String& msg, int line) {
		S_printf("OTA message: %s", msg.c_str());
	});
	ddrive.init();
	pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
	unsigned long now = millis();
	static unsigned long last_now = millis();

  OTA.loop(now);

	//if (now - last_now > 500000L) {
	//	digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN) ? LOW : HIGH);
	//	last_now = now;
	//}
}
