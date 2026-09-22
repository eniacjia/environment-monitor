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
#define LIGHTGREEN 0x87F0
#define DKGREY 0x2104

const uint8_t SDA_PIN = 11;
const uint8_t SCL_PIN = 13;
const uint8_t DHT_PIN = A5;
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
int16_t temp8h[HISTORY_SIZE], pressure8h[HISTORY_SIZE];
int16_t temp24h[HISTORY_SIZE], pressure24h[HISTORY_SIZE];
int16_t humidity1h[HISTORY_SIZE], humidity8h[HISTORY_SIZE];
int16_t humidity24h[HISTORY_SIZE];
uint8_t count1h = 0, count8h = 0, count24h = 0;
uint8_t humidityCount1h = 0, humidityCount8h = 0, humidityCount24h = 0;
uint8_t samples1h = 0, samples8h = 0, samples24h = 0;
uint8_t humiditySamples1h = 0, humiditySamples8h = 0, humiditySamples24h = 0;
int32_t tempSum1h = 0, pressureSum1h = 0;
int32_t tempSum8h = 0, pressureSum8h = 0;
int32_t tempSum24h = 0, pressureSum24h = 0;
int32_t humiditySum1h = 0, humiditySum8h = 0, humiditySum24h = 0;
// Display modes: 0=current, 1=one hour, 2=eight hours, 3=24 hours.
uint8_t tempMode = 0, pressureMode = 0, humidityMode = 0;
uint8_t displayRotation = 0;
unsigned long lastReadingMs = 0;
float currentTemp = 0;
float currentPressure = 0;
float currentHumidity = NAN;
bool sensorFound = false;

bool waitForPinState(uint8_t state, unsigned long timeoutUs) {
  unsigned long started = micros();
  while (digitalRead(DHT_PIN) != state) {
    if (micros() - started > timeoutUs) return false;
  }
  return true;
}

bool readDHT11(float &humidity) {
  uint8_t data[5] = {0, 0, 0, 0, 0};
  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delay(20);
  digitalWrite(DHT_PIN, HIGH);
  delayMicroseconds(30);
  pinMode(DHT_PIN, INPUT_PULLUP);

  if (!waitForPinState(LOW, 120) || !waitForPinState(HIGH, 120) ||
      !waitForPinState(LOW, 120)) {
    return false;
  }
  for (uint8_t bit = 0; bit < 40; bit++) {
    if (!waitForPinState(HIGH, 100)) return false;
    unsigned long highStarted = micros();
    if (!waitForPinState(LOW, 120)) return false;
    data[bit / 8] <<= 1;
    if (micros() - highStarted > 40) data[bit / 8] |= 1;
  }

  uint8_t checksum = data[0] + data[1] + data[2] + data[3];
  if (checksum != data[4]) return false;
  humidity = data[0] + data[1] * 0.1F;
  return humidity >= 0.0F && humidity <= 100.0F;
}

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

void pushSingle(int16_t *values, uint8_t &count, int16_t value) {
  if (count < HISTORY_SIZE) values[count++] = value;
  else {
    memmove(values, values + 1, (HISTORY_SIZE - 1) * sizeof(int16_t));
    values[HISTORY_SIZE - 1] = value;
  }
}

void collectHistory(float temperature, float pressure) {
  int16_t t = (int16_t)(temperature * 10.0F);
  int16_t p = (int16_t)(pressure * 10.0F);
  tempSum1h += t; pressureSum1h += p; samples1h++;
  tempSum8h += t; pressureSum8h += p; samples8h++;
  tempSum24h += t; pressureSum24h += p; samples24h++;
  if (samples1h == 6) {
    pushPair(temp1h, pressure1h, count1h,
             tempSum1h / samples1h, pressureSum1h / samples1h);
    tempSum1h = pressureSum1h = 0; samples1h = 0;
  }
  if (samples8h == 48) {
    pushPair(temp8h, pressure8h, count8h,
             tempSum8h / samples8h, pressureSum8h / samples8h);
    tempSum8h = pressureSum8h = 0; samples8h = 0;
  }
  if (samples24h == 144) {
    pushPair(temp24h, pressure24h, count24h,
             tempSum24h / samples24h, pressureSum24h / samples24h);
    tempSum24h = pressureSum24h = 0; samples24h = 0;
  }
}

