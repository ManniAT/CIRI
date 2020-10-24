#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <PubSubClient.h>
#include <XPT2046_Touchscreen.h> // Touchscreen Treiber
#include "time.h"
#include "./WSElements.h"
//#include "./ModbusHelper.h"
#include "./MyDisplay.h"
#include "./EE895.h"
#include "./FSM.h"
#include "./Beeper.h"
#include "./credentials.h"

const uint8_t RX_PIN = 27;
const uint8_t TX_PIN = 26;
const int HWSERIAL_NUM = 2;

//#define TOUCH_CS 14
//#define TOUCH_IRQ 2
#define	BEEPER_CHANNEL 1
#define BEEPER_PIN 21

MyDisplay TheDisplay;
FSM SMachine(680, 720, 750, 10);

const char* host = "esp32";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

long lastReconnectAttempt = 0;
long lastSend = 0;
long lastModbusRead = 0;
char* lastTopic = "ON";
EE895_Reading* _EE895Data;
EE895* _EE895;	//; = EE895(&ReadModbusData, &WriteModusRegister);
WiFiMulti wifiMulti;
TaskHandle_t Task1;
TaskHandle_t _EE950ReadTask;

IPAddress serverIP(10, 1, 1, 60);
uint16_t portMQTT = 1883;
bool _StartTask = false;
Beeper _Beeper(BEEPER_CHANNEL, BEEPER_PIN);

WiFiClient wClient;

void printLocalTime() {
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo)) {
		Serial.println("Failed to obtain time");
		return;
	}

	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void callback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println();
}

PubSubClient PSClient(serverIP, portMQTT, callback, wClient);
char bufferTemp[10];
char bufferMBar[10];
char bufferCO2[10];
//XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

void HandleNewValues() {
	if (!_EE895->ReadDataValues(_EE895Data)) {	//transfer data to buffer
		return;
	}
	if (_EE895Data->TempC_Changed) {
		sprintf(bufferTemp, "%.1f", _EE895Data->TempC);
		PSClient.publish("EE895/TempC", bufferTemp);
		Serial.printf("Temp: %s C°\n", bufferTemp);
		_Beeper.Beep(Tone::LOW_F, 800);
		TheDisplay.WriteTemp(_EE895Data->TempC, "°C");
	}
	if (_EE895Data->PressureMBar_Changed) {
		sprintf(bufferMBar, "%.1f", _EE895Data->PressureMBar);
		PSClient.publish("EE895/MBar", bufferMBar);
		Serial.printf("MBar: %s\n", bufferMBar);
		_Beeper.Beep(Tone::HIGH_F, 800);
		TheDisplay.WritePressureMBar(_EE895Data->PressureMBar);
	}
	if (_EE895Data->CO2_Changed) {
		sprintf(bufferCO2, "%d", _EE895Data->CO2);
		PSClient.publish("EE895/CO2", bufferCO2);
		Serial.printf("CO2: %s\n", bufferCO2);
		_Beeper.Beep(Tone::DEFAULT_F, 800);
		if (SMachine.SetValue(_EE895Data->CO2)) {
			TheDisplay.WriteCO2(SMachine.CurState, _EE895Data->CO2);
		}
		else {
			TheDisplay.WriteCO2(_EE895Data->CO2);
		}
	}
	SetHPValues(_EE895Data->TempC, _EE895Data->PressureMBar, _EE895Data->CO2);
}
//#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)(((TickType_t)(xTimeInMs) * (TickType_t)configTICK_RATE_HZ) / (TickType_t)1000))

void ReadEE895Task(void* parameter) {
	const TickType_t xBlockTime = pdMS_TO_TICKS(500);
	uint32_t ulNotifiedValue;
	while (1) {
		/* Block to wait for a notification form the ISR.
		The first parameter is set to pdTRUE, clearing the task's
		notification value to 0, meaning each outstanding outstanding deferred
		interrupt event must be processed before ulTaskNotifyTake() is called again.
		In this case only last data ready counts - so don't care if one on more interrupts happened */
		ulNotifiedValue = ulTaskNotifyTake(pdTRUE, xBlockTime);
		if (ulNotifiedValue == 0) {
			/* Did not receive a notification within the expected time. */
			continue;
		}
		Serial.println("InInterrupt");
		if (_EE895->ReadValues()) {	//load data via class method
			Serial.println("Data Read");
			HandleNewValues();
		}
		else {	//inform about problems
			Serial.println("Read Error");
		}
	}
}

void IRAM_ATTR dataReady() {
	BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR(_EE950ReadTask, &xHigherPriorityTaskWoken);
	if (xHigherPriorityTaskWoken == pdTRUE) {
		portYIELD_FROM_ISR();
	}
}

