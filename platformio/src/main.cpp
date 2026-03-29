/*
  ESP32 SPI OLED Video Player — LittleFS edition
  Arduino IDE 2.x on macOS

  Display: SSD1306 0.96" 128x64 SPI OLED

  Wiring:
    ESP32 D15 (GPIO15) --> OLED VCC  (or wire to 3.3V pin directly)
    ESP32 GND          --> OLED GND
    ESP32 D2  (GPIO2)  --> OLED D0   (SCLK)
    ESP32 D4  (GPIO4)  --> OLED D1   (MOSI)
    ESP32 RX2 (GPIO16) --> OLED RES  (Reset)
    ESP32 TX2 (GPIO17) --> OLED DC   (Data/Command)
    ESP32 D5  (GPIO5)  --> OLED CS   (Chip Select)

  Libraries required (install via Library Manager):
    - Adafruit SSD1306
    - Adafruit GFX

  Prepare video:
    1. Convert MP4 with FFmpeg:
         ffmpeg -i input.mp4 -vf "scale=128:64,format=gray" -r 10 frames/frame_%04d.png
    2. Pack frames into video.bin:
         python3 pack_frames.py
    3. Place video.bin inside the sketch's data/ folder
    4. In Arduino IDE 2: Sketch -> Upload LittleFS to Pico/ESP8266/ESP32

  Python packing script (pack_frames.py) — run from sketch folder:
    from PIL import Image
    import os, pathlib
    pathlib.Path("data").mkdir(exist_ok=True)
    out = open("data/video.bin", "wb")
    for fname in sorted(os.listdir("frames")):
        if not fname.endswith(".png"): continue
        img = Image.open(f"frames/{fname}").convert("1")
        out.write(img.tobytes())
    out.close()
    print("Done")
*/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LittleFS.h>

// ── Pin definitions ──────────────────────────────────────────────────────────
#define OLED_SCLK   2     // D2  -> OLED D0 (SCLK)
#define OLED_MOSI   4     // D4  -> OLED D1 (MOSI)
#define OLED_DC     17    // TX2 -> OLED DC
#define OLED_RES    16    // RX2 -> OLED RES
#define OLED_CS     5     // D5  -> OLED CS
#define OLED_VCC    15    // D15 -> OLED VCC

// ── Display settings ─────────────────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define VIDEO_FPS     10
#define FRAME_DELAY   (1000 / VIDEO_FPS)
#define FRAME_BYTES   (SCREEN_WIDTH * SCREEN_HEIGHT / 8)  // 1024 bytes/frame

// ── Objects ──────────────────────────────────────────────────────────────────
SPIClass mySPI(VSPI);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         &mySPI, OLED_DC, OLED_RES, OLED_CS);

uint8_t frameBuf[FRAME_BYTES];

// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n== ESP32 OLED Video Player ==");

  // Power OLED via GPIO15
  pinMode(OLED_VCC, OUTPUT);
  digitalWrite(OLED_VCC, HIGH);
  delay(20);

  // Init SPI: SCLK=2, MISO=none, MOSI=4, SS=5
  mySPI.begin(OLED_SCLK, -1, OLED_MOSI, OLED_CS);

  // Init display
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

  // Init LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    showError("LittleFS failed!", "Reflash firmware");
  }
  Serial.println("LittleFS OK");

  // Check video file exists
  if (!LittleFS.exists("/video.bin")) {
    Serial.println("video.bin not found in LittleFS");
    showError("video.bin missing", "Upload via LittleFS");
  }

  // Print video info to Serial
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

    // Pace to target FPS
    uint32_t elapsed = millis() - t0;
    if (elapsed < FRAME_DELAY) {
      delay(FRAME_DELAY - elapsed);
    }
  }

  f.close();

  float actualFps = (float)frameCount / ((millis() - startMs) / 1000.0f);
  Serial.printf("Loop done: %u frames at %.1f fps actual\n",
                frameCount, actualFps);

  delay(300);  // brief pause before replay
}
