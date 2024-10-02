#include <ESP8266WiFi.h>
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

const int buzzerPin = D2;  // Pin D2 pada ESP8266 untuk buzzer

// Wi-Fi credentials
const char* ssid = "your_wifi";        // Ganti dengan SSID Wi-Fi Anda
const char* password = "your_passsword";  // Ganti dengan password Wi-Fi Anda

// MQTT broker settings
const char* mqtt_server = "10.34.1.150";
const int mqtt_port = 1883;
const char* topic_temp = "[NAMA ANDA]/temperature";  // Topik untuk suhu
const char* topic_hum = "[NAMA ANDA]/humidity";      // Topik untuk kelembapan
const char* topic_co = "[NAMA ANDA]/co";      // Topik untuk karbon monoksida

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
  client.setUsernamePassword(ssid, password);  // Ganti dengan nama Wi-Fi dan password.

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
