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

#define MQ2_PIN A0  // Pin analog yang terhubung ke sensor MQ-2

void setup() {
  Serial.begin(115200);  // Memulai komunikasi serial
}

void loop() {
  int sensorValue = analogRead(MQ2_PIN);  // Membaca nilai analog dari sensor MQ-2
  
  // Mengonversi nilai analog menjadi tegangan (0 - 1023 ke 0 - 3.3V)
  float voltage = sensorValue * (3.3 / 1023.0);

  // Menampilkan nilai sensor dan tegangan pada Serial Monitor
  Serial.print("Nilai Sensor: ");
  Serial.print(sensorValue);
  Serial.print(" || Tegangan: ");
  Serial.print(voltage);
  Serial.println(" V");

  // Perhitungan CO dalam ppm
  float VRL = voltage;  // Tegangan keluaran sensor
  float Rs = ((3.3 * RL) / VRL) - RL;  // Menghitung resistansi sensor Rs
  float ratio = Rs / Ro;  // Menghitung rasio Rs/Ro
  float co = pow(10, (log10(ratio) - bco) / aco);  // Menghitung konsentrasi CO dalam ppm

  // Menampilkan nilai CO dalam ppm
  Serial.print("CO    : ");
  Serial.print(co);
  Serial.println(" ppm");

  delay(2000); // Menunggu 2 detik sebelum pembacaan berikutnya
}
