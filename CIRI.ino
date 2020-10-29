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
#include "./SimpleMQTT.h"
#include "./credentials.h"

const uint8_t RX_PIN = 27;
const uint8_t TX_PIN = 26;
const int HWSERIAL_NUM = 2;


#define	BEEPER_CHANNEL 1
#define BEEPER_PIN 21
#define CO2_SECONDS 10
#define MQTT_CLIENT_NAME "EE895C1"
#define MQTT_DATA_TOPIC MQTT_CLIENT_NAME "/Data"
#define MQTT_SWITCH_TOPIC MQTT_CLIENT_NAME "/Switch"
#define MQTT_SETTINGS_TOPIC MQTT_CLIENT_NAME "/Settings"

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
Beeper _Beeper(BEEPER_CHANNEL, BEEPER_PIN);

WiFiClient wClient;

bool _WifiConnected = false;
SimpleMQTT* _MQTT;
char bufferTemp[10];
char bufferMBar[10];
char bufferCO2[10];
char bufferJSON[150];
int _LastSecond = -1;
struct tm curTimeInfo;

/// <summary>
/// MQTTs callback.
/// </summary>
/// <param name="pTopic">The topic.</param>
/// <param name="pData">The data.</param>
void MQTTCallback(char* pTopic, char* pData) {
	Serial.print("Message arrived [");
	Serial.print(pTopic);
	Serial.print("]: ");
	Serial.println(pData);
}


void HandleNewValues() {
	if (!_EE895->ReadDataValues(_EE895Data)) {	//transfer data to buffer
		return;
	}
	_EE895->GetJSON(bufferJSON);
	_MQTT->PublishJSON(MQTT_DATA_TOPIC, bufferJSON);

	if (_EE895Data->TempC_Changed) {
		sprintf(bufferTemp, "%.1f", _EE895Data->TempC);
		Serial.printf("Temp: %s C°\n", bufferTemp);
		_Beeper.Beep(Tone::LOW_F, 800);
		TheDisplay.WriteTemp(_EE895Data->TempC, "°C");
	}
	if (_EE895Data->PressureMBar_Changed) {
		sprintf(bufferMBar, "%.1f", _EE895Data->PressureMBar);
		Serial.printf("MBar: %s\n", bufferMBar);
		_Beeper.Beep(Tone::HIGH_F, 800);
		TheDisplay.WritePressureMBar(_EE895Data->PressureMBar);
	}
	if (_EE895Data->CO2_Changed) {
		sprintf(bufferCO2, "%d", _EE895Data->CO2);
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

void ReadEE895Task(void* parameter) {
	const TickType_t xBlockTime = pdMS_TO_TICKS(500);
	uint32_t ulNotifiedValue;
	while (1) {
		/* Block to wait for a notification form the ISR.
		The first parameter is set to pdTRUE, clearing the task's
		notification value to 0, meaning each outstanding deferred
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

/// <summary>
/// Datas the ready.
/// </summary>
/// <returns>void IRAM_ATTR.</returns>
void IRAM_ATTR dataReady() {
	BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR(_EE950ReadTask, &xHigherPriorityTaskWoken);
	if (xHigherPriorityTaskWoken == pdTRUE) {
		portYIELD_FROM_ISR();
	}
}

void HandleTouch() {
	int nErg = TheDisplay.HandleTouch();
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
		else if (nErg == MUTE_KEY) {
				_Beeper.IsMuted = !_Beeper.IsMuted;
				TheDisplay.SetButtonStatus(MUTE_KEY, _Beeper.IsMuted);
			}
		else if (nErg == SWITCH_KEY) {
			bool bIsOn = !TheDisplay.GetButtonStatus(SWITCH_KEY);
			TheDisplay.SetButtonStatus(SWITCH_KEY, bIsOn);
			_MQTT->PublishJSON(MQTT_SWITCH_TOPIC, bIsOn ? "ON" : "OFF");
		}
	}
}


 /// <summary>
 /// Setups this instance.
 /// </summary>
void setup(void) {
	Serial.begin(115200);
	TheDisplay.Init();

	wifiMulti.addAP(ssidManni24, passwordManni24);
	wifiMulti.addAP(ssidS24, passwordS24);
	wifiMulti.addAP(ssidDevWL, passwordDevWL);
	wifiMulti.addAP(ssidHUA, passwordHUA);

	Serial.println("");
	for (int nX = 0; nX < 10; nX++) {
		if (wifiMulti.run() == WL_CONNECTED) {
			_WifiConnected = true;
			Serial.println("");
			Serial.println("WiFiMulti connected");
			Serial.print("IP address: ");
			Serial.println(WiFi.localIP());
			break;
		}
		delay(500);
	}
	if (_WifiConnected) {
		/*use mdns for host name resolution*/
		if (!MDNS.begin(host)) { //http://esp32.local
			Serial.println("Error setting up MDNS responder!");
		}
		else {
			Serial.println("mDNS responder started");
		}
	}
	else{
		Serial.println("WiFi not connected");
	}

	_Beeper.Init();
	_Beeper.IsMuted = true;
	Serial.println("Tone inited");

	//init and get the time
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	InitOTAServer();
	_MQTT = SimpleMQTT::CreateInstance(serverIP, portMQTT, wClient, MQTTCallback);
	//if (_WifiConnected) {
	_MQTT->Connect(MQTT_CLIENT_NAME);
	_MQTT->Subscribe(MQTT_SETTINGS_TOPIC "/+");	//no matter if connected (we'll connect later and automatically subscribe to this topic)
	//}

	//https://stackoverflow.com/questions/58719387/writing-and-reading-object-into-esp32-flash-memory-arduino
	//http://esp32-server.de/eeprom/
	_EE895Data = new EE895_Reading();
	_EE895Data->Ignore_CO2Raw = true;
	_EE895Data->Ignore_TempF = true;
	_EE895Data->Ignore_TempK = true;
	_EE895Data->Ignore_PressurePSI = true;

	_EE895 = new EE895(HWSERIAL_NUM, RX_PIN, TX_PIN);



	xTaskCreatePinnedToCore(
		ReadEE895Task,		 // Function that should be called
		"EE895ReadTask", // Name of the task (for debugging)
		8192,			 // Stack size (bytes)
		NULL,			 // Parameter to pass
		1,				 // Task priority
		&_EE950ReadTask, // Task handle
		1);				 //run on core 1
	Serial.printf("\nSet Interval to %d Seconds: ",CO2_SECONDS);
	Serial.println(_EE895->SetCO2Interval(CO2_SECONDS));
	pinMode(25, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(25), dataReady, FALLING);
	TheDisplay.SetDisplayBacklight(Brightness::NORM_1);
	Serial.println("Setup done");
}


void loop(void) {
	if (wifiMulti.run() != WL_CONNECTED) {
		_WifiConnected = false;
		Serial.println("WiFi not connected!");
	}
	else {
		_WifiConnected = true;
		MyWebServer.handleClient();
		if (!_MQTT->Loop()) {	//false if not connected
			_MQTT->ReConnect();	//try to reconnect
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
	}
	HandleTouch();
	delay(200);
}
