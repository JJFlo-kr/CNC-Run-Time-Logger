# CNC-Run-Time-Logger


Arduino 기반 CNC 가동 시간 로깅 시스템입니다.  
전류 센서(SCT-013)와 RTC(DS3231)를 이용하여 CNC 스핀들의 작동 여부를 감지하고,  
가동 시간 및 날짜를 Firebase 또는 SD 카드에 자동으로 기록합니다.

---

## ✅ 주요 기능

- 🔌 SCT-013 전류 센서로 실시간 가동 감지
- ⏱ DS3231 RTC 모듈로 시간 정확도 유지
- 💾 SD 카드에 로컬 백업 저장 (예정)
- ☁️ Firebase를 통한 실시간 데이터 업로드
- 📺 LCD로 기기 ID, WiFi 상태, 누적 시간 출력
- 🔁 매일 자동 리셋 및 데이터 전송

---

## 🔧 하드웨어 구성

| 부품 | 설명 |
|------|------|
| Arduino UNO R4 WiFi | 메인 보드 |
| SCT-013-000 | 전류 센서 (33Ω 버든 저항 사용) |
| ADS1115 | 고정밀 ADC |
| DS3231 | RTC 모듈 |
| 16x2 I2C LCD | 실시간 상태 표시 |
| tact switch | LCD 화면 전환 및 메뉴 기능 |
| microSD 모듈 | 로컬 저장 기능 (예정) |

---

## 🧠 프로젝트 구조

CNC-Run-Time-Logger/
├── src/
│ ├── CNC_Run_Time_Logger_V4_250604R1.ino
│ ├── CNC_SD_Logger.ino
│ └── ...
├── docs/
│ └── hardware_diagram.png
├── README.md
└── .gitignore

yaml
복사
편집

---

## 🚀 설치 및 사용 방법

1. Arduino IDE에서 `.ino` 파일 열기
2. 보드: Arduino UNO R4 WiFi 선택
3. 필요한 라이브러리 설치:
   - `Wire.h`
   - `RTClib.h`
   - `LiquidCrystal_I2C.h`
4. WiFi 설정 및 Firebase URL 수정
5. 코드 업로드 후 작동 확인

---

## 🧪 개발 중 기능 (향후 브랜치별 개발)

- [ ] `feature/sd-logging`: SD 카드 로깅
- [ ] `feature/ads-error-check`: ADS1115 오류 감지 알고리즘
- [ ] `feature/watchdog`: 반복 오류 시 자동 재부팅
- [ ] `feature/dust-collector`: CNC 그룹 연동 집진기 제어

---

## 📅 버전 히스토리

| 버전 | 날짜 | 설명 |
|------|------|------|
| V4_250604R1 | 2025-06-04 | 기준 버전. Firebase 업로드 및 RTC 시간 정렬 완료 |
| V5_Pre_SD | (예정) | SD 카드 로깅 기능 추가 예정 |

---

## 📜 라이선스

본 프로젝트는 개인 학습 및 연구 목적이며, 상업적 사용 시 저작자에게 문의 바랍니다.

---

## 🙋‍♂️ 만든이

**JJ IM**  
문의: GitHub Issues 또는 ChatGPT QQ 세션을 통해 소통해주세요.