#include <Arduino.h>
#include <MFRC522.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include "secrets/wifi.h"
#include "secrets/mqtt.h"
#include "wifi_connect.h"
#include <WiFiClientSecure.h>
#include <Ticker.h>
#include "ca_cert.h"

#define SERVO_PIN 18
#define GREEN_LED_PIN 2
#define RED_LED_PIN 15

WiFiClientSecure tlsClient;
PubSubClient mqttClient(tlsClient);
Ticker mqttPulishTicker;
Servo servo;
MFRC522 rfid(5, 4);

unsigned long startTime = 0;
unsigned long endTime = 0;
String current_id = "";
float fee_per_hour = 10000.0;
bool status = false;
float fee;

namespace
{
    const char *ssid = WiFiSecrets::ssid;
    const char *password = WiFiSecrets::pass;
    const char *parkingFee_topic = "parking/fee";
    const char *parkingChangeFee_topic = "parking/changeFee";
    const char *parkingStatus_topic = "parking/status";
    const char *parkingGateControl_topic = "parking/gateControl";
    unsigned int publish_count = 0;
    uint16_t keepAlive = 15;    // seconds (default is 15)
    uint16_t socketTimeout = 5; // seconds (default is 15)
}

void mqttPublish()
{
    Serial.print("Publishing to MQTT...");
    mqttClient.publish(parkingFee_topic, String(fee).c_str(), false);
    mqttClient.publish(parkingStatus_topic, String(status).c_str(), false);
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    String message;
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
        message += (char)payload[i];
    }
    Serial.println();
    message.trim();

    if (String(topic) == parkingGateControl_topic)
    {
      if (message == "open") {
        servo.write(90);
        delay(3000);
        servo.write(0);
      }
    }
    if (String(topic) == parkingChangeFee_topic) {
    fee_per_hour = message.toFloat();
    Serial.printf("Updated Fee per Hour: %.2f VND\n", fee_per_hour);
  }
}

void setup()
{
    Serial.begin(115200);
    delay(10);
    setup_wifi(ssid, password);
    tlsClient.setCACert(ca_cert);

    mqttClient.setCallback(mqttCallback);
    mqttClient.setServer(MQTT::broker, MQTT::port);
    mqttPulishTicker.attach(1, mqttPublish);

    SPI.begin();
    rfid.PCD_Init();

    servo.attach(SERVO_PIN);
    servo.write(0);

    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);

    updateParkingStatus(false);

    Serial.println("System is ready.");
}

void mqttReconnect()
{
    while (!mqttClient.connected())
    {
        Serial.println("Attempting MQTT connection...");
        String client_id = "esp32-client-";
        client_id += String(WiFi.macAddress());
        if (mqttClient.connect(client_id.c_str(), MQTT::username, MQTT::password))
        {
            Serial.print(client_id);
            Serial.println(" connected");
            mqttClient.subscribe("parking/fee");
        }
        else
        {
            Serial.print("MTTT connect failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 1 seconds");
            delay(1000);
        }
    }
}

void loop()
{
    if (!mqttClient.connected())
    {
        mqttReconnect();
    }
    mqttClient.loop();

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String id = getCardID();
      Serial.println("Card ID: " + id);
      if (isValidCard(id)) {
      digitalWrite(GREEN_LED_PIN, HIGH);
      delay(1000);
      digitalWrite(GREEN_LED_PIN, LOW);

      if (!status) {
        startTime = millis();
        servo.write(90); // Mở cửa
        delay(3000);
        servo.write(0);  // Đóng cửa
        status = true;
        updateParkingStatus(true);
      } else {
        endTime = millis();
        float parkingTime = (endTime - startTime) / 3600000.0; // Đổi ra giờ
        fee = parkingTime * fee_per_hour;

        Serial.println("Fee: " + String(fee) + " VND");

        servo.write(90); // Mở cửa
        delay(3000);
        servo.write(0);  // Đóng cửa

        status = false;
        updateParkingStatus(false);
        startTime = 0; // Reset thời gian
      }
    } else {
      digitalWrite(RED_LED_PIN, HIGH);
      delay(1000);
      digitalWrite(RED_LED_PIN, LOW);
    }
    rfid.PICC_HaltA();
  }
}

String getCardID() {
  String cardID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardID += String(rfid.uid.uidByte[i], HEX);
  }
  return cardID;
}

bool isValidCard(String cardID) {
  return (cardID == "abcd1234" || cardID == "efgh5678");
}

void updateParkingStatus(bool occupied) {
  occupied ? "occupied" : "vacant";
}