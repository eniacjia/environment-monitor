#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

MCUFRIEND_kbv tft;

#define BLACK 0x0000
#define WHITE 0xFFFF
#define CYAN 0x07FF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define DKGREY 0x2104

const uint8_t SDA_PIN = 11;
const uint8_t SCL_PIN = 13;
uint8_t sensorAddress = 0;

const int XP = 8, XM = A2, YP = A3, YM = 9;
const int TS_LEFT = 125, TS_RT = 903, TS_TOP = 129, TS_BOT = 878;
#define MINPRESSURE 200
#define MAXPRESSURE 1000
TouchScreen ts(XP, YP, XM, YM, 300);

uint16_t dig_T1, dig_P1;
int16_t dig_T2, dig_T3, dig_P2, dig_P3, dig_P4;
int16_t dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
int32_t tFine;

const uint8_t HISTORY_SIZE = 60;
int16_t temp1h[HISTORY_SIZE], pressure1h[HISTORY_SIZE];
int16_t temp24h[HISTORY_SIZE], pressure24h[HISTORY_SIZE];
uint8_t count1h = 0, count24h = 0;
uint8_t samples1h = 0, samples24h = 0;
int32_t tempSum1h = 0, pressureSum1h = 0;
int32_t tempSum24h = 0, pressureSum24h = 0;
// Display modes: 0=current value, 1=one-hour chart, 2=24-hour chart.
uint8_t tempMode = 0, pressureMode = 0;
uint8_t displayRotation = 0;
unsigned long lastReadingMs = 0;
float currentTemp = 0;
float currentPressure = 0;
bool sensorFound = false;

