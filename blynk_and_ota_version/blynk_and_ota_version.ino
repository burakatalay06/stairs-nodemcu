#include "secrets.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ArduinoOTA.h>

// LED pinleri
const int led1 = D6;
const int led2 = D5;
const int led3 = D4;

bool fallback_mode = false;

void startOTA() {
  ArduinoOTA.setHostname("esp-blynk-led");
  ArduinoOTA.onStart([]() {
    Serial.println("OTA başladı...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA tamamlandı.");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Yükleniyor: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA HATA [%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Yetkilendirme Hatası");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Başlatma Hatası");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Bağlantı Hatası");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Alım Hatası");
    else if (error == OTA_END_ERROR) Serial.println("Bitiş Hatası");
  });
  ArduinoOTA.begin();
}

void setup() {
  Serial.begin(115200);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  digitalWrite(led1, HIGH);
  digitalWrite(led2, HIGH);
  digitalWrite(led3, HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Wi-Fi bağlanıyor...");

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 60000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi bağlı!");
    Serial.print("IP adresi: ");
    Serial.println(WiFi.localIP());

    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
    startOTA(); // normal OTA başlat
  } else {
    Serial.println("\nWi-Fi bağlantısı başarısız. AP mod başlatılıyor...");
    fallback_mode = true;

    IPAddress apIP(192, 168, 10, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, subnet);
    WiFi.softAP("ESP_AP", "esp123456");

    Serial.print("AP IP adresi: ");
    Serial.println(WiFi.softAPIP());

    startOTA(); // fallback OTA başlat
  }
}

// Blynk kontrolleri
BLYNK_WRITE(V0) {
  digitalWrite(led1, param.asInt() == 1 ? LOW : HIGH);
}
BLYNK_WRITE(V1) {
  digitalWrite(led2, param.asInt() == 1 ? LOW : HIGH);
}
BLYNK_WRITE(V2) {
  digitalWrite(led3, param.asInt() == 1 ? LOW : HIGH);
}

void loop() {
  if (!fallback_mode) {
    Blynk.run();
  }
  ArduinoOTA.handle();
}
