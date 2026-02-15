#include <WiFi.h>
#include <HTTPClient.h>

#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

hd44780_I2Cexp lcd;

// ---------- WiFi / URL ----------
const char* SSID = "Jahangir";
const char* PASS = "jahangir";
const char* URL  = "http://10.17.114.210:5000/poop";

// ---------- Touch sensor ----------
const int TOUCH_PIN = 13;
const unsigned long DEBOUNCE_MS = 50;
const unsigned long TOUCH_COOLDOWN_MS = 300;

int lastStableState = -1;
int lastReadState = -1;
unsigned long lastChangeMs = 0;

bool touchPressed = false;
unsigned long lastTouchEventMs = 0;

// ---------- LCD paging ----------
const unsigned long PAGE_MS = 3000;
unsigned long nextPageMs = 0;

String pages[80];
int pageCount = 0;
int pageIndex = 0;

// ---------- HTTP refresh ----------
const unsigned long REFRESH_MS = 15000;
unsigned long nextFetchMs = 0;

// ---------- Split by '>' into up to 5 sections ----------
const int MAX_SECTIONS = 5;
String sections[MAX_SECTIONS];
int sectionCount = 0;
int sectionIndex = 0;

// ---------- Helpers ----------
void padTo16(char* line) {
  int n = strlen(line);
  while (n < 16) line[n++] = ' ';
  line[16] = '\0';
}

void showPage(const char* line1, const char* line2) {
  char l1[17], l2[17];
  strncpy(l1, line1, 16); l1[16] = '\0';
  strncpy(l2, line2, 16); l2[16] = '\0';
  padTo16(l1);
  padTo16(l2);
  lcd.setCursor(0, 0); lcd.print(l1);
  lcd.setCursor(0, 1); lcd.print(l2);
}

bool nextToken(const char* s, int& i, char* out, int outSize) {
  while (s[i] == ' ') i++;
  if (s[i] == '\0') return false;

  int j = 0;
  while (s[i] != '\0' && s[i] != ' ' && j < outSize - 1) {
    out[j++] = s[i++];
  }
  out[j] = '\0';
  return true;
}

// Split network message into sections by '>' (max 5)
void splitIntoSections(const String& msg) {
  sectionCount = 0;

  if (msg.length() == 0) {
    sections[0] = "No message received";
    sectionCount = 1;
    return;
  }

  int start = 0;
  while (sectionCount < MAX_SECTIONS) {
    int pos = msg.indexOf('>', start);
    String part;

    if (pos == -1) {
      part = msg.substring(start);
      part.trim();
      if (part.length() > 0) sections[sectionCount++] = part;
      break;
    } else {
      part = msg.substring(start, pos);
      part.trim();
      if (part.length() > 0) sections[sectionCount++] = part;
      start = pos + 1;
    }
  }

  if (sectionCount == 0) {
    sections[0] = msg;
    sectionCount = 1;
  }
}

void buildPagesFromMessage(const String& msg) {
  pageCount = 0;
  pageIndex = 0;

  if (msg.length() == 0) {
    pages[0] = "No message|received";
    pageCount = 1;
    return;
  }

  const char* MSG = msg.c_str();
  int i = 0;

  while (pageCount < 80) {
    char line1[64] = {0};
    char line2[64] = {0};

    while (true) {
      int save = i;
      char word[32];
      if (!nextToken(MSG, i, word, sizeof(word))) break;

      int curLen = strlen(line1);
      int wLen = strlen(word);
      int add = (curLen == 0) ? wLen : (1 + wLen);

      if (curLen + add <= 16) {
        if (curLen != 0) strcat(line1, " ");
        strcat(line1, word);
      } else {
        i = save;
        break;
      }
    }

    while (true) {
      int save = i;
      char word[32];
      if (!nextToken(MSG, i, word, sizeof(word))) break;

      int curLen = strlen(line2);
      int wLen = strlen(word);
      int add = (curLen == 0) ? wLen : (1 + wLen);

      if (curLen + add <= 16) {
        if (curLen != 0) strcat(line2, " ");
        strcat(line2, word);
      } else {
        i = save;
        break;
      }
    }

    if (strlen(line1) == 0 && strlen(line2) == 0) break;

    pages[pageCount] = String(line1) + "|" + String(line2);
    pageCount++;

    int tmp = i;
    while (MSG[tmp] == ' ') tmp++;
    if (MSG[tmp] == '\0') break;
  }
}

