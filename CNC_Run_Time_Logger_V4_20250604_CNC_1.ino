/* CNC Run Time Logger V4_250528
작성일: 250521
수정일: 250528
       250530
       250602_Debug
       250602_Debug_Fixed
       250603_Debug_307_Redirect_Fix
       250603_Compile_Error_Fix (현재 수정)
       250604_ Spread Sheet 에서 Firebase로 전환

기능: 
1. 실시간 전류측정
2. 0.1A 이상 측정되면 작동시작으로 판단하여 동작시간 측정 시작 [if (current > 1.0)] // 이전 1.0A에서 수정됨
3. 0.1A 이상인 상태였다가 0.1A 이하로 내려간 후 30초(shutdownDelay)동안 1A 이상 측정되지않으면 
  처음 0.1A 이상이었던 시점부터 0.1A 이하로 내려간 시점까지 시간을 누적하여 스프레드시트로 전송.
4. 기기 아이디 구분 [const String deviceID   = "CNC_1";]

수정내역
250528 - ADS1115 안정화 딜레이 및 리셋 로직 추가
250530 - 스프레드시트 URL 교체
250602_Debug - 디버깅 로그 (WiFi IP, payload, 응답 결과)
250602_Debug_Fixed - Google Apps Script 302 오류 대응 (script.googleusercontent.com 사용)
250603_Debug_307_Redirect_Fix - Google Apps Script 307 오류 대응 (script.google.com 사용)
250603_Compile_Error_Fix - 문자열 연결 및 HttpClient 메소드 호출 오류 수정
250604_ Firebase 전환. uploadSessionToSheet() 함수 전체 삭제 후 uploadSessionToFirebase() 함수로 대체
250604_ RTC 적용방식 수정. 별도의 보드로 RTC 시간을 싱크로 후 장착함. 이 코드에서는 RTC 시간을 건들지 않도록 수정함.
*/


#include <WiFiS3.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <WiFiSSLClient.h>
#include <ArduinoHttpClient.h>
#include <Adafruit_ADS1X15.h>

#define LCD_ADDR      0x27
#define LCD_COLS      16
#define LCD_ROWS      2
#define BUTTON_PIN    2

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
RTC_DS3231 rtc;
WiFiSSLClient wifi;
Adafruit_ADS1115 ads;

const char* ssid        = "BP Wifi";
const char* password    = "bp1200aa";
const String deviceID   = "CNC_1";

const float BURDEN_RESISTOR   = 33.0f;
const float CT_RATIO          = 2000.0f;
const float LSB_VOLTAGE       = 4.096f / 32768.0f;
const unsigned long SAMPLE_WINDOW = 1000;
const float CALIBRATION       = 146.0f / 65.0f;

int screenIndex           = 0;
int lastScreenIndex       = -1;
bool lastButtonState      = HIGH;

bool spindleRunning       = false;
bool waitingForShutdown   = false;
DateTime spindleStartTime;
DateTime spindleStopTime;
DateTime lastActiveTime;
unsigned long totalRunSeconds     = 0;
const unsigned long shutdownDelay = 600;

bool hasUploadedToday     = false;
bool hasResetToday        = false;

float lastAmps            = 0.0;

String formatDuration(unsigned long seconds) {
  unsigned long h = seconds / 3600;
  seconds %= 3600;
  unsigned long m = seconds / 60;
  unsigned long s = seconds % 60;

  String formattedTime = "";
  if (h < 10) formattedTime += "0";
  formattedTime += String(h) + "h ";
  if (m < 10) formattedTime += "0";
  formattedTime += String(m) + "m ";
  if (s < 10) formattedTime += "0";
  formattedTime += String(s) + "s";
  return formattedTime;
}

float measureCurrentRMS() {
  unsigned long startMillis = millis();
  uint32_t samplesRead = 0;
  double sumCurrentSq = 0.0;

  while (millis() - startMillis < SAMPLE_WINDOW) {
    int16_t rawADC = ads.readADC_Differential_0_1();
    float voltage = rawADC * LSB_VOLTAGE;
    float currentSample = (voltage / BURDEN_RESISTOR) * CT_RATIO;

    sumCurrentSq += (double)currentSample * currentSample;
    samplesRead++;
    delay(1);
  }

  if (samplesRead == 0) return 0.0f;

  float rmsCurrent = sqrt(sumCurrentSq / samplesRead);
  rmsCurrent *= CALIBRATION;

  return rmsCurrent;
}

