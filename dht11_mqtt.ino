#include <ESP8266WiFi.h>
#include <ArduinoMqttClient.h>
#include <DHT.h>

#define DHTPIN D1        // Pin digital yang terhubung ke DHT11
#define DHTTYPE DHT11    // Jenis sensor DHT11 
DHT dht(DHTPIN, DHTTYPE);

const int buzzerPin = D2;  // Pin D2 pada ESP8266 untuk buzzer

// Wi-Fi credentials
const char* ssid = "your_wifi";        // Ganti dengan SSID Wi-Fi Anda
const char* password = "your_passsword";  // Ganti dengan password Wi-Fi Anda

// MQTT broker settings
const char* mqtt_server = "10.34.1.150";
const int mqtt_port = 1883;
const char* topic_temp = "[NAMA ANDA]/temperature";  // Topik untuk suhu
const char* topic_hum = "[NAMA ANDA]/humidity";      // Topik untuk kelembapan

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
  Serial.begin(115200);
  
  // Initialize DHT11 sensor
  dht.begin();
  
  // Setup WiFi
  setup_wifi();
  
  // Initialize MQTT
  client.setId("NodeMCUClient");
  client.setUsernamePassword(ssid, password);  

  if (!client.connect(mqtt_server, mqtt_port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(client.connectError());
    while (1);
    }

  // Initialize buzzer
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);  // Mematikan buzzer pada awalnya
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

  // Print values to Serial Monitor
  Serial.print("Suhu: ");
  Serial.print(suhu);
  Serial.print(" °C || Kelembapan: ");
  Serial.println(kelembapan);

  // Publish suhu ke MQTT
  client.beginMessage(topic_temp);
  client.print(suhu);
  client.endMessage();

  // Publish kelembapan ke MQTT
  client.beginMessage(topic_hum);
  client.print(kelembapan);
  client.endMessage();

  // Buzzer logic based on temperature
  if(suhu > 30) {
    digitalWrite(buzzerPin, HIGH);
    Serial.println("Buzzer Menyala");
  } else {
    digitalWrite(buzzerPin, LOW);
    Serial.println("Buzzer Mati");
  }

  delay(5000);  // Delay 5 detik sebelum membaca sensor lagi
}
