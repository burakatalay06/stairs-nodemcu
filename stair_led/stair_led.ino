/*
  | V Pin          | İşlev                          | Widget          |
  | -------------- | ------------------------------ | --------------- |
  | V0–V5          | LED1–LED6 manuel kontrol       | Button (Switch) |
  | V6             | Tüm LED’leri override aç/kapat | Button (Switch) |
  | V7             | Otomatik mod açık/kapalı       | Button (Switch) |
  | V8             | Kısık PWM seviyesi             | Slider (0–1023) |
  | V9             | Tam PWM seviyesi               | Slider (0–1023) |

  | LED  | ESP8266 (NodeMCU) Pin | Not |
  | ---- | --------------------- | --- |
  | LED1 | D1 (GPIO5)            | PWM |
  | LED2 | D2 (GPIO4)            | PWM |
  | LED3 | D5 (GPIO14)           | PWM |
  | LED4 | D6 (GPIO12)           | PWM |
  | LED5 | D7 (GPIO13)           | PWM |
  | LED6 | D8 (GPIO15)           | PWM |
*/

#include "secrets.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ArduinoOTA.h>
#include <time.h>


// LED pinleri (PWM)
const int ledPins[6] = {D1, D2, D5, D6, D7, D8};

bool manualStates[6] = {false, false, false, false, false, false};
bool allLedOverride = false;
bool autoMode = true;
bool fallback_mode = false;

int dimLevel = 100;   // Kısık yanma (varsayılan)
int fullLevel = 300; // Tam yanma (varsayılan)

int autoStartHour = 19; // Otomatik başlama saati
int autoEndHour = 23;   // Otomatik bitiş saati

int currentHour = 0;
float sinusCounter = 0;

void startOTA() {
  ArduinoOTA.setHostname("esp-led-system");
  ArduinoOTA.onStart([]() {
    Serial.println("OTA başladı...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA tamamlandı.");
  });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
    Serial.printf("Yükleniyor: %u%%\r", (p / (t / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA HATA [%u]\n", error);
  });
  ArduinoOTA.begin();
}

void configTimeWithNTP() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("NTP zamanı bekleniyor...");
  int attempts = 0;
  while (time(nullptr) < 100000 && attempts++ < 30) {
    delay(500); Serial.print(".");
  }
  Serial.println();
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
  Serial.print("Saat: "); Serial.println(timeinfo->tm_hour);
  currentHour = timeinfo->tm_hour;
}

void updateTime() {
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
  currentHour = timeinfo->tm_hour;
}

bool isWithinAutoHour(int hour) {
  if (autoStartHour <= autoEndHour)
    return hour >= autoStartHour && hour < autoEndHour;
  else
    return hour >= autoStartHour || hour < autoEndHour;
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 6; i++) {
    pinMode(ledPins[i], OUTPUT);
    analogWrite(ledPins[i], fullLevel);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Wi-Fi bağlanıyor...");

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 60000) {
    delay(500); Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi bağlı: ");
    Serial.println(WiFi.localIP());
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
    configTimeWithNTP();
    startOTA();
  } else {
    Serial.println("\nWi-Fi bağlantısı başarısız. AP mod aktif...");
    fallback_mode = true;
    IPAddress apIP(192, 168, 10, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, subnet);
    WiFi.softAP("ESP_AP", "esp123456");
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
    startOTA();
  }
}

BLYNK_WRITE(V0) {
  manualStates[0] = param.asInt();
}
BLYNK_WRITE(V1) {
  manualStates[1] = param.asInt();
}
BLYNK_WRITE(V2) {
  manualStates[2] = param.asInt();
}
BLYNK_WRITE(V3) {
  manualStates[3] = param.asInt();
}
BLYNK_WRITE(V4) {
  manualStates[4] = param.asInt();
}
BLYNK_WRITE(V5) {
  manualStates[5] = param.asInt();
}
BLYNK_WRITE(V6) {
  allLedOverride = param.asInt();
}
BLYNK_WRITE(V7) {
  autoMode = param.asInt();
}
BLYNK_WRITE(V8) {
  dimLevel = param.asInt();
}
BLYNK_WRITE(V9) {
  fullLevel = param.asInt();
}
BLYNK_WRITE(V10) {
  autoStartHour = param.asInt();
}
BLYNK_WRITE(V11) {
  autoEndHour = param.asInt();
}

void writeLED(int index, int value) {
  analogWrite(ledPins[index], value);
}

void autoLedControl() {
  updateTime();
  bool isAutoActiveNow = isWithinAutoHour(currentHour);

  for (int i = 0; i < 6; i++) {
    if (allLedOverride) {
      writeLED(i, manualStates[5] ? fullLevel : 0);
    } else {
      if (autoMode && isAutoActiveNow) {
        //float angle = sinusCounter + i * 10;
        //float radians = angle * 3.1416 / 180.0;
        //float sinValue = (sin(radians) + 1.0) / 2.0;
        //int pwm = dimLevel + (int)(sinValue * (fullLevel - dimLevel));
        //writeLED(i, pwm);
        writeLED(i, 50);

      } else if(autoMode) {
        writeLED(i, 0);
      }else{
              writeLED(i, dimLevel);

        }


    }
  }

  sinusCounter += 2;
  if (sinusCounter >= 360) sinusCounter = 0;

  Serial.print("Saat: ");
  Serial.print(currentHour);
  Serial.print(" | Mod: ");
  Serial.print(autoMode ? "Auto" : "Manual");
  Serial.print(" | Override: ");
  Serial.println(allLedOverride);
}

void loop() {
  if (fallback_mode) {
    for (int i = 0; i < 6; i++) {
      analogWrite(ledPins[i], fullLevel);
    }
  } else {
    Blynk.run();
  }

  ArduinoOTA.handle();
  autoLedControl();
  delay(100);
}
