#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ====== Konfigurasi WiFi & MQTT ======
const char* ssid = "Realme";   // ganti sesuai WiFi
const char* password = "12345678";
const char* mqtt_server = "test.mosquitto.org"; // broker publik
const int   mqtt_port   = 1883; 
const char* mqtt_topic  = "iot/esp32-waterlevel";

WiFiClient espClient;
PubSubClient client(espClient);

// ====== Konfigurasi Pin ======
#define TRIG_PIN 25
#define ECHO_PIN 34
const int kolamTinggi = 100; // cm

// ====== LCD I2C ======
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ====== Fungsi koneksi WiFi ======
void setup_wifi() {
  Serial.println("Menghubungkan ke WiFi...");
  WiFi.begin(ssid, password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi terhubung");
  } else {
    Serial.println("\nGagal konek WiFi");
  }
}

// ====== Fungsi koneksi MQTT ======
void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Terhubung!");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi...");
      delay(2000);
    }
  }
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.clear();
}

// ====== Loop utama ======
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Baca jarak dari sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms
  float distance = duration * 0.034 / 2; // cm

  // Hitung ketinggian air
  float ketinggian = kolamTinggi - distance;
  if (ketinggian < 0) {
    ketinggian = 0;
  }

  // Tentukan status
  String status;
  if (ketinggian < 70) {
    status = "Aman";
  } else if (ketinggian <= 90) {
    status = "Siaga";
  } else {
    status = "Darurat";
  }

  // Tampilkan di LCD
  lcd.setCursor(0, 0);
  lcd.print("Tinggi:");
  lcd.print(ketinggian);
  lcd.print("cm   "); // tambahkan spasi agar overwrite data lama

  lcd.setCursor(0, 1);
  lcd.print("Status:");
  lcd.print(status);
  lcd.print("   ");
  
  String payload =  String(ketinggian);
  client.publish(mqtt_topic,payload.c_str());

  // Debug serial
  Serial.print("Publish ke ");
  Serial.print(mqtt_topic);
  Serial.print(": ");
  Serial.println(ketinggian);

  delay(2000);
}
