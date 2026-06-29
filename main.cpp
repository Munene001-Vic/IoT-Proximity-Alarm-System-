/**
 * Proximity Alarm with Cartoon OLED Display
 * ------------------------------------------
 * Passive buzzer siren + HC-SR04 ultrasonic sensor
 * + SSD1306 128x64 OLED with cartoon face expressions.
 *
 * Wiring:
 *   HC-SR04 TRIG   → Pin 9
 *   HC-SR04 ECHO   → Pin 10
 *   Passive Buzzer → Pin 8 (+ leg), GND (- leg)
 *   OLED SDA       → A4 (Uno)
 *   OLED SCL       → A5 (Uno)
 *   OLED VCC       → 3.3V or 5V
 *   OLED GND       → GND
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─── OLED Configuration ───────────────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ─── Pin Configuration ────────────────────────────────────────────────────────
#define TRIG_PIN    9
#define ECHO_PIN    10
#define BUZZER_PIN  8

// ─── Distance Thresholds (cm) ─────────────────────────────────────────────────
#define DIST_SILENT   100
#define DIST_FAR       50
#define DIST_NEAR      20

// ─── Siren Frequency Ranges (Hz) ─────────────────────────────────────────────
#define FREQ_FAR_LOW    300
#define FREQ_FAR_HIGH   600
#define FREQ_NEAR_LOW   400
#define FREQ_NEAR_HIGH  900
#define FREQ_URGENT    1000

// ─── Zone Enum ────────────────────────────────────────────────────────────────
enum Zone { ZONE_CLEAR, ZONE_FAR, ZONE_NEAR, ZONE_CRITICAL };

// ─── Face Layout Constants ────────────────────────────────────────────────────
#define FACE_X   30    // Face center X
#define FACE_Y   37    // Face center Y
#define FACE_R   20    // Head radius
#define EYE_L_X  23    // Left eye X
#define EYE_R_X  37    // Right eye X
#define EYE_Y    31    // Eye Y
#define MOUTH_Y  42    // Mouth base Y


// ─── Function Prototypes ──────────────────────────────────────────────────────
int   getDistance();
Zone  getZone(int distance);
void  sirenSweep(int freqLow, int freqHigh, int freqStep, int sweepDelay);
void  urgentBeep();
void  updateDisplay(int distance, Zone zone);
void  drawFace(Zone zone);
void  drawInfo(int distance, Zone zone);
void  drawSmile();
void  drawFrown();
void  drawFlatMouth();


// ─── Setup ────────────────────────────────────────────────────────────────────
void setup() {
  pinMode(TRIG_PIN,   OUTPUT);
  pinMode(ECHO_PIN,   INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not found! Check wiring.");
    while (true);
  }

  // Splash screen
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(15, 20);
  display.print("PROXIMITY  ALARM");
  display.setCursor(38, 36);
  display.print("Loading...");
  display.display();
  delay(2000);
}


// ─── Main Loop ────────────────────────────────────────────────────────────────
void loop() {
  int  distance = getDistance();
  Zone zone     = getZone(distance);

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  updateDisplay(distance, zone);

  switch (zone) {
    case ZONE_CLEAR:
      noTone(BUZZER_PIN);
      break;

    case ZONE_FAR: {
      int sweepDelay = map(distance, DIST_FAR, DIST_SILENT, 5, 20);
      sirenSweep(FREQ_FAR_LOW, FREQ_FAR_HIGH, 5, sweepDelay);
      break;
    }

    case ZONE_NEAR: {
      int sweepDelay = map(distance, DIST_NEAR, DIST_FAR, 3, 8);
      sirenSweep(FREQ_NEAR_LOW, FREQ_NEAR_HIGH, 10, sweepDelay);
      break;
    }

    case ZONE_CRITICAL:
      urgentBeep();
      break;
  }
}


// ─── Determine Zone ───────────────────────────────────────────────────────────
Zone getZone(int distance) {
  if (distance > DIST_SILENT) return ZONE_CLEAR;
  if (distance > DIST_FAR)    return ZONE_FAR;
  if (distance > DIST_NEAR)   return ZONE_NEAR;
  return ZONE_CRITICAL;
}


// ─── Measure Distance ─────────────────────────────────────────────────────────
int getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return (int)(duration * 0.034 / 2);
}


// ─── Siren Sweep ──────────────────────────────────────────────────────────────
void sirenSweep(int freqLow, int freqHigh, int freqStep, int sweepDelay) {
  for (int freq = freqLow; freq <= freqHigh; freq += freqStep) {
    tone(BUZZER_PIN, freq);
    delay(sweepDelay);
  }
  for (int freq = freqHigh; freq >= freqLow; freq -= freqStep) {
    tone(BUZZER_PIN, freq);
    delay(sweepDelay);
  }
}


// ─── Urgent Beep ──────────────────────────────────────────────────────────────
void urgentBeep() {
  tone(BUZZER_PIN, FREQ_URGENT);
  delay(100);
  noTone(BUZZER_PIN);
  delay(50);
}


// ─── Update Display ───────────────────────────────────────────────────────────
void updateDisplay(int distance, Zone zone) {
  display.clearDisplay();

  // Inverted header bar
  display.fillRect(0, 0, SCREEN_WIDTH, 9, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(18, 1);
  display.print("** PROXIMITY ALARM **");

  // Reset to white for rest of drawing
  display.setTextColor(SSD1306_WHITE);

  // Vertical divider
  display.drawLine(62, 9, 62, SCREEN_HEIGHT - 1, SSD1306_WHITE);

  drawFace(zone);
  drawInfo(distance, zone);

  display.display();
}


// ─── Draw Cartoon Face ────────────────────────────────────────────────────────
void drawFace(Zone zone) {
  // Head outline
  display.drawCircle(FACE_X, FACE_Y, FACE_R, SSD1306_WHITE);

  switch (zone) {

    // ── CLEAR: Big happy face ─────────────────────────────────────────────────
    case ZONE_CLEAR:
      // Filled eyes with shine dot
      display.fillCircle(EYE_L_X, EYE_Y, 3, SSD1306_WHITE);
      display.fillCircle(EYE_R_X, EYE_Y, 3, SSD1306_WHITE);
      display.drawPixel(EYE_L_X - 1, EYE_Y - 1, SSD1306_BLACK);
      display.drawPixel(EYE_R_X - 1, EYE_Y - 1, SSD1306_BLACK);
      // Raised happy eyebrows
      display.drawLine(19, 26, 26, 24, SSD1306_WHITE);
      display.drawLine(34, 24, 41, 26, SSD1306_WHITE);
      // Big smile
      drawSmile();
      // Rosy cheeks
      display.fillCircle(15, 40, 2, SSD1306_WHITE);
      display.fillCircle(45, 40, 2, SSD1306_WHITE);
      // Little stars
      display.drawPixel(8,  14, SSD1306_WHITE);
      display.drawPixel(9,  13, SSD1306_WHITE);
      display.drawPixel(10, 14, SSD1306_WHITE);
      display.drawPixel(9,  15, SSD1306_WHITE);
      display.drawPixel(52, 14, SSD1306_WHITE);
      display.drawPixel(53, 13, SSD1306_WHITE);
      display.drawPixel(54, 14, SSD1306_WHITE);
      display.drawPixel(53, 15, SSD1306_WHITE);
      break;

    // ── FAR: Curious face ─────────────────────────────────────────────────────
    case ZONE_FAR:
      // Normal eyes with shine
      display.fillCircle(EYE_L_X, EYE_Y, 3, SSD1306_WHITE);
      display.fillCircle(EYE_R_X, EYE_Y, 3, SSD1306_WHITE);
      display.drawPixel(EYE_L_X - 1, EYE_Y - 1, SSD1306_BLACK);
      display.drawPixel(EYE_R_X - 1, EYE_Y - 1, SSD1306_BLACK);
      // One raised brow (curious look)
      display.drawLine(19, 27, 26, 24, SSD1306_WHITE);
      display.drawLine(34, 25, 41, 26, SSD1306_WHITE);
      // Flat neutral mouth
      drawFlatMouth();
      // Floating "?" above face
      display.setTextSize(1);
      display.setCursor(50, 12);
      display.print("?");
      break;

    // ── NEAR: Worried face ────────────────────────────────────────────────────
    case ZONE_NEAR:
      // Squinted eyes (angled lines)
      display.drawLine(20, EYE_Y,     26, EYE_Y - 1, SSD1306_WHITE);
      display.drawLine(20, EYE_Y + 1, 26, EYE_Y,     SSD1306_WHITE);
      display.drawLine(34, EYE_Y - 1, 40, EYE_Y,     SSD1306_WHITE);
      display.drawLine(34, EYE_Y,     40, EYE_Y + 1, SSD1306_WHITE);
      // Inward worried eyebrows (↘ ↙)
      display.drawLine(19, 25, 26, 27, SSD1306_WHITE);
      display.drawLine(34, 27, 41, 25, SSD1306_WHITE);
      // Frown
      drawFrown();
      // Sweat drop (right side of face)
      display.fillCircle(50, 27, 2, SSD1306_WHITE);
      display.drawLine(50, 23, 50, 25, SSD1306_WHITE);
      // Forehead worry lines
      display.drawLine(25, 20, 27, 22, SSD1306_WHITE);
      display.drawLine(30, 19, 32, 21, SSD1306_WHITE);
      break;

    // ── CRITICAL: Scared face ─────────────────────────────────────────────────
    case ZONE_CRITICAL:
      // Wide open eyes (big circles + pupils)
      display.drawCircle(EYE_L_X, EYE_Y, 5, SSD1306_WHITE);
      display.fillCircle(EYE_L_X, EYE_Y + 1, 2, SSD1306_WHITE);
      display.drawCircle(EYE_R_X, EYE_Y, 5, SSD1306_WHITE);
      display.fillCircle(EYE_R_X, EYE_Y + 1, 2, SSD1306_WHITE);
      // Raised alarmed eyebrows
      display.drawLine(17, 22, 26, 20, SSD1306_WHITE);
      display.drawLine(34, 20, 43, 22, SSD1306_WHITE);
      // Screaming O mouth
      display.drawCircle(FACE_X, MOUTH_Y + 1, 5, SSD1306_WHITE);
      // Shock lines radiating outward
      display.drawLine(6,  16, 10, 19, SSD1306_WHITE);
      display.drawLine(5,  24, 9,  24, SSD1306_WHITE);
      display.drawLine(6,  33, 10, 30, SSD1306_WHITE);
      display.drawLine(50, 16, 54, 19, SSD1306_WHITE);
      display.drawLine(51, 24, 55, 24, SSD1306_WHITE);
      display.drawLine(50, 33, 54, 30, SSD1306_WHITE);
      break;
  }
}


// ─── Mouth Shapes ─────────────────────────────────────────────────────────────
void drawSmile() {
  // Parabola curving downward (happy)
  for (int i = -8; i <= 8; i++) {
    int y = MOUTH_Y + (i * i) / 8;
    display.drawPixel(FACE_X + i, y,     SSD1306_WHITE);
    display.drawPixel(FACE_X + i, y + 1, SSD1306_WHITE);
  }
}

void drawFrown() {
  // Parabola curving upward (sad)
  for (int i = -7; i <= 7; i++) {
    int y = MOUTH_Y + 3 - (i * i) / 7;
    display.drawPixel(FACE_X + i, y,     SSD1306_WHITE);
    display.drawPixel(FACE_X + i, y + 1, SSD1306_WHITE);
  }
}

void drawFlatMouth() {
  display.drawLine(FACE_X - 7, MOUTH_Y,     FACE_X + 7, MOUTH_Y,     SSD1306_WHITE);
  display.drawLine(FACE_X - 7, MOUTH_Y + 1, FACE_X + 7, MOUTH_Y + 1, SSD1306_WHITE);
}


// ─── Draw Info Panel (right side) ─────────────────────────────────────────────
void drawInfo(int distance, Zone zone) {
  const char* zoneLabels[] = { "CLEAR", "FAR", "NEAR", "CRITICAL" };
  const char* label = zoneLabels[zone];

  // Large centered distance number (size 3 = 18px per digit)
  display.setTextSize(3);
  int numDigits = (distance >= 100) ? 3 : (distance >= 10) ? 2 : 1;
  int numWidth  = numDigits * 18;
  int xNum      = 64 + (62 - numWidth) / 2;
  display.setCursor(xNum, 13);
  display.print(distance);

  // "cm" unit
  display.setTextSize(1);
  display.setCursor(91, 38);
  display.print("cm");

  // Centered zone badge (rounded rect)
  int labelWidth = strlen(label) * 6;
  int badgeX     = 64 + (62 - labelWidth - 4) / 2;
  display.drawRoundRect(badgeX, 46, labelWidth + 4, 10, 2, SSD1306_WHITE);
  display.setCursor(badgeX + 2, 48);
  display.print(label);

  // Proximity fill bar (fills left→right as object gets closer)
  int barFill = map(constrain(distance, 0, DIST_SILENT), 0, DIST_SILENT, 60, 0);
  display.drawRect(65, 58, 60, 5, SSD1306_WHITE);
  display.fillRect(65, 58, barFill, 5, SSD1306_WHITE);
}