void drawCurrentPage() {
  if (pageCount == 0) {
    showPage("No message", "received");
    return;
  }
  int sep = pages[pageIndex].indexOf('|');
  String l1 = pages[pageIndex].substring(0, sep);
  String l2 = pages[pageIndex].substring(sep + 1);
  showPage(l1.c_str(), l2.c_str());
}

// NEW: header only when showHeader=true
void loadCurrentSectionToPages(bool showHeader) {
  if (sectionCount == 0) buildPagesFromMessage("No sections");
  else buildPagesFromMessage(sections[sectionIndex]);

  lcd.clear();

  if (showHeader) {
    char header1[17];
    snprintf(header1, sizeof(header1), "Part %d/%d", sectionIndex + 1, sectionCount);
    showPage(header1, "");
    delay(400);
    lcd.clear();
  }

  drawCurrentPage();
  nextPageMs = millis() + PAGE_MS;
}

String fetchText() {
  HTTPClient http;
  http.setTimeout(15000);
  http.begin(URL);
  http.addHeader("Accept", "text/plain,text/html,*/*");

  int code = http.GET();

  if (code == 405) {
    http.end();
    http.begin(URL);
    http.setTimeout(15000);
    http.addHeader("Accept", "text/plain,text/html,*/*");
    http.addHeader("Content-Type", "text/plain");
    code = http.POST("");
  }

  String body;
  if (code > 0) body = http.getString();
  else body = "ERROR: request failed";

  http.end();

  body.replace("\r", " ");
  body.replace("\n", " ");
  body.replace("\t", " ");

  if (body.length() > 800) body = body.substring(0, 800);
  return body;
}

void updateTouch() {
  int state = digitalRead(TOUCH_PIN);

  if (state != lastReadState) {
    lastReadState = state;
    lastChangeMs = millis();
  }

  if ((millis() - lastChangeMs) > DEBOUNCE_MS && state != lastStableState) {
    lastStableState = state;

    if (lastStableState == HIGH) {
      if (millis() - lastTouchEventMs > TOUCH_COOLDOWN_MS) {
        touchPressed = true;
        lastTouchEventMs = millis();
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(TOUCH_PIN, INPUT_PULLDOWN);
  lastReadState = digitalRead(TOUCH_PIN);
  lastStableState = lastReadState;

  Wire.begin(21, 22);
  lcd.begin(16, 2);
  lcd.backlight();

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);

  while (WiFi.status() != WL_CONNECTED) {
    updateTouch();
    delay(10);
  }

  String netMsg = fetchText();
  splitIntoSections(netMsg);
  loadCurrentSectionToPages(false);

  nextFetchMs = millis() + REFRESH_MS;
}

void loop() {
  updateTouch();

  // Touch = next section
  if (touchPressed) {
    touchPressed = false;
    sectionIndex = (sectionIndex + 1) % sectionCount;
    loadCurrentSectionToPages(true);
  }

  // Auto page switch
  if (millis() >= nextPageMs) {
    nextPageMs = millis() + PAGE_MS;
    if (pageCount > 0) pageIndex = (pageIndex + 1) % pageCount;
    drawCurrentPage();
  }

  // Auto refresh (without resetting section)
  if (millis() >= nextFetchMs) {
    nextFetchMs = millis() + REFRESH_MS;

    String netMsg = fetchText();
    splitIntoSections(netMsg);

    if (sectionIndex >= sectionCount) sectionIndex = 0;

    loadCurrentSectionToPages(false);
  }

  delay(5);
}