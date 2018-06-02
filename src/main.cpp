#include <Arduino.h>
#include <EasyOTA.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <math.h>
#include "wificonfig.h"
#include "DiffDrive.h"
#include <ArduinoJson.h>
#include "AsyncJson.h"
#include "adns3080.h"

#define MOTOR_A0  5
#define MOTOR_B0  4
#define MOTOR_A1  2
#define MOTOR_B1  0
#define WHEEL_BASE 1.0
#define IMG_WIDTH ADNS3080_PIXELS_X
#define IMG_HEIGHT ADNS3080_PIXELS_Y
#define SS 16
#define RESET 3

EasyOTA OTA(ARDUINO_HOSTNAME);
DiffDrive ddrive(MOTOR_A0, MOTOR_B0, MOTOR_A1, MOTOR_B1, WHEEL_BASE);
bool connected = false;
unsigned long last_updated = 0;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
ADNS3080 a3080(SS, RESET);

void textThem(const char * text) {
	int tryId = 0;
  for (int count = 0; count < ws.count();) {
    if (ws.hasClient(tryId)) {
      ws.client(tryId)->text(text);
      count++;
    }
    tryId++;
  }
}

void S_printf(const char * format, ...) {
//	char buffer[512];
//	va_list args;
//	va_start (args, format);
//	vsnprintf (buffer, sizeof(buffer), format, args);
//	Serial.println(buffer);
//	va_end (args);
}

void read_image(byte * data, size_t size)
{
	a3080.frame_capture(data);
	data[40] = 40;
	data[41] = 70;
	data[42] = 128;
}

void setup() {
	//Serial.begin(115200);
	OTA.addAP(WIFI_SSID, WIFI_PASSWORD);
	OTA.onConnect([](const String& ssid, EasyOTA::STATE state) {
		S_printf("Connected %s, state: %s", ssid.c_str(), state == EasyOTA::EOS_STA ? "Station" : "Access Point");
	});

	OTA.onMessage([](const String& msg, int line) {
		S_printf("OTA message: %s", msg.c_str());
	});
	ddrive.init();
	a3080.begin();
	SPIFFS.begin();

	server.addHandler(&ws);
	server.serveStatic("/", SPIFFS, "/web").setDefaultFile("index.html");
	server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(200, "text/plain", String(ESP.getFreeHeap()));
	});
	server.onNotFound([](AsyncWebServerRequest *request) { request->send(404); });

	ws.onEvent([](AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
		if (type == WS_EVT_DATA) {
			char cmd[256] = "";
			memcpy(cmd, data, min(len, sizeof(cmd) - 1));
			DynamicJsonBuffer jsonBuffer;
			JsonObject &json = jsonBuffer.parseObject(cmd);

			ddrive.setDiffSpeed(json["power"], json["steering"]);
			last_updated = millis();

		} else if (type == WS_EVT_CONNECT) {
			connected = true;
			a3080.reset();
			S_printf("Connected...");
		} else if (type == WS_EVT_DISCONNECT) {
			connected = false;
			S_printf("Disconnected...");
		}
	});

	server.begin();
}

void loop() {
	unsigned long now = millis();
	static unsigned long last_now = millis();

  OTA.loop(now);

	if (now - last_now > 20L && connected) {
		static byte data[IMG_WIDTH * IMG_HEIGHT];
		read_image(data, sizeof(data));
		float voltage = analogRead(0) * 10.0 / 1024.0;

		//
		static char buffer[4000] = "";
		char tmp[10] = "";
		sprintf(buffer, "{\"battery\":%.2f,\"width\":%i,\"height\":%i,\"heap\":%i,\"image\":[%i",
			voltage, IMG_WIDTH, IMG_HEIGHT, ESP.getFreeHeap(), (int)data[0]);

		for (int i = 1; i < sizeof(data); i++) {
			//image.add((int)data[i]);
			sprintf(tmp, ",%i", (int)data[i]);
			strcat(buffer, tmp);
		}

		strcat(buffer, "]}");

		textThem(buffer);

		last_now = now;
	}

	if (now - last_updated > 2000) {
		ddrive.setDiffSpeed(0, 0);
		last_updated = now;
	}
}
