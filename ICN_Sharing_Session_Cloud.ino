#include <ArduinoWiFiServer.h>
#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiGratuitous.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiServer.h>
#include <WiFiServerSecure.h>
#include <WiFiServerSecureBearSSL.h>
#include <WiFiUdp.h>
#include <ArduinoMqttClient.h>
#include <DHT.h>

#define DHTPIN D1        // Pin digital yang terhubung ke DHT11
#define DHTTYPE DHT11    // Jenis sensor DHT11 
DHT dht(DHTPIN, DHTTYPE);

/*
   Exponential regression:
  a = [log(y2)-log(y1)]/[log(x2)-log(x1)]

  a =  -0.31728 //CO

  b = log(y)-a*log(x)
  
  b =  1.46061 //CO
  
*/

// Konstanta untuk perhitungan CO
#define RL 10              // Load resistor value in kilo ohms
#define aco -0.31728       // Konstanta a untuk CO
#define bco 1.46061        // Konstanta b untuk CO
#define Ro 2.7             // Resistansi referensi untuk udara bersih (dalam kilo ohm)
#define MQ2_PIN A0         // Pin analog yang terhubung ke sensor MQ-2

// Wi-Fi credentials
const char* ssid = "Lab-ICN_v3";        // Ganti dengan SSID Wi-Fi Anda
const char* password = "labjarkomnomorsatu";  // Ganti dengan password Wi-Fi Anda

// MQTT broker settings
const char* mqtt_server = "10.34.1.150";
const int mqtt_port = 1883;
const char* mqtt_username = "icnss";
const char* mqtt_password = "sharingsession";
const char* topic_temp = "icnss/temperature";  // Topik untuk suhu
const char* topic_hum = "icnss/humidity";      // Topik untuk kelembapan
const char* topic_co = "icnss/co";      // Topik untuk karbon monoksida

WiFiClient espClient;
MqttClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
}

void reconnect() {
  // Loop until we're reconnected


  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("NodeMCUClient")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.connectError());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  
  // Initialize DHT11 sensor
  dht.begin();
  
  // Setup WiFi
  setup_wifi();
  
  // Initialize MQTT
  client.setId("NodeMCUClient");
  client.setUsernamePassword(mqtt_username, mqtt_password); 

  if (!client.connect(mqtt_server, mqtt_port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(client.connectError());
    while (1);
    }

}

void loop() {
  // Reconnect to MQTT if connection is lost
  if (!client.connected()) {
    reconnect();
  }
  client.poll();

  // Read sensor data
  float kelembapan = dht.readHumidity();
  float suhu = dht.readTemperature();

  if (isnan(kelembapan) || isnan(suhu)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  int sensorValue = analogRead(MQ2_PIN);  // Membaca nilai analog dari sensor MQ-2
  
  // Mengonversi nilai analog menjadi tegangan (0 - 1023 ke 0 - 3.3V)
  float voltage = sensorValue * (3.3 / 1023.0);
  float VRL = voltage;  // Tegangan keluaran sensor
  float Rs = ((3.3 * RL) / VRL) - RL;  // Menghitung resistansi sensor Rs
  float ratio = Rs / Ro;  // Menghitung rasio Rs/Ro
  float co = pow(10, (log10(ratio) - bco) / aco);  // Menghitung konsentrasi CO dalam ppm

  // Print values to Serial Monitor
  Serial.print("Suhu: ");
  Serial.print(suhu);
  Serial.print(" °C || Kelembapan: ");
  Serial.print(kelembapan);
  Serial.print(" || CO: ");
  Serial.print(co);
  Serial.println(" ppm");

  // Publish suhu ke MQTT
  client.beginMessage(topic_temp);
  client.print(suhu);
  client.endMessage();

  // Publish kelembapan ke MQTT
  client.beginMessage(topic_hum);
  client.print(kelembapan);
  client.endMessage();

  // Publish karbon monoksida ke MQTT
  client.beginMessage(topic_co);
  client.print(co);
  client.endMessage();

  delay(5000);  // Delay 5 detik sebelum membaca sensor lagi
}