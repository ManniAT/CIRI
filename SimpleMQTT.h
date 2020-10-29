#pragma once
#include <PubSubClient.h>
#define SIMPLE_MQTT_CALLBACK_SIGNATURE std::function<void(const char*, const char*)>
#define TOPIC_BUFFSIZE 100
#define JSON_VALUE_BUFFSIZE 500
#define VALUE_BUFFSIZE 100
class SimpleMQTT {
private:
	char* _ClientID;
	char* _SubscribedTopic;
	char _JSONTopicBuffer[TOPIC_BUFFSIZE + 1];
	char _JSONValueBuffer[JSON_VALUE_BUFFSIZE + 1];
	char _TopicBuffer[TOPIC_BUFFSIZE + 1];
	char _ValueBuffer[VALUE_BUFFSIZE + 1];
	SIMPLE_MQTT_CALLBACK_SIGNATURE _CallBack;
	static SimpleMQTT* _InstancePTR;

	PubSubClient _PSClient;
	static void MQTTCallback(char* topic, byte* payload, unsigned int length) {
		Serial.print("Message arrived [");
		Serial.print(topic);
		Serial.print("] ");
		char* _ReceiveBuffer = new char(length + 1);
		memccpy(_ReceiveBuffer, payload, length, 1);
		_ReceiveBuffer[length + 1] = 0;
		_InstancePTR->OnMessage(topic, _ReceiveBuffer);
	}
	SimpleMQTT(IPAddress pAddress, uint16_t pPort, Client& pClient, SIMPLE_MQTT_CALLBACK_SIGNATURE pCallback) : _PSClient(pAddress, pPort, MQTTCallback, pClient) {
		_CallBack = pCallback;
	}
	void OnMessage(char* pTopic, char* pContent) {
		_CallBack(pTopic, pContent);
	}
public:
	static SimpleMQTT* CreateInstance(IPAddress pAddress, uint16_t pPort, Client& pClient, SIMPLE_MQTT_CALLBACK_SIGNATURE pCallback) {
		if (_InstancePTR == NULL) {
			_InstancePTR = new SimpleMQTT(pAddress, pPort, pClient, pCallback);
		}
		return(_InstancePTR);
	}
	static SimpleMQTT* GetInstance() {
		return(_InstancePTR);
	}
	bool PublishJSON(const char* pTopic, const char* pValue) {
		int nTopicLen = strnlen(pTopic, TOPIC_BUFFSIZE);
		int nValueLen = strnlen(pValue, JSON_VALUE_BUFFSIZE);

		memccpy(_JSONTopicBuffer, pTopic, nTopicLen, 1);
		memccpy(_JSONValueBuffer, pValue, nValueLen, 1);
		_JSONTopicBuffer[nTopicLen] = 0;
		_JSONValueBuffer[nValueLen] = 0;
		if (!IsConnected()) {
			return(false);
		}
		return(_PSClient.publish(_JSONTopicBuffer, _JSONValueBuffer));
	}
	bool Publish(const char* pTopic, const char* pValue) {
		int nTopicLen = strnlen(pTopic, TOPIC_BUFFSIZE);
		int nValueLen = strnlen(pValue, VALUE_BUFFSIZE);

		memccpy(_TopicBuffer, pTopic, nTopicLen, 1);
		memccpy(_ValueBuffer, pValue, nValueLen, 1);
		_TopicBuffer[nTopicLen] = 0;
		_ValueBuffer[nValueLen] = 0;
		if (!IsConnected()) {
			return(false);
		}
		return(_PSClient.publish(_TopicBuffer, _ValueBuffer));
	}

	bool Connect(const char* pClientID, const char* pUser = NULL, const char* pPassword = NULL) {
		_ClientID = new char[strlen(pClientID) + 1];
		strcpy(_ClientID, _ClientID);
		return(_PSClient.connect(pClientID, pUser, pPassword));
	}
	bool Subscribe(const char* pTopic) {
		_SubscribedTopic= new char[strlen(pTopic) + 1];
		strcpy(_SubscribedTopic, pTopic);
		_PSClient.subscribe(_SubscribedTopic);
	}
	bool ReConnect() {
		if (_PSClient.connect(_ClientID)) {
			// Once connected, resubscribe...
			// ... and resubscribe
			_PSClient.subscribe(_SubscribedTopic);
		}
		return IsConnected();
	}
	bool IsConnected() {
		return _PSClient.connected();
	}
	bool Loop() {
		return(_PSClient.loop());
	}
};
SimpleMQTT* SimpleMQTT::_InstancePTR = NULL;