void uploadSessionToFirebase(DateTime start, DateTime stop, unsigned long duration) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot upload.");
    return;
  }

  String datePath = String(start.year()) + "-";
  datePath += (start.month() < 10 ? "0" : "") + String(start.month()) + "-";
  datePath += (start.day() < 10 ? "0" : "") + String(start.day());

  String firebasePath = "/logs/" + datePath + ".json";

  String startTimeStr = (start.hour() < 10 ? "0" : "") + String(start.hour()) + ":" +
                        (start.minute() < 10 ? "0" : "") + String(start.minute()) + ":" +
                        (start.second() < 10 ? "0" : "") + String(start.second());

  String endTimeStr = (stop.hour() < 10 ? "0" : "") + String(stop.hour()) + ":" +
                      (stop.minute() < 10 ? "0" : "") + String(stop.minute()) + ":" +
                      (stop.second() < 10 ? "0" : "") + String(stop.second());

  String payload = "{";
  payload += "\"device_id\":\"" + deviceID + "\",";
  payload += "\"start_time\":\"" + startTimeStr + "\",";
  payload += "\"end_time\":\"" + endTimeStr + "\",";
  payload += "\"runtime\":\"" + formatDuration(duration) + "\"";
  payload += "}";

  HttpClient firebaseClient = HttpClient(wifi, "cnc-run-time-logger-default-rtdb.asia-southeast1.firebasedatabase.app", 443);
  firebaseClient.beginRequest();
  firebaseClient.post(firebasePath);
  firebaseClient.sendHeader("Content-Type", "application/json");
  firebaseClient.sendHeader("Content-Length", payload.length());
  firebaseClient.beginBody();
  firebaseClient.print(payload);
  firebaseClient.endRequest();

  int statusCode = firebaseClient.responseStatusCode();
  String response = firebaseClient.responseBody();

  Serial.print("Firebase Upload Status: ");
  Serial.println(statusCode);
  Serial.println("Response: " + response);

  if (statusCode >= 200 && statusCode < 300) {
    Serial.println("Firebase Upload Success!");
  } else {
    Serial.println("Firebase Upload Failed!");
  }

  firebaseClient.stop();
}

void handleDailyUploadAndReset() {
  DateTime now = rtc.now();
  if (now.hour() == 23 && now.minute() == 30 && now.second() >= 0 && now.second() <= 2 && !hasUploadedToday) {
    hasUploadedToday = true;
  }

  if (now.hour() == 23 && now.minute() == 50 && now.second() == 0 && !hasResetToday) {
    lcd.clear(); lcd.print("Daily Reset...");
    delay(1000);
    Serial.println("Performing daily system reset.");
    NVIC_SystemReset();
  }

  if (now.hour() == 0 && now.minute() == 0 && now.second() == 0) {
    hasUploadedToday = false;
    hasResetToday = false;
    totalRunSeconds = 0;
  }
}

void handleScreenButton() {
  static unsigned long buttonPressStart = 0;
  bool reading = digitalRead(BUTTON_PIN);

  if (reading == LOW && lastButtonState == HIGH) {
    buttonPressStart = millis();
  }

  if (reading == HIGH && lastButtonState == LOW) {
    unsigned long pressDuration = millis() - buttonPressStart;
    if (pressDuration >= 500) {
      screenIndex = (screenIndex + 1) % 5;
    }
  }
  lastButtonState = reading;
}