void lineLow(uint8_t pin) { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
void lineHigh(uint8_t pin) { pinMode(pin, INPUT_PULLUP); }
void i2cDelay() { delayMicroseconds(5); }

void i2cStart() {
  lineHigh(SDA_PIN); lineHigh(SCL_PIN); i2cDelay();
  lineLow(SDA_PIN); i2cDelay(); lineLow(SCL_PIN);
}

void i2cStop() {
  lineLow(SDA_PIN); lineHigh(SCL_PIN); i2cDelay();
  lineHigh(SDA_PIN); i2cDelay();
}

bool i2cWrite(uint8_t value) {
  for (uint8_t i = 0; i < 8; i++) {
    if (value & 0x80) lineHigh(SDA_PIN); else lineLow(SDA_PIN);
    i2cDelay(); lineHigh(SCL_PIN); i2cDelay(); lineLow(SCL_PIN);
    value <<= 1;
  }
  lineHigh(SDA_PIN); i2cDelay(); lineHigh(SCL_PIN); i2cDelay();
  bool ack = !digitalRead(SDA_PIN);
  lineLow(SCL_PIN);
  return ack;
}

uint8_t i2cRead(bool ack) {
  uint8_t value = 0;
  lineHigh(SDA_PIN);
  for (uint8_t i = 0; i < 8; i++) {
    value <<= 1; lineHigh(SCL_PIN); i2cDelay();
    if (digitalRead(SDA_PIN)) value |= 1;
    lineLow(SCL_PIN); i2cDelay();
  }
  if (ack) lineLow(SDA_PIN); else lineHigh(SDA_PIN);
  lineHigh(SCL_PIN); i2cDelay(); lineLow(SCL_PIN); lineHigh(SDA_PIN);
  return value;
}

bool writeReg(uint8_t reg, uint8_t value) {
  i2cStart();
  bool ok = i2cWrite(sensorAddress << 1) && i2cWrite(reg) && i2cWrite(value);
  i2cStop();
  return ok;
}

bool readRegs(uint8_t reg, uint8_t *data, uint8_t length) {
  i2cStart();
  if (!i2cWrite(sensorAddress << 1) || !i2cWrite(reg)) { i2cStop(); return false; }
  i2cStart();
  if (!i2cWrite((sensorAddress << 1) | 1)) { i2cStop(); return false; }
  for (uint8_t i = 0; i < length; i++) data[i] = i2cRead(i + 1 < length);
  i2cStop();
  return true;
}

uint16_t u16le(const uint8_t *p) { return uint16_t(p[0]) | (uint16_t(p[1]) << 8); }
int16_t s16le(const uint8_t *p) { return (int16_t)u16le(p); }

bool beginBMP280() {
  lineHigh(SDA_PIN); lineHigh(SCL_PIN);
  uint8_t id = 0;
  const uint8_t addresses[] = {0x76, 0x77};
  for (uint8_t i = 0; i < 2; i++) {
    sensorAddress = addresses[i];
    // 0x60 = BME280.  Read only its temperature and pressure channels.
    if (readRegs(0xD0, &id, 1) && id == 0x60) break;
    if (i == 1) return false;
  }
  uint8_t c[24];
  if (!readRegs(0x88, c, 24)) return false;
  dig_T1=u16le(c); dig_T2=s16le(c+2); dig_T3=s16le(c+4);
  dig_P1=u16le(c+6); dig_P2=s16le(c+8); dig_P3=s16le(c+10);
  dig_P4=s16le(c+12); dig_P5=s16le(c+14); dig_P6=s16le(c+16);
  dig_P7=s16le(c+18); dig_P8=s16le(c+20); dig_P9=s16le(c+22);
  writeReg(0xF4, 0x27);
  writeReg(0xF5, 0xA0);
  return true;
}

bool readBMP280(float &temperature, float &pressure) {
  uint8_t d[6];
  if (!readRegs(0xF7, d, 6)) return false;
  int32_t adcP=(int32_t(d[0])<<12)|(int32_t(d[1])<<4)|(d[2]>>4);
  int32_t adcT=(int32_t(d[3])<<12)|(int32_t(d[4])<<4)|(d[5]>>4);
  int32_t v1=((((adcT>>3)-((int32_t)dig_T1<<1)))*dig_T2)>>11;
  int32_t v2=(((((adcT>>4)-(int32_t)dig_T1)*((adcT>>4)-(int32_t)dig_T1))>>12)*dig_T3)>>14;
  tFine=v1+v2;
  temperature=((tFine*5+128)>>8)/100.0F;
  int64_t p1=(int64_t)tFine-128000;
  int64_t p2=p1*p1*dig_P6;
  p2+=(p1*dig_P5)<<17;
  p2+=((int64_t)dig_P4)<<35;
  p1=((p1*p1*dig_P3)>>8)+((p1*dig_P2)<<12);
  p1=(((int64_t)1<<47)+p1)*dig_P1>>33;
  if (p1 == 0) return false;
  int64_t p=1048576-adcP;
  p=(((p<<31)-p2)*3125)/p1;
  p1=(dig_P9*(p>>13)*(p>>13))>>25;
  p2=(dig_P8*p)>>19;
  p=((p+p1+p2)>>8)+((int64_t)dig_P7<<4);
  pressure=(p/256.0F)/100.0F;
  return true;
}

void pushPair(int16_t *temperatures, int16_t *pressures, uint8_t &count,
              int16_t temperature, int16_t pressure) {
  if (count < HISTORY_SIZE) {
    temperatures[count] = temperature;
    pressures[count] = pressure;
    count++;
  }
  else {
    memmove(temperatures, temperatures + 1, (HISTORY_SIZE - 1) * sizeof(int16_t));
    memmove(pressures, pressures + 1, (HISTORY_SIZE - 1) * sizeof(int16_t));
    temperatures[HISTORY_SIZE - 1] = temperature;
    pressures[HISTORY_SIZE - 1] = pressure;
  }
}

void collectHistory(float temperature, float pressure) {
  int16_t t = (int16_t)(temperature * 10.0F);
  int16_t p = (int16_t)(pressure * 10.0F);
  tempSum1h += t; pressureSum1h += p; samples1h++;
  tempSum24h += t; pressureSum24h += p; samples24h++;
  if (samples1h == 6) {
    pushPair(temp1h, pressure1h, count1h,
             tempSum1h / samples1h, pressureSum1h / samples1h);
    tempSum1h = pressureSum1h = 0; samples1h = 0;
  }
  if (samples24h == 144) {
    pushPair(temp24h, pressure24h, count24h,
             tempSum24h / samples24h, pressureSum24h / samples24h);
    tempSum24h = pressureSum24h = 0; samples24h = 0;
  }
}

void drawPanel(uint8_t panel, const __FlashStringHelper *title, float current,
               const __FlashStringHelper *unit, int16_t *history, uint16_t color,
               int16_t minimumSpan, uint8_t count, bool show24h) {
  int16_t halfHeight = tft.height() / 2;
  int16_t top = panel * halfHeight;
  int16_t chartTop = top + 42;
  int16_t chartBottom = top + halfHeight - 9;
  int16_t chartLeft = 8;
  int16_t chartRight = tft.width() - 8;
  tft.fillRect(0, top, tft.width(), halfHeight, BLACK);
  tft.drawRect(0, top, tft.width(), halfHeight, WHITE);
  tft.setTextSize(1); tft.setTextColor(WHITE); tft.setCursor(8, top + 15);
  tft.print(title);
  tft.setTextSize(2); tft.setTextColor(color); tft.setCursor(92, top + 9);
  tft.print(current, 1); tft.print(unit);

  for (uint8_t i = 0; i <= 4; i++) {
    int16_t y = chartTop + (chartBottom - chartTop) * i / 4;
    tft.drawFastHLine(chartLeft, y, chartRight - chartLeft + 1, DKGREY);
  }
  for (uint8_t i = 0; i <= 6; i++) {
    int16_t x = chartLeft + (chartRight - chartLeft) * i / 6;
    tft.drawFastVLine(x, chartTop, chartBottom - chartTop + 1, DKGREY);
  }
  if (count == 0) return;

  int16_t low = history[0], high = history[0];
  for (uint8_t i = 1; i < count; i++) {
    if (history[i] < low) low = history[i];
    if (history[i] > high) high = history[i];
  }
  if (high - low < minimumSpan) {
    int16_t middle = (high + low) / 2;
    low = middle - minimumSpan / 2;
    high = low + minimumSpan;
  } else {
    int16_t pad = (high - low) / 8 + 1;
    low -= pad; high += pad;
  }

  int16_t previousX = chartRight - (count - 1) * (chartRight - chartLeft) / 59;
  int16_t previousY = map(history[0], low, high, chartBottom, chartTop);
  for (uint8_t i = 1; i < count; i++) {
    int16_t x = chartRight - (count - 1 - i) * (chartRight - chartLeft) / 59;
    int16_t y = map(history[i], low, high, chartBottom, chartTop);
    tft.drawLine(previousX, previousY, x, y, color);
    previousX = x; previousY = y;
  }
  tft.fillCircle(previousX, previousY, 2, color);

  tft.setTextSize(1); tft.setTextColor(WHITE);
  tft.setCursor(chartLeft + 2, chartTop + 2); tft.print(high / 10.0F, 1);
  tft.setCursor(chartLeft + 2, chartBottom - 9); tft.print(low / 10.0F, 1);
  tft.setCursor(chartRight - 56, chartBottom - 9);
  tft.print(show24h ? F("last 24h") : F("last 1h"));
}

void drawCurrentPanel(uint8_t panel, const __FlashStringHelper *title,
                      float current, const __FlashStringHelper *unit,
                      uint16_t color, uint8_t textSize) {
  int16_t halfHeight = tft.height() / 2;
  int16_t top = panel * halfHeight;
  tft.fillRect(0, top, tft.width(), halfHeight, BLACK);
  tft.drawRect(0, top, tft.width(), halfHeight, WHITE);
  tft.setTextColor(WHITE); tft.setTextSize(2);
  tft.setCursor(8, top + 12); tft.print(title);
  tft.setTextColor(color); tft.setTextSize(2);
  tft.setCursor(tft.width() - (panel == 0 ? 30 : 48), top + 12);
  tft.print(unit);

  char valueText[12];
  dtostrf(current, 0, 1, valueText);
  int16_t boundsX, boundsY;
  uint16_t boundsW, boundsH;
  tft.setTextSize(textSize);
  tft.getTextBounds(valueText, 0, 0, &boundsX, &boundsY, &boundsW, &boundsH);
  int16_t valueX = (tft.width() - boundsW) / 2;
  int16_t valueY = top + 42 + (halfHeight - 42 - boundsH) / 2;
  tft.setTextColor(color);
  tft.setCursor(valueX, valueY); tft.print(valueText);
  tft.setCursor(valueX + 1, valueY); tft.print(valueText);
}

void drawDashboard() {
  if (tempMode == 0)
    drawCurrentPanel(0, F("TEMPERATURE"), currentTemp, F("C"), YELLOW, 6);
  else
    drawPanel(0, F("TEMPERATURE"), currentTemp, F("C"),
              tempMode == 2 ? temp24h : temp1h, YELLOW, 20,
              tempMode == 2 ? count24h : count1h, tempMode == 2);

  if (pressureMode == 0)
    drawCurrentPanel(1, F("PRESSURE"), currentPressure, F("hPa"), CYAN, 5);
  else
    drawPanel(1, F("PRESSURE"), currentPressure, F("hPa"),
              pressureMode == 2 ? pressure24h : pressure1h, CYAN, 50,
              pressureMode == 2 ? count24h : count1h, pressureMode == 2);

  int16_t centerX = tft.width() / 2;
  int16_t centerY = tft.height() / 2;
  tft.fillCircle(centerX, centerY, 8, BLACK);
  tft.drawCircle(centerX, centerY, 7, WHITE);
  tft.drawCircle(centerX, centerY, 4, WHITE);
}

void showStartupPage() {
  tft.fillScreen(BLACK);
  tft.drawRoundRect(8, 12, 224, 296, 10, DKGREY);
  tft.setTextColor(YELLOW); tft.setTextSize(2);
  tft.setCursor(18, 72); tft.print(F("ENVIRONMENTAL"));
  tft.setCursor(70, 100); tft.print(F("MONITOR"));
  tft.drawFastHLine(30, 135, 180, CYAN);
  tft.setTextColor(WHITE); tft.setTextSize(1);
  tft.setCursor(45, 158); tft.print(F("Local climate at a glance"));
  tft.setTextColor(CYAN);
  tft.setCursor(70, 214); tft.print(F("Build: Sep 11 2026"));
  tft.setTextColor(WHITE);
  tft.setCursor(47, 250); tft.print(F("Built by DJIA using Codex"));
  delay(10000);
}

void showError() {
  tft.fillScreen(BLACK);
  tft.setTextColor(RED); tft.setTextSize(2); tft.setCursor(22, 80);
  tft.print(F("BME280 not found"));
  tft.setTextColor(WHITE); tft.setTextSize(1); tft.setCursor(15, 125);
  tft.print(F("SDA=D11  SCL=D13"));
  tft.setCursor(15, 145); tft.print(F("Check 3.3V and GND"));
}

bool readTouch(int16_t &x, int16_t &y) {
  TSPoint point = ts.getPoint();
  pinMode(YP, OUTPUT); pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH); digitalWrite(XM, HIGH);
  if (point.z <= MINPRESSURE || point.z >= MAXPRESSURE) return false;
  if (displayRotation == 0) {
    x = map(point.x, TS_LEFT, TS_RT, 0, tft.width());
    y = map(point.y, TS_TOP, TS_BOT, 0, tft.height());
  } else if (displayRotation == 1) {
    x = map(point.y, TS_TOP, TS_BOT, 0, tft.width());
    y = map(point.x, TS_RT, TS_LEFT, 0, tft.height());
  } else if (displayRotation == 2) {
    x = map(point.x, TS_RT, TS_LEFT, 0, tft.width());
    y = map(point.y, TS_BOT, TS_TOP, 0, tft.height());
  } else {
    x = map(point.y, TS_BOT, TS_TOP, 0, tft.width());
    y = map(point.x, TS_LEFT, TS_RT, 0, tft.height());
  }
  return true;
}

