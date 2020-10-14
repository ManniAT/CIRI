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
//#include "./MyDisplay.h"
#include "./EE895.h"
#include "./FSM.h"
#include "./Beeper.h"
#include "./credentials.h"

int freq = 2000;
int channel = 1;
int resolution = 8;

int ToneLow = 1555;
int ToneMediume = 2222;
int ToneDefault = 4444;
int ToneHigh = 7777;
int IsMuted = true;

const uint8_t RX_PIN = 27;
const uint8_t TX_PIN = 26;
const int HWSERIAL_NUM = 2;


#define TOUCH_CS 14
#define TOUCH_IRQ 2


//MyDisplay TheDisplay;
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

WiFiClient wClient;

void PlayTone(int pTone, int pDuration);
void printLocalTime()
{
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo))
	{
		Serial.println("Failed to obtain time");
		return;
	}
	
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void callback(char* topic, byte* payload, unsigned int length)
{
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++)
	{
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
	if (_EE895Data->TempC_Changed)
	{
		sprintf(bufferTemp, "%.1f", _EE895Data->TempC);
		PSClient.publish("EE895/TempC", bufferTemp);
		Serial.printf("Temp: %s C°\n", bufferTemp);
		PlayTone(ToneLow, 800);
		//TheDisplay.WriteTemp(_EE895Data->TempC,"°C");

	}
	if (_EE895Data->PressureMBar_Changed)
	{
		sprintf(bufferMBar, "%.1f", _EE895Data->PressureMBar);
		PSClient.publish("EE895/MBar", bufferMBar);
		Serial.printf("MBar: %s\n", bufferMBar);
		PlayTone(ToneHigh, 800);
		//TheDisplay.WritePressureMBar(_EE895Data->PressureMBar);
	}
	if (_EE895Data->CO2_Changed)
	{
		sprintf(bufferCO2, "%d", _EE895Data->CO2);
		PSClient.publish("EE895/CO2", bufferCO2);
		Serial.printf("CO2: %s\n", bufferCO2);
		PlayTone(ToneDefault, 800);
		if (SMachine.SetValue(_EE895Data->CO2)) {
			//TheDisplay.WriteCO2(SMachine.CurState, _EE895Data->CO2);
		}
		else {
			//TheDisplay.WriteCO2(_EE895Data->CO2);
		}
	}
	//SetHPValues(_EE895Data->TempC, _EE895Data->PressureMBar, _EE895Data->CO2);
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
	return;
	BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR(_EE950ReadTask, &xHigherPriorityTaskWoken);
	if (xHigherPriorityTaskWoken == pdTRUE) {
		portYIELD_FROM_ISR();
	}
}

void PlayTone(int pTone, int pDuration) {
	if (IsMuted) { return; }
	Serial.println("In tone");
	ledcWriteTone(channel, pTone);
	delay(pDuration);
	ledcWriteTone(channel, 0);
}
/*
 * setup function
 */

void setup(void) {
	Serial.begin(115200);
	//TheDisplay.Init();
	//touch.begin();
	//touch.setRotation(1);


	wifiMulti.addAP(ssidManni24, passwordManni24);
	wifiMulti.addAP(ssidS24, passwordS24);
	wifiMulti.addAP(ssidDevWL, passwordDevWL);
	wifiMulti.addAP(ssidHUA, passwordHUA);

	Serial.println("");
	if (wifiMulti.run() == WL_CONNECTED)
	{
		Serial.println("");
		Serial.println("WiFiMulti connected");
	}
	else
	{
		// Connect to WiFi network
		WiFi.begin(ssidManni24, passwordManni24);
		// Wait for connection
		while (WiFi.status() != WL_CONNECTED)
		{
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
	if (!MDNS.begin(host))
	{ //http://esp32.local
		Serial.println("Error setting up MDNS responder!");
		while (1)
		{
			delay(1000);
		}
	}
	//init and get the time
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	printLocalTime();
	Serial.println(xPortGetCoreID());

	Serial.println("mDNS responder started");
	InitOTAServer();
	//StartWebServer();
	//if (PSClient.connect("arduinoClient", "testuser", "testpass"))	{
//		PSClient.publish("outTopic", "ON");
		//PSClient.subscribe("inTopic");
	//}

	_EE895Data = new EE895_Reading();
	_EE895Data->Ignore_CO2Raw = true;
	_EE895Data->Ignore_TempF = true;
	_EE895Data->Ignore_TempK = true;
	_EE895Data->Ignore_PressurePSI=true;

	_EE895 = new EE895(HWSERIAL_NUM, RX_PIN, TX_PIN);

	//InitModBus();
	//sound configuration
	ledcSetup(channel, freq, resolution);
	ledcAttachPin(21, channel);
	ledcWrite(channel, 255);
	ledcWriteTone(channel, 0);
	Serial.println("Tone off");
	//PlayTones();

	//xTaskCreatePinnedToCore(
	//	Task1code, /* Function to implement the task */
	//	"Task1",   /* Name of the task */
	//	10000,	 /* Stack size in words */
	//	NULL,	  /* Task input parameter */
	//	0,		   /* Priority of the task */
	//	&Task1,	/* Task handle. */
	//	0);		   /* Core where the task should run */

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
	//TheDisplay.SetDisplayBacklight(Brightness::LOW_6);
}

boolean reconnect()
{
	if (PSClient.connect("arduinoClient"))
	{
		// Once connected, publish an announcement...
		PSClient.publish("outTopic", "ON");
		// ... and resubscribe
		PSClient.subscribe("inTopic");
	}
	return PSClient.connected();
}
bool wastouched = true;
void handleTouch() {
	return;
	//TODO: add SPI lock
	//bool istouched = touch.touched();
	//if (istouched) {
	//	TS_Point p = touch.getPoint();
	//	Serial.print("x = ");
	//	Serial.print(p.x);
	//	Serial.print(", y = ");
	//	Serial.println(p.y);
	//}
	//else {
	//	if (wastouched) {
	//		Serial.println("End Touch");
	//	}
	//}
	//wastouched = istouched;
}
int _LastSecond=-1;
struct tm curTimeInfo;
void loop(void)
{
	if (wifiMulti.run() != WL_CONNECTED)
	{
		Serial.println("WiFi not connected!");
		delay(1000);
	}
	else
	{
		if (PSClient.connected())
		{
			long now1 = millis();
			if (now1 - lastSend > 10000)
			{
				lastSend = now1;
				if (lastTopic == "ON")
				{
					lastTopic = "OFF";
				}
				else
				{
					lastTopic = "ON";
				}
				PSClient.publish("outTopic", lastTopic);
			}
		PSClient.loop();
		}
		if (!getLocalTime(&curTimeInfo))
		{
			Serial.println("Failed to obtain time in loop");
		}
		else {
			if (_LastSecond != curTimeInfo.tm_sec) {
				_LastSecond = curTimeInfo.tm_sec;
				//TheDisplay.WriteTimeAndDate(&curTimeInfo);
			}
		}

		server.handleClient();
		handleTouch();
		delay(200);
	}
}