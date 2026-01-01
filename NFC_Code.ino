#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <SD.h>
#include <Adafruit_PN532.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <RTClib.h>

/* ================= CONFIG ================= */
const char* ssid = "Iqoo_neo_10";
const char* password = "esp32222";
String googleURL =
"https://script.google.com/macros/s/AKfycbyFsdpEfl4hqpepQZTWno8o2aLAOQwtoYYzmjAtvqp5e95fuJ1OtC3msS_FoeUYf8-V/exec";

/* ===== PIN DEFINITIONS ===== */
// PN532 (VSPI)
#define PN532_SS   5

// SD card (HSPI)
#define SD_CS      25
#define SD_SCK     14
#define SD_MISO    12
#define SD_MOSI    13

// I2C
#define I2C_SDA    21
#define I2C_SCL    22

// IO
#define BUZZER     15
#define LED_GREEN  26
#define LED_RED    27

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
/* ========================================= */

// SPI buses
SPIClass hspi(HSPI);

// Modules
Adafruit_PN532 nfc(PN532_SS, &SPI);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_DS3231 rtc;
AsyncWebServer server(80);

/* ================= USERS ================= */
struct User {
  String uid;
  String name;
};

User users[50];
int userCount = 0;

bool registerMode = false;
String pendingName = "";

/* ================= TIME ================= */
String hh = "--", mm = "--";
unsigned long lastClockUpdate = 0;

/* ================= UTIL ================= */
void beep(uint16_t ms = 80) {
  digitalWrite(BUZZER, HIGH);
  delay(ms);
  digitalWrite(BUZZER, LOW);
}

String urlEncode(const String &s) {
  String out = "";
  char buf[4];
  for (char c : s) {
    if (isalnum(c)) out += c;
    else if (c == ' ') out += "%20";
    else {
      sprintf(buf, "%%%02X", (uint8_t)c);
      out += buf;
    }
  }
  return out;
}

/* ================= DISPLAY ================= */
void centerText(const String &t, int y, int size = 1) {
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextSize(size);
  display.getTextBounds(t, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, y);
  display.print(t);
}

void showScreen(const String &l1, const String &l2 = "") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  centerText(l1, 20, 1);
  if (l2 != "") centerText(l2, 38, 1);
  display.display();
}

void progressBar() {
  for (int i = 0; i <= 100; i += 5) {
    display.clearDisplay();
    display.drawRect(10, 40, 108, 10, SSD1306_WHITE);
    display.fillRect(12, 42, i, 6, SSD1306_WHITE);
    display.display();
    delay(35);
  }
}

void showClock() {
  if (millis() - lastClockUpdate >= 1000) {
    DateTime now = rtc.now();
    char b1[3], b2[3];
    sprintf(b1, "%02d", now.hour());
    sprintf(b2, "%02d", now.minute());
    hh = b1;
    mm = b2;
    lastClockUpdate = millis();
  }

  display.clearDisplay();
  display.setTextSize(3);
  centerText(hh + ":" + mm, 12, 3);
  centerText("SCAN CARD", 52, 1);
  display.display();
}

/* ================= STORAGE ================= */
int findUser(const String &uid) {
  for (int i = 0; i < userCount; i++)
    if (users[i].uid == uid) return i;
  return -1;
}

void saveUsers() {
  File f = SPIFFS.open("/users.txt", "w");
  for (int i = 0; i < userCount; i++) {
    f.println(users[i].uid + "," + users[i].name);
  }
  f.close();
}

void loadUsers() {
  if (!SPIFFS.exists("/users.txt")) return;
  File f = SPIFFS.open("/users.txt", "r");
  while (f.available()) {
    String l = f.readStringUntil('\n');
    int c = l.indexOf(',');
    if (c > 0 && userCount < 50)
      users[userCount++] = { l.substring(0, c), l.substring(c + 1) };
  }
  f.close();
}

/* ================= SD ================= */
void logToSD(const String &uid, const String &name) {
  File f = SD.open("/attendance.csv", FILE_APPEND);
  if (!f) return;

  DateTime n = rtc.now();
  f.printf("%04d-%02d-%02d,%02d:%02d:%02d,%s,%s\n",
           n.year(), n.month(), n.day(),
           n.hour(), n.minute(), n.second(),
           uid.c_str(), name.c_str());
  f.close();
}