void setup() {
  uint16_t id = tft.readID();
  if (id == 0xD3D3) id = 0x9341;
  tft.begin(id); tft.setRotation(0); tft.fillScreen(BLACK);
  showStartupPage();
  sensorFound = beginBMP280();
  if (!sensorFound || !readBMP280(currentTemp, currentPressure)) {
    showError();
    return;
  }
  pushPair(temp1h, pressure1h, count1h,
           currentTemp * 10.0F, currentPressure * 10.0F);
  pushPair(temp24h, pressure24h, count24h,
           currentTemp * 10.0F, currentPressure * 10.0F);
  lastReadingMs = millis();
  drawDashboard();
}

void loop() {
  static bool touchWasDown = false;
  int16_t touchX, touchY;
  bool touchDown = readTouch(touchX, touchY);
  if (sensorFound && touchDown && !touchWasDown) {
    int16_t centerX = tft.width() / 2;
    int16_t centerY = tft.height() / 2;
    if (abs(touchX - centerX) <= 16 && abs(touchY - centerY) <= 16) {
      displayRotation = (displayRotation + 1) % 4;
      tft.setRotation(displayRotation);
      tft.fillScreen(BLACK);
    } else if (touchY < centerY) tempMode = (tempMode + 1) % 3;
    else pressureMode = (pressureMode + 1) % 3;
    drawDashboard();
  }
  touchWasDown = touchDown;

  if (!sensorFound) {
    if (millis() - lastReadingMs >= 5000) {
      lastReadingMs = millis();
      sensorFound = beginBMP280();
      if (sensorFound && readBMP280(currentTemp, currentPressure)) {
        pushPair(temp1h, pressure1h, count1h,
                 currentTemp * 10.0F, currentPressure * 10.0F);
        pushPair(temp24h, pressure24h, count24h,
                 currentTemp * 10.0F, currentPressure * 10.0F);
        drawDashboard();
      }
    }
    return;
  }
  if (millis() - lastReadingMs >= 10000UL) {
    lastReadingMs = millis();
    if (!readBMP280(currentTemp, currentPressure)) {
      sensorFound = false; showError(); return;
    }
    collectHistory(currentTemp, currentPressure);
    drawDashboard();
  }
}
