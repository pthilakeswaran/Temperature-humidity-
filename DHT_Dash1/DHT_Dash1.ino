#include <SPI.h>
#include <TFT_22_ILI9225.h>
#include <DHT.h>
#include <math.h>

#define TFT_RST 26
#define TFT_RS  25
#define TFT_CS  15
#define TFT_LED 0
#define TFT_BRIGHTNESS 200

#define DHT_PIN 18
#define DHT_TYPE DHT11

TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);
DHT dht(DHT_PIN, DHT_TYPE);

float temperature = 0.0;
float humidity = 0.0;
float lastDisplayedTemp = -999.0;
float lastDisplayedHum = -999.0;
unsigned long lastReading = 0;
const unsigned long readingInterval = 1000;
bool displayInitialized = false;

const float TEMP_THRESHOLD = 0.3;
const float HUM_THRESHOLD  = 1.0;

const int centerX = 120;
const int tempY = 65;
const int humY  = 140;

void setup() {
  Serial.begin(9600);
  delay(1000);

  SPIClass hspi(HSPI);
  hspi.begin(14, -1, 13, 15);
  tft.begin(hspi);
  tft.setOrientation(1);
  tft.setFont(Terminal12x16);
  tft.setBacklight(HIGH);
  tft.clear();
  tft.fillRectangle(0, 0, tft.maxX(), tft.maxY(), COLOR_BLACK);

  dht.begin();
  delay(2000);

  setupDisplay();
  readSensorData();
  updateDisplay(true);
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastReading >= readingInterval) {
    readSensorData();

    bool tempChanged = abs(temperature - lastDisplayedTemp) >= TEMP_THRESHOLD;
    bool humChanged = abs(humidity - lastDisplayedHum) >= HUM_THRESHOLD;

    if (tempChanged || humChanged) {
      // Flash notice
      tft.fillRectangle(0, 205, tft.maxX(), 225, COLOR_BLACK);
      String notice = (tempChanged ? (temperature > lastDisplayedTemp ? "Temp Rising!" : "Temp Dropping!") : "Humidity Change!");
      int txtX = (tft.maxX() - notice.length() * 12) / 2;
      tft.drawText(txtX, 210, notice.c_str(), COLOR_WHITE);
      delay(300);
      updateDisplay(true);
      delay(200);
      tft.fillRectangle(0, 205, tft.maxX(), 225, COLOR_BLACK);
    }

    lastReading = millis();
  }

  delay(100);
}

void setupDisplay() {
  tft.fillRectangle(0, 0, tft.maxX(), tft.maxY(), COLOR_BLACK);

  String title = "ENVIRONMENT MONITOR";
  int titleX = (tft.maxX() - title.length() * 10) / 2;
  tft.drawText(titleX, 5, title.c_str(), COLOR_CYAN);

  displayInitialized = true;
}

void readSensorData() {
  float tempSum = 0, humSum = 0;
  int valid = 0;

  for (int i = 0; i < 3; i++) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      tempSum += t;
      humSum += h;
      valid++;
    }
    delay(100);
  }
  if (valid > 0) {
    temperature = tempSum / valid;
    humidity = humSum / valid;
  } else {
    temperature = -999;
    humidity = -999;
  }
}
void updateDisplay(bool forceUpdate) {
  if (!displayInitialized) return;

  bool tempChanged = abs(temperature - lastDisplayedTemp) >= TEMP_THRESHOLD;
  bool humChanged = abs(humidity - lastDisplayedHum) >= HUM_THRESHOLD;

  if (!forceUpdate && !tempChanged && !humChanged) return;

  // Clear the data area (not the top title)
  tft.fillRectangle(0, 30, tft.maxX(), tft.maxY(), COLOR_BLACK);

  drawGauge(centerX, tempY, 45, temperature, 0, 50, "TEMP", COLOR_ORANGE);
  drawGauge(centerX, humY, 45, humidity, 0, 100, "HUMID", COLOR_CYAN);

  lastDisplayedTemp = temperature;
  lastDisplayedHum = humidity;
}
void drawGauge(int x, int y, int radius, float value, float minVal, float maxVal, String label, uint16_t color) {
  // Background arc
  for (int a = -135; a <= 135; a += 3) {
    int arcX = x + cos(a * 3.14159 / 180.0) * radius;
    int arcY = y + sin(a * 3.14159 / 180.0) * radius;
    tft.drawPixel(arcX, arcY, COLOR_DARKGREY);
  }

  // Needle
  float angle = mapFloat(constrain(value, minVal, maxVal), minVal, maxVal, -135, 135);
  float angleRad = angle * 3.14159 / 180.0;
  int x2 = x + cos(angleRad) * (radius - 5);
  int y2 = y + sin(angleRad) * (radius - 5);
  tft.drawLine(x, y, x2, y2, color);
  // Rounded value box
  String valStr = String(value, 1) + (label == "TEMP" ? "C" : "%");
  int boxW = valStr.length() * 12;
  int boxX = x - (boxW / 2);
  tft.fillRectangle(boxX - 4, y - 20, boxX + boxW + 4, y - 4, COLOR_BLACK);
  tft.drawText(boxX, y - 18, valStr.c_str(), COLOR_WHITE);
  // Status
  String status = getStatus(label, value);
  uint16_t statusColor = getStatusColor(status);
  int statX = x - (status.length() * 6);
  tft.drawText(statX, y + 10, status.c_str(), statusColor);
}

String getStatus(String type, float val) {
  if (type == "TEMP") {
    if (val < 20.0) return "LOW";
    else if (val > 30.0) return "HIGH";
    else return "OK";
  } else {
    if (val < 40.0) return "DRY";
    else if (val > 70.0) return "WET";
    else return "OK";
  }
}
uint16_t getStatusColor(String status) {
  if (status == "LOW" || status == "DRY") return COLOR_BLUE;
  else if (status == "HIGH" || status == "WET") return COLOR_RED;
  else return COLOR_GREEN;
}
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