void collectHumidityHistory(float humidity) {
  int16_t h = (int16_t)(humidity * 10.0F);
  humiditySum1h += h; humiditySamples1h++;
  humiditySum8h += h; humiditySamples8h++;
  humiditySum24h += h; humiditySamples24h++;
  if (humiditySamples1h == 6) {
    pushSingle(humidity1h, humidityCount1h, humiditySum1h / humiditySamples1h);
    humiditySum1h = 0; humiditySamples1h = 0;
  }
  if (humiditySamples8h == 48) {
    pushSingle(humidity8h, humidityCount8h, humiditySum8h / humiditySamples8h);
    humiditySum8h = 0; humiditySamples8h = 0;
  }
  if (humiditySamples24h == 144) {
    pushSingle(humidity24h, humidityCount24h, humiditySum24h / humiditySamples24h);
    humiditySum24h = 0; humiditySamples24h = 0;
  }
}

void drawPanel(uint8_t panel, const __FlashStringHelper *title, float current,
               const __FlashStringHelper *unit, int16_t *history, uint16_t color,
               int16_t minimumSpan, uint8_t count,
               const __FlashStringHelper *rangeLabel) {
  int16_t sectionHeight = tft.height() / 3;
  int16_t top = panel * sectionHeight;
  if (panel == 2) sectionHeight = tft.height() - top;
  int16_t chartTop = top + 30;
  int16_t chartBottom = top + sectionHeight - 7;
  int16_t chartLeft = 8;
  int16_t chartRight = tft.width() - 8;
  tft.fillRect(0, top, tft.width(), sectionHeight, BLACK);
  tft.drawRect(0, top, tft.width(), sectionHeight, WHITE);
  tft.setTextSize(1); tft.setTextColor(WHITE); tft.setCursor(8, top + 11);
  tft.print(title);
  tft.setTextSize(1); tft.setTextColor(color); tft.setCursor(92, top + 11);
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
  tft.fillRect(previousX - 1, previousY - 1, 3, 3, color);

  tft.setTextSize(1); tft.setTextColor(WHITE);
  tft.setCursor(chartLeft + 2, chartTop + 2); tft.print(high / 10.0F, 1);
  tft.setCursor(chartLeft + 2, chartBottom - 9); tft.print(low / 10.0F, 1);
  tft.setCursor(chartRight - 45, chartBottom - 9);
  tft.print(rangeLabel);
}

void drawCurrentPanel(uint8_t panel, const __FlashStringHelper *title,
                      float current, const __FlashStringHelper *unit,
                      uint16_t color, uint8_t textSize) {
  int16_t sectionHeight = tft.height() / 3;
  int16_t top = panel * sectionHeight;
  if (panel == 2) sectionHeight = tft.height() - top;
  tft.fillRect(0, top, tft.width(), sectionHeight, BLACK);
  tft.drawRect(0, top, tft.width(), sectionHeight, WHITE);
  tft.setTextColor(WHITE); tft.setTextSize(2);
  tft.setCursor(8, top + 12); tft.print(title);
  tft.setTextColor(color); tft.setTextSize(2);
  tft.setCursor(tft.width() - (panel == 1 ? 48 : 30), top + 12);
  tft.print(unit);

  if (sectionHeight < 100 && textSize > 4) textSize = 4;
  else if (textSize > 5) textSize = 5;
  tft.setTextSize(textSize);
  bool valid = !isnan(current);
  uint8_t characters = valid ? (current >= 1000.0F ? 6 :
                       (current >= 100.0F ? 5 : (current >= 10.0F ? 4 : 3))) : 2;
  if (valid && current < 0.0F) characters++;
  int16_t valueX = (tft.width() - characters * 6 * textSize) / 2;
  int16_t valueY = top + 36 + (sectionHeight - 36 - 8 * textSize) / 2;
  tft.setTextColor(color);
  tft.setCursor(valueX, valueY);
  if (valid) tft.print(current, 1); else tft.print(F("--"));
  tft.setCursor(valueX + 1, valueY);
  if (valid) tft.print(current, 1); else tft.print(F("--"));
}