void checkSpindleState() {
  DateTime now = rtc.now();
  float current = measureCurrentRMS();
  lastAmps = current;

  Serial.print("Current RMS: ");
  Serial.print(current, 3);
  Serial.println(" A");

  if (current > 1.0) {
    lastActiveTime = now;
    if (!spindleRunning) {
      spindleStartTime = now;
      spindleRunning = true;
      waitingForShutdown = false;
      Serial.println("Spindle STARTED");
    }
  } else {
    if (spindleRunning && !waitingForShutdown) {
      waitingForShutdown = true;
      Serial.println("Spindle STOPPED, waiting for shutdown delay...");
    }

    if (waitingForShutdown && (now.unixtime() - lastActiveTime.unixtime() >= shutdownDelay)) {
      spindleStopTime = lastActiveTime;
      unsigned long duration = spindleStopTime.unixtime() - spindleStartTime.unixtime();
      totalRunSeconds += duration;

      Serial.println("Shutdown delay elapsed. Uploading session.");
      uploadSessionToFirebase(spindleStartTime, spindleStopTime, duration);

      spindleRunning = false;
      waitingForShutdown = false;
    }
  }
}

void updateLCD() {
  if (lastScreenIndex != screenIndex) {
    lcd.clear();
    lastScreenIndex = screenIndex;
  }

  lcd.setCursor(0, 0);
  switch (screenIndex) {
    case 0:
      lcd.print("WiFi: ");
      if (WiFi.status() == WL_CONNECTED) {
        lcd.print("OK ");
        lcd.setCursor(0,1);
        lcd.print(WiFi.localIP());
      } else {
        lcd.print("FAIL");
        lcd.setCursor(0,1);
        lcd.print("                ");
      }
      break;
    case 1:
      lcd.print("Run Time Today:");
      lcd.setCursor(0, 1);
      lcd.print(formatDuration(totalRunSeconds));
      break;
    case 2:
      lcd.print("Device ID:");
      lcd.setCursor(0, 1);
      lcd.print(deviceID);
      break;
    case 3:
      lcd.print("Current: ");
      lcd.print(lastAmps, 2);
      lcd.print(" A");
      lcd.setCursor(0,1);
      lcd.print("                ");
      break;
    case 4:
      {
        DateTime now = rtc.now();
        // 첫 번째 줄: 날짜 (250604 형식)
        lcd.print("Date: ");
        lcd.print(now.year() % 100);  // 연도 끝 2자리
        if (now.month() < 10) lcd.print("0");
        lcd.print(now.month());
        if (now.day() < 10) lcd.print("0");
        lcd.print(now.day());

        // 두 번째 줄: 시간 (HH:MM:SS)
        lcd.setCursor(0, 1);
        lcd.print("Time: ");
        if (now.hour() < 10) lcd.print("0");
        lcd.print(now.hour()); lcd.print(":");
        if (now.minute() < 10) lcd.print("0");
        lcd.print(now.minute()); lcd.print(":");
        if (now.second() < 10) lcd.print("0");
        lcd.print(now.second());
      }
      break;
  }
  delay(100);
}

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  delay(100);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Stabilizing...");
  delay(5000);

  if (!rtc.begin()) {
    lcd.clear(); lcd.print("RTC FAIL"); while (1);
  }
  
/*
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(__DATE__, __TIME__));
    Serial.println("RTC time set from compile time");
  }
*/
  Wire.begin();
  if (!ads.begin(0x48)) {
    Serial.println("ERROR: ADS1115 init failed (@0x48)");
    lcd.clear(); lcd.print("ADC FAIL"); while (1);
  }
  ads.setGain(GAIN_ONE);

  WiFi.begin(ssid, password);
  lcd.clear(); lcd.print("WiFi...");
  int wifiConnectTry = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
    wifiConnectTry++;
    if (wifiConnectTry > 20) {
        lcd.clear(); lcd.print("WiFi FAIL");
        Serial.println("WiFi connection failed after multiple retries.");
        break;
    }
  }
  if(WiFi.status() == WL_CONNECTED){
    lcd.clear(); lcd.print("WiFi OK"); delay(1000); lcd.clear();
    Serial.println("WiFi connected.");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
  } else {
    lcd.clear(); lcd.print("WiFi Connect Err"); delay(1000); lcd.clear();
  }
}

void loop() {


  handleScreenButton();
  checkSpindleState();
  updateLCD();
  handleDailyUploadAndReset();
}