/* ================= NFC ================= */
String readUID() {
  uint8_t uid[7], len;
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &len)) return "";
  String s = "";
  for (int i = 0; i < len; i++) {
    if (uid[i] < 0x10) s += "0";
    s += String(uid[i], HEX);
  }
  s.toUpperCase();
  return s;
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  Wire.begin(I2C_SDA, I2C_SCL);

  // SPI buses
  SPI.begin();                               // VSPI → PN532
  hspi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS); // HSPI → SD

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  showScreen("ATTENDANCE", "SYSTEM");
  delay(800);
  showScreen("BOOTING...");
  delay(800);

  SPIFFS.begin(true);
  loadUsers();

  rtc.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    showScreen("CONNECTING", "WIFI...");
    delay(300);
  }

  configTime(6 * 3600, 0, "pool.ntp.org");
  struct tm t;
  if (getLocalTime(&t)) {
    rtc.adjust(DateTime(
      t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
      t.tm_hour, t.tm_min, t.tm_sec
    ));
  }

  showScreen("WIFI CONNECTED", WiFi.localIP().toString());
  delay(2000);

  // PN532
  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 NOT FOUND");
    while (1);
  }
  nfc.SAMConfig();

  // SD (retry logic)
  bool sdOK = false;
  delay(500);
  for (int i = 0; i < 3; i++) {
    if (SD.begin(SD_CS, hspi)) {
      sdOK = true;
      break;
    }
    delay(300);
  }
  Serial.println(sdOK ? "SD OK" : "SD INIT FAILED");

  /* ===== ADMIN WEB SERVER ===== */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
    req->send(200, "text/html",
      "<h2>ESP32 Attendance Admin</h2>"
      "<form action='/add'>Name: <input name='name'>"
      "<input type='submit' value='Add User'></form><br>"
      "<form action='/remove'>UID: <input name='uid'>"
      "<input type='submit' value='Remove User'></form>"
    );
  });

  server.on("/add", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("name")) {
      req->send(400, "text/plain", "Missing name");
      return;
    }
    pendingName = req->getParam("name")->value();
    registerMode = true;
    showScreen("REGISTER USER", "SCAN CARD");
    req->send(200, "text/plain", "Scan card now");
  });

  server.on("/remove", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (!req->hasParam("uid")) {
      req->send(400, "text/plain", "Missing UID");
      return;
    }
    String uid = req->getParam("uid")->value();
    uid.toUpperCase();
    int idx = findUser(uid);
    if (idx >= 0) {
      users[idx] = users[--userCount];
      saveUsers();
      req->send(200, "text/plain", "User removed");
    } else {
      req->send(404, "text/plain", "UID not found");
    }
  });

  server.begin();

  showScreen("SYSTEM READY");
  delay(1000);
}

/* ================= LOOP ================= */
void loop() {
  showClock();

  String uid = readUID();
  if (uid == "") return;

  Serial.print("RFID UID: ");
  Serial.println(uid);

  if (registerMode) {
    if (findUser(uid) == -1 && userCount < 50) {
      users[userCount++] = { uid, pendingName };
      saveUsers();
      beep();
      showScreen("USER ADDED", pendingName);
      delay(2000);
    } else {
      showScreen("ALREADY EXISTS");
      delay(2000);
    }
    registerMode = false;
    return;
  }

  int idx = findUser(uid);
  if (idx == -1) {
    digitalWrite(LED_RED, HIGH);
    beep(40);
    showScreen("INVALID USER", "TRY AGAIN");
    delay(2000);
    digitalWrite(LED_RED, LOW);
    return;
  }

  digitalWrite(LED_GREEN, HIGH);
  beep();
  showScreen("WELCOME", users[idx].name);
  delay(1200);

  progressBar();
  logToSD(uid, users[idx].name);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(googleURL +
               "?uid=" + uid +
               "&name=" + urlEncode(users[idx].name));
    http.GET();
    http.end();
  }

  showScreen("SYNC COMPLETE");
  delay(1500);
  digitalWrite(LED_GREEN, LOW);
}
