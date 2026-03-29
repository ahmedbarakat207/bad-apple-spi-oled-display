
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LittleFS.h>

#define OLED_SCLK   2     // D2  -> OLED D0 (SCLK)
#define OLED_MOSI   4     // D4  -> OLED D1 (MOSI)
#define OLED_DC     17    // TX2 -> OLED DC
#define OLED_RES    16    // RX2 -> OLED RES
#define OLED_CS     5     // D5  -> OLED CS
#define OLED_VCC    15    // D15 -> OLED VCC
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define VIDEO_FPS     10
#define FRAME_DELAY   (1000 / VIDEO_FPS)
#define FRAME_BYTES   (SCREEN_WIDTH * SCREEN_HEIGHT / 8)  // 1024 bytes/frame

SPIClass mySPI(VSPI);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         &mySPI, OLED_DC, OLED_RES, OLED_CS);

uint8_t frameBuf[FRAME_BYTES];
void showError(const char* line1, const char* line2 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 16);
  display.println(line1);
  display.println(line2);
  display.display();
  while (true) delay(1000);
}
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n== ESP32 OLED Video Player ==");
  pinMode(OLED_VCC, OUTPUT);
  digitalWrite(OLED_VCC, HIGH);
  delay(20);
  mySPI.begin(OLED_SCLK, -1, OLED_MOSI, OLED_CS);
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("SSD1306 init failed — check wiring");
    while (true) delay(1000);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 28);
  display.print("Loading...");
  display.display();
  Serial.println("Display OK");
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    showError("LittleFS failed!", "Reflash firmware");
  }
  Serial.println("LittleFS OK");
  if (!LittleFS.exists("/video.bin")) {
    Serial.println("video.bin not found in LittleFS");
    showError("video.bin missing", "Upload via LittleFS");
  }
  File f = LittleFS.open("/video.bin", "r");
  int totalFrames = f.size() / FRAME_BYTES;
  f.close();
  float duration = (float)totalFrames / VIDEO_FPS;
  Serial.printf("Video: %d frames, %.1f sec at %d fps\n",
                totalFrames, duration, VIDEO_FPS);

  display.clearDisplay();
  display.display();
}
void loop() {
  File f = LittleFS.open("/video.bin", "r");
  if (!f) {
    Serial.println("Cannot open video.bin");
    delay(2000);
    return;
  }

  uint32_t frameCount = 0;
  uint32_t startMs    = millis();

  while (f.available() >= FRAME_BYTES) {
    uint32_t t0 = millis();

    f.read(frameBuf, FRAME_BYTES);

    display.clearDisplay();
    memcpy(display.getBuffer(), frameBuf, FRAME_BYTES);
    display.display();

    frameCount++;
    uint32_t elapsed = millis() - t0;
    if (elapsed < FRAME_DELAY) {
      delay(FRAME_DELAY - elapsed);
    }
  }

  f.close();

  float actualFps = (float)frameCount / ((millis() - startMs) / 1000.0f);
  Serial.printf("Loop done: %u frames at %.1f fps actual\n",
                frameCount, actualFps);

  delay(300);
}