/*
 * setup function
 */

void setup(void) {
	Serial.begin(115200);
	TheDisplay.Init();
	//touch.begin();
	//touch.setRotation(1);

	wifiMulti.addAP(ssidManni24, passwordManni24);
	wifiMulti.addAP(ssidS24, passwordS24);
	wifiMulti.addAP(ssidDevWL, passwordDevWL);
	wifiMulti.addAP(ssidHUA, passwordHUA);

	Serial.println("");
	if (wifiMulti.run() == WL_CONNECTED) {
		Serial.println("");
		Serial.println("WiFiMulti connected");
	}
	else {
		// Connect to WiFi network
		WiFi.begin(ssidManni24, passwordManni24);
		// Wait for connection
		while (WiFi.status() != WL_CONNECTED) {
			delay(500);
			Serial.print(".");
		}

		Serial.println("");
		Serial.print("Connected to ");
		Serial.println(ssidManni24);
	}
	//if we reach this point wifi is connected
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	/*use mdns for host name resolution*/
	if (!MDNS.begin(host)) { //http://esp32.local
		Serial.println("Error setting up MDNS responder!");
		while (1) {
			delay(1000);
		}
	}
	//init and get the time
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	printLocalTime();
	Serial.println(xPortGetCoreID());

	Serial.println("mDNS responder started");
	InitOTAServer();

	//if (PSClient.connect("arduinoClient", "testuser", "testpass"))	{
//		PSClient.publish("outTopic", "ON");
		//PSClient.subscribe("inTopic");
	//}

	_EE895Data = new EE895_Reading();
	_EE895Data->Ignore_CO2Raw = true;
	_EE895Data->Ignore_TempF = true;
	_EE895Data->Ignore_TempK = true;
	_EE895Data->Ignore_PressurePSI = true;

	_EE895 = new EE895(HWSERIAL_NUM, RX_PIN, TX_PIN);

	//InitModBus();
	//sound configuration
	_Beeper.Init();
	_Beeper.IsMuted = true;
	Serial.println("Tone inited");
	//PlayTones();

	xTaskCreatePinnedToCore(
		ReadEE895Task,		 // Function that should be called
		"EE895ReadTask", // Name of the task (for debugging)
		8192,			 // Stack size (bytes)
		NULL,			 // Parameter to pass
		1,				 // Task priority
		&_EE950ReadTask, // Task handle
		1);				 //run on core 1
	Serial.println("Set Interval to 10 Seconds");
	Serial.println(_EE895->SetCO2Interval(10));
	pinMode(25, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(25), dataReady, FALLING);
	TheDisplay.SetDisplayBacklight(Brightness::NORM_1);
}

boolean reconnect() {
	if (PSClient.connect("arduinoClient")) {
		// Once connected, publish an announcement...
		PSClient.publish("outTopic", "ON");
		// ... and resubscribe
		PSClient.subscribe("inTopic");
	}
	return PSClient.connected();
}
void handleTouch() {
	int nErg=TheDisplay.HandleTouch();
	if (!nErg < 0) {
		// button released
	}
	else if (nErg > 0) {
		_Beeper.PlayKeyTone();
		Serial.printf("Key %d pressed\n", nErg);
		if (nErg == BRIGHT_UP_KEY || nErg == BRIGHT_DOWN_KEY) {
			TheDisplay.ChangeDisplayBacklight(nErg == BRIGHT_UP_KEY);
			TheDisplay.DisOrEnableButton(BRIGHT_UP_KEY, !TheDisplay.IsCurBrightnessFull());
			TheDisplay.DisOrEnableButton(BRIGHT_DOWN_KEY, !TheDisplay.IsCurBrightnessOff());
		}
	}
}
int _LastSecond = -1;
struct tm curTimeInfo;
void loop(void) {
	if (wifiMulti.run() != WL_CONNECTED) {
		Serial.println("WiFi not connected!");
		delay(1000);
	}
	else {
		if (PSClient.connected()) {
			long now1 = millis();
			if (now1 - lastSend > 10000) {
				lastSend = now1;
				if (lastTopic == "ON") {
					lastTopic = "OFF";
				}
				else {
					lastTopic = "ON";
				}
				PSClient.publish("outTopic", lastTopic);
			}
			PSClient.loop();
		}
		if (!getLocalTime(&curTimeInfo)) {
			Serial.println("Failed to obtain time in loop");
		}
		else {
			if (_LastSecond != curTimeInfo.tm_sec) {
				_LastSecond = curTimeInfo.tm_sec;
				TheDisplay.WriteTimeAndDate(&curTimeInfo);
			}
		}

		server.handleClient();
		handleTouch();
		delay(200);
	}
}