void drawDashboard() {
  if (tempMode == 0)
    drawCurrentPanel(0, F("TEMPERATURE"), currentTemp, F("C"), YELLOW, 6);
  else
    drawPanel(0, F("TEMPERATURE"), currentTemp, F("C"),
              tempMode == 1 ? temp1h : (tempMode == 2 ? temp8h : temp24h),
              YELLOW, 20,
              tempMode == 1 ? count1h : (tempMode == 2 ? count8h : count24h),
              tempMode == 1 ? F("1 HOUR") : (tempMode == 2 ? F("8 HOUR") : F("24 HOUR")));

  if (pressureMode == 0)
    drawCurrentPanel(1, F("PRESSURE"), currentPressure, F("hPa"), CYAN, 5);
  else
    drawPanel(1, F("PRESSURE"), currentPressure, F("hPa"),
              pressureMode == 1 ? pressure1h : (pressureMode == 2 ? pressure8h : pressure24h),
              CYAN, 50,
              pressureMode == 1 ? count1h : (pressureMode == 2 ? count8h : count24h),
              pressureMode == 1 ? F("1 HOUR") : (pressureMode == 2 ? F("8 HOUR") : F("24 HOUR")));

  if (humidityMode == 0)
    drawCurrentPanel(2, F("HUMIDITY"), currentHumidity, F("%"), LIGHTGREEN, 5);
  else
    drawPanel(2, F("HUMIDITY"), currentHumidity, F("%"),
              humidityMode == 1 ? humidity1h : (humidityMode == 2 ? humidity8h : humidity24h),
              LIGHTGREEN, 100,
              humidityMode == 1 ? humidityCount1h :
                (humidityMode == 2 ? humidityCount8h : humidityCount24h),
              humidityMode == 1 ? F("1 HOUR") :
                (humidityMode == 2 ? F("8 HOUR") : F("24 HOUR")));

  int16_t centerX = tft.width() / 2;
  int16_t centerY = tft.height() / 2;
  tft.fillRect(centerX - 8, centerY - 8, 17, 17, BLACK);
  tft.drawCircle(centerX, centerY, 7, WHITE);
  tft.drawCircle(centerX, centerY, 4, WHITE);
}

void printBuildNumber() {
  static const char months[] PROGMEM = "JanFebMarAprMayJunJulAugSepOctNovDec";
  uint8_t month = 1;
  for (uint8_t i = 0; i < 12; i++) {
    if (pgm_read_byte(months + i * 3) == __DATE__[0] &&
        pgm_read_byte(months + i * 3 + 1) == __DATE__[1] &&
        pgm_read_byte(months + i * 3 + 2) == __DATE__[2]) {
      month = i + 1;
      break;
    }
  }
  tft.print(F("BUILD# "));
  tft.print(__DATE__ + 7);
  if (month < 10) tft.print('0');
  tft.print(month);
  tft.write(__DATE__[4] == ' ' ? '0' : __DATE__[4]);
  tft.write(__DATE__[5]);
}

void showStartupPage() {
  tft.fillScreen(BLACK);
  tft.drawRect(8, 12, 224, 296, DKGREY);
  tft.setTextColor(YELLOW); tft.setTextSize(2);
  tft.setCursor(42, 45); tft.print(F("ENVIRONMENTAL"));
  tft.setCursor(78, 72); tft.print(F("MONITOR"));
  tft.drawFastHLine(30, 102, 180, CYAN);

  tft.setTextColor(WHITE); tft.setTextSize(2);
  tft.setCursor(42, 122); tft.print(F("Local climate"));
  tft.setCursor(54, 148); tft.print(F("at a glance"));

  tft.setTextColor(CYAN); tft.setTextSize(2);
  tft.setCursor(30, 190); printBuildNumber();

  tft.setTextColor(WHITE); tft.setTextSize(2);
  tft.setCursor(42, 232); tft.print(F("Built by DJIA"));
  tft.setCursor(54, 258); tft.print(F("using Codex"));
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
  if (readDHT11(currentHumidity)) {
    int16_t initialHumidity = currentHumidity * 10.0F;
    pushSingle(humidity1h, humidityCount1h, initialHumidity);
    pushSingle(humidity8h, humidityCount8h, initialHumidity);
    pushSingle(humidity24h, humidityCount24h, initialHumidity);
  }
  pushPair(temp1h, pressure1h, count1h,
           currentTemp * 10.0F, currentPressure * 10.0F);
  pushPair(temp8h, pressure8h, count8h,
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
    } else {
      int16_t sectionHeight = tft.height() / 3;
      if (touchY < sectionHeight) tempMode = (tempMode + 1) % 4;
      else if (touchY < sectionHeight * 2) pressureMode = (pressureMode + 1) % 4;
      else humidityMode = (humidityMode + 1) % 4;
    }
    drawDashboard();
  }
  touchWasDown = touchDown;

  if (!sensorFound) {
    if (millis() - lastReadingMs >= 5000) {
      lastReadingMs = millis();
      sensorFound = beginBMP280();
      if (sensorFound && readBMP280(currentTemp, currentPressure)) {
        if (readDHT11(currentHumidity)) {
          int16_t initialHumidity = currentHumidity * 10.0F;
          pushSingle(humidity1h, humidityCount1h, initialHumidity);
          pushSingle(humidity8h, humidityCount8h, initialHumidity);
          pushSingle(humidity24h, humidityCount24h, initialHumidity);
        }
        pushPair(temp1h, pressure1h, count1h,
                 currentTemp * 10.0F, currentPressure * 10.0F);
        pushPair(temp8h, pressure8h, count8h,
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
    if (readDHT11(currentHumidity)) collectHumidityHistory(currentHumidity);
    collectHistory(currentTemp, currentPressure);
    drawDashboard();
  }
}
