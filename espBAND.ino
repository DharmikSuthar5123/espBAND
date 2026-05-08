#include <U8g2lib.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLE2901.h>
#include <BLE2902.h>

// ======================================================
// DEVICE
// ======================================================

#define DEVICE_NAME "espBAND"

// ======================================================
// UUIDS
// ======================================================

#define SERVICE_UUID       "aae42004-acfa-487a-835c-1e9ce04f0c44"
#define SERVICE_TIME_UUID       "3bf4d21b-645f-4c68-9ff2-f42b35ec4a41"

// Android -> ESP32
#define TIME_CHAR_UUID    "ef95cac9-9cf2-4d9a-ace4-4f6d6b66d74b"
#define TRACK_CHAR_UUID    "11111111-1111-1111-1111-111111111111"

// ESP32 -> Android
#define CONTROL_CHAR_UUID  "22222222-2222-2222-2222-222222222222"

// ======================================================
// BUTTON PINS
// ======================================================

#define RIGHT_BTN   27
#define LEFT_BTN    14
#define CONFIRM_BTN 12
#define BACK_BTN    13

// ======================================================
// DEBOUNCE
// ======================================================

#define DEBOUNCE_MS 200

uint32_t lastRightPress   = 0;
uint32_t lastLeftPress    = 0;
uint32_t lastConfirmPress = 0;
uint32_t lastBackPress    = 0;

// ======================================================
// OLED
// ======================================================

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// ======================================================
// STATE MACHINE
// ======================================================

enum AppState {
  STATE_HOME,
  STATE_MENU,
  STATE_MUSIC,
  STATE_SETTINGS,
  STATE_INACTIVITY,
  STATE_TIMER,
  STATE_WEATHER
};

AppState currentState = STATE_HOME;

// ======================================================
// MENU SYSTEM
// ======================================================

// --- Music icon (32x32 XBM) ---
// A music note icon
static const uint8_t icon_music[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xF0, 0x1F,
  0x00, 0x00, 0xFC, 0x1F,
  0x00, 0x00, 0xFF, 0x1F,
  0x00, 0x80, 0xFF, 0x1F,
  0x00, 0x80, 0x0F, 0x18,
  0x00, 0x80, 0x07, 0x18,
  0x00, 0x80, 0x03, 0x18,
  0x00, 0x80, 0x03, 0x18,
  0x00, 0x80, 0x03, 0x18,
  0x00, 0x80, 0x03, 0x18,
  0x00, 0x80, 0x03, 0x18,
  0x00, 0x80, 0x03, 0x18,
  0x00, 0x80, 0x03, 0x18,
  0x00, 0x80, 0x03, 0x18,
  0x00, 0x80, 0x03, 0x18,
  0x00, 0x80, 0x03, 0x18,
  0x00, 0x80, 0x03, 0x18,
  0x00, 0x80, 0x03, 0x18,
  0x00, 0x80, 0x03, 0x00,
  0x00, 0x80, 0x03, 0x00,
  0x00, 0x80, 0x03, 0x00,
  0x00, 0xC0, 0x03, 0x00,
  0x00, 0xF0, 0x03, 0x00,
  0x00, 0xFC, 0x03, 0x00,
  0x00, 0xFE, 0x01, 0x00,
  0x00, 0xFF, 0x01, 0x00,
  0x80, 0xFF, 0x00, 0x00,
  0x80, 0x7F, 0x00, 0x00,
  0x80, 0x3F, 0x00, 0x00,
  0x00, 0x1F, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// --- Settings icon (32x32 XBM) ---
// A gear / cog icon
static const uint8_t icon_settings[] PROGMEM = {
  0x00, 0x80, 0x01, 0x00,
  0x00, 0x80, 0x01, 0x00,
  0x00, 0xC0, 0x03, 0x00,
  0x10, 0xE0, 0x07, 0x08,
  0x38, 0xF0, 0x0F, 0x1C,
  0x7C, 0xF0, 0x0F, 0x3E,
  0xFC, 0xF0, 0x0F, 0x3F,
  0xF8, 0x01, 0x80, 0x1F,
  0xF0, 0x01, 0x80, 0x0F,
  0xE0, 0x07, 0xE0, 0x07,
  0xE0, 0x0F, 0xF0, 0x07,
  0xE0, 0x1F, 0xF8, 0x07,
  0xC0, 0x1F, 0xF8, 0x03,
  0xC0, 0x3F, 0xFC, 0x03,
  0xC0, 0x3F, 0xFC, 0x03,
  0xC0, 0x3F, 0xFC, 0x03,
  0xC0, 0x3F, 0xFC, 0x03,
  0xC0, 0x3F, 0xFC, 0x03,
  0xC0, 0x3F, 0xFC, 0x03,
  0xE0, 0x1F, 0xF8, 0x07,
  0xE0, 0x0F, 0xF0, 0x07,
  0xE0, 0x07, 0xE0, 0x07,
  0xF0, 0x01, 0x80, 0x0F,
  0xF8, 0x01, 0x80, 0x1F,
  0xFC, 0xF0, 0x0F, 0x3F,
  0x7C, 0xF0, 0x0F, 0x3E,
  0x38, 0xF0, 0x0F, 0x1C,
  0x10, 0xE0, 0x07, 0x08,
  0x00, 0xC0, 0x03, 0x00,
  0x00, 0x80, 0x01, 0x00,
  0x00, 0x80, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// --- Timer icon (32x32 XBM) ---
// A stopwatch / clock icon
static const uint8_t icon_timer[] PROGMEM = {
  0x00, 0xF0, 0x0F, 0x00,
  0x00, 0xF0, 0x0F, 0x00,
  0x00, 0x80, 0x01, 0x00,
  0x00, 0xFC, 0x3F, 0x00,
  0x00, 0xFF, 0xFF, 0x00,
  0x80, 0xFF, 0xFF, 0x01,
  0xC0, 0x07, 0xE0, 0x03,
  0xE0, 0x03, 0xC0, 0x07,
  0xF0, 0x01, 0x80, 0x0F,
  0xF0, 0x00, 0x80, 0x0F,
  0x78, 0x00, 0x80, 0x1F,
  0x78, 0x00, 0x80, 0x1E,
  0x3C, 0x00, 0x80, 0x3C,
  0x3C, 0x00, 0x80, 0x3C,
  0x3C, 0x00, 0xC0, 0x3C,
  0x3C, 0x00, 0xE0, 0x3C,
  0x3C, 0x00, 0xF0, 0x3C,
  0x3C, 0x00, 0x78, 0x3C,
  0x3C, 0x00, 0x3C, 0x3C,
  0x78, 0x00, 0x00, 0x1E,
  0x78, 0x00, 0x00, 0x1E,
  0xF0, 0x00, 0x00, 0x0F,
  0xF0, 0x01, 0x80, 0x0F,
  0xE0, 0x03, 0xC0, 0x07,
  0xC0, 0x07, 0xE0, 0x03,
  0x80, 0xFF, 0xFF, 0x01,
  0x00, 0xFF, 0xFF, 0x00,
  0x00, 0xFC, 0x3F, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// --- Weather icon (32x32 XBM) ---
// A cloud + sun icon
static const uint8_t icon_weather[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x06, 0x00,
  0x00, 0x00, 0x06, 0x00,
  0x00, 0x30, 0x00, 0x00,
  0x00, 0x18, 0x00, 0x01,
  0x00, 0x0C, 0x80, 0x00,
  0x00, 0xF8, 0x1F, 0x00,
  0x00, 0xFE, 0x7F, 0x00,
  0x00, 0x0F, 0xF0, 0x00,
  0x80, 0x07, 0xE0, 0x01,
  0xC0, 0x03, 0xC0, 0x03,
  0x1C, 0x03, 0xC0, 0x03,
  0x00, 0x03, 0xC0, 0x03,
  0x80, 0x07, 0xE0, 0x01,
  0x00, 0x0F, 0xF0, 0x38,
  0x00, 0xFE, 0x7F, 0x00,
  0x00, 0xFC, 0x3F, 0x00,
  0x00, 0x0C, 0x00, 0x01,
  0xC0, 0x3F, 0x00, 0x00,
  0xE0, 0x7F, 0x00, 0x00,
  0xF0, 0xE1, 0x00, 0x00,
  0x38, 0xC0, 0xFF, 0x01,
  0x18, 0x80, 0xFF, 0x03,
  0x18, 0x00, 0x00, 0x07,
  0x18, 0x00, 0x00, 0x06,
  0x38, 0x00, 0x00, 0x06,
  0xF0, 0x00, 0x00, 0x07,
  0xE0, 0xFF, 0xFF, 0x03,
  0x80, 0xFF, 0xFF, 0x01,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

struct MenuItem {
  const uint8_t* icon;
  const char*    label;
  AppState       targetState;
};

MenuItem menuItems[] = {
  { icon_music,    "Music",    STATE_MUSIC    },
  { icon_settings, "Settings", STATE_SETTINGS },
  { icon_timer,    "Timer",    STATE_TIMER    },
  { icon_weather,  "Weather",  STATE_WEATHER  }
};

const int MENU_COUNT = sizeof(menuItems) / sizeof(menuItems[0]);

int menuIndex = 0;

// ======================================================
// SETTINGS
// ======================================================

// Sub-items inside the Settings screen
const char* settingsItems[] = { "Inactivity" };
const int   SETTINGS_COUNT  = sizeof(settingsItems) / sizeof(settingsItems[0]);
int         settingsIndex   = 0;

// Inactivity timeout (saved & editing values)
int inactivityTimeout  = 30;   // saved value (seconds), 0 = disabled
int inactivityEdit     = 30;   // temp value while editing

// Inactivity auto-home timer
uint32_t lastActivityTime = 0;

// Hold-repeat acceleration for inactivity editor
uint32_t inactRightStart  = 0;
uint32_t inactLeftStart   = 0;
uint32_t inactLastRepeat  = 0;
int      inactRepeatCount = 0;

// ======================================================
// MUSIC GLOBALS
// ======================================================

String currentTrack   = "No Track";
String playbackState  = "PAUSED";
String currentTime    = "";

// ======================================================
// BLE GLOBALS
// ======================================================

bool deviceConnected = false;

BLECharacteristic* trackCharacteristic;
BLECharacteristic* controlCharacteristic;
BLECharacteristic* timeCharacteristic;

// ======================================================
// FLAG: tells loop() to refresh the screen
// ======================================================

bool displayDirty = true;

// ======================================================
// MUSIC HOLD DETECTION  (Left / Right)
// ======================================================

#define HOLD_THRESHOLD_MS 700

uint32_t rightPressStart = 0;
uint32_t leftPressStart  = 0;
bool     rightHeld       = false;
bool     leftHeld        = false;

// ======================================================
// TRACK SCROLL STATE
// ======================================================

#define SCROLL_SPEED_MS   50      // pixels shift interval
#define SCROLL_GAP        40      // pixel gap before wrap
#define SCROLL_MAX_PX     128     // screen width

int      scrollOffset     = 0;
uint32_t lastScrollTime   = 0;
String   lastScrolledTrack = "";

// Resets scroll when track changes
void checkScrollReset() {
  if (currentTrack != lastScrolledTrack) {
    scrollOffset     = 0;
    lastScrolledTrack = currentTrack;
  }
}

// Draws a horizontally scrolling string if wider than
// the screen, otherwise draws it centered.
// y = baseline y coordinate, font must already be set.
void drawScrollingText(const char* text, int y) {

  int textW = u8g2.getStrWidth(text);

  if (textW <= SCROLL_MAX_PX) {

    // Fits on screen – just centre it
    u8g2.drawStr((SCROLL_MAX_PX - textW) / 2, y, text);

  } else {

    // Scrolling: draw text twice with a gap
    int totalW = textW + SCROLL_GAP;
    int x1     = -scrollOffset;
    int x2     = x1 + totalW;

    u8g2.drawStr(x1, y, text);
    u8g2.drawStr(x2, y, text);

    // Wrap when first copy is fully off-screen
    if (scrollOffset >= totalW) {
      scrollOffset = 0;
    }
  }
}

// ======================================================
// DISPLAY – HOME SCREEN
// ======================================================

void drawHomeScreen() {

  u8g2.clearBuffer();

  // ---- Time or device name (large, centered) ----

  u8g2.setFont(u8g2_font_ncenB14_tr);

  const char* title =
    (currentTime.length() > 0) ? currentTime.c_str() : DEVICE_NAME;

  int tw = u8g2.getStrWidth(title);
  u8g2.drawStr((128 - tw) / 2, 24, title);

  // ---- Connection status ----

  u8g2.setFont(u8g2_font_ncenB08_tr);

  const char* status =
    deviceConnected ? "Connected" : "Disconnected";

  int sw = u8g2.getStrWidth(status);
  u8g2.drawStr((128 - sw) / 2, 40, status);

  // ---- Track name if connected (scrolling) ----

  if (deviceConnected) {

    checkScrollReset();

    u8g2.setFont(u8g2_font_ncenB08_tr);
    drawScrollingText(currentTrack.c_str(), 52);
  }

  // ---- Hint ----

  u8g2.setFont(u8g2_font_5x7_tr);

  const char* hint = "Press OK";
  int hw = u8g2.getStrWidth(hint);
  u8g2.drawStr((128 - hw) / 2, 63, hint);

  u8g2.sendBuffer();
}

// ======================================================
// DISPLAY – MENU SCREEN
// ======================================================

void drawMenuScreen() {

  u8g2.clearBuffer();

  // ---- Icon (32x32 centred) ----

  const uint8_t* icon = menuItems[menuIndex].icon;
  u8g2.drawXBMP(48, 4, 32, 32, icon);

  // ---- Label centred below icon ----

  u8g2.setFont(u8g2_font_ncenB10_tr);

  const char* label = menuItems[menuIndex].label;
  int lw = u8g2.getStrWidth(label);
  u8g2.drawStr((128 - lw) / 2, 52, label);

  // ---- Left arrow (drawn triangle) ----

  if (menuIndex > 0) {
    u8g2.drawTriangle(10, 20, 2, 24, 10, 28);
  }

  // ---- Right arrow (drawn triangle) ----

  if (menuIndex < MENU_COUNT - 1) {
    u8g2.drawTriangle(118, 20, 126, 24, 118, 28);
  }

  // ---- Page indicator dots ----

  if (MENU_COUNT > 1) {

    int dotSpacing = 10;
    int totalWidth = (MENU_COUNT - 1) * dotSpacing;
    int startX     = (128 - totalWidth) / 2;

    for (int i = 0; i < MENU_COUNT; i++) {

      int cx = startX + i * dotSpacing;

      if (i == menuIndex) {
        u8g2.drawDisc(cx, 59, 2);  // filled
      } else {
        u8g2.drawCircle(cx, 59, 2); // hollow
      }
    }
  }

  u8g2.sendBuffer();
}

// ======================================================
// DISPLAY – MUSIC SCREEN
// ======================================================

void drawMusicScreen() {

  u8g2.clearBuffer();

  // ---- Header ----

  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 12, "Now Playing");

  // ---- Divider line ----

  u8g2.drawHLine(0, 15, 128);

  // ---- Track name (scrolling) ----

  checkScrollReset();

  u8g2.setFont(u8g2_font_ncenB08_tr);
  drawScrollingText(currentTrack.c_str(), 32);

  // ---- Playback state with icon ----

  u8g2.setFont(u8g2_font_open_iconic_play_1x_t);

  if (playbackState == "PLAYING") {
    u8g2.drawGlyph(52, 46, 0x45);  // play icon
  } else {
    u8g2.drawGlyph(52, 46, 0x44);  // pause icon
  }

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(64, 46, playbackState.c_str());

  // ---- Button hints ----

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0,  63, "<Prev");
  u8g2.drawStr(50, 63, "PP");
  u8g2.drawStr(96, 63, "Next>");

  u8g2.sendBuffer();
}

// ======================================================
// SEND CONTROL COMMAND  (BLE)
// ======================================================

void sendControlCommand(String cmd) {

  if (!deviceConnected) return;

  Serial.print("Sending Command: ");
  Serial.println(cmd);

  controlCharacteristic->setValue(cmd.c_str());
  controlCharacteristic->notify();
}

// ======================================================
// SERVER CALLBACKS
// ======================================================

class MyServerCallbacks : public BLEServerCallbacks {

  void onConnect(BLEServer* pServer) {

    deviceConnected = true;

    digitalWrite(2, HIGH);

    Serial.println("Phone Connected");

    displayDirty = true;
  }

  void onDisconnect(BLEServer* pServer) {

    deviceConnected = false;

    digitalWrite(2, LOW);

    Serial.println("Phone Disconnected");

    BLEDevice::startAdvertising();

    displayDirty = true;
  }
};

// ======================================================
// TRACK CALLBACKS
// ======================================================
class TimeCallbacks : public BLECharacteristicCallbacks {

  void onWrite(BLECharacteristic* pCharacteristic) {

    String value = pCharacteristic->getValue();
    Serial.println(value);
    if (value.length() > 0) {

      Serial.print("Time Received: ");
      Serial.println(value);

      currentTime  = value;
      displayDirty = true;
    }
  }
};

class TrackCallbacks : public BLECharacteristicCallbacks {

  void onWrite(BLECharacteristic* pCharacteristic) {

    String value = pCharacteristic->getValue();

    if (value.length() > 0) {

      Serial.print("Received: ");
      Serial.println(value);

      // ==========================================
      // FORMAT:
      // TRACK_NAME*|*PLAYING
      // ==========================================

      int separator = value.indexOf("*|*");

      if (separator != -1) {

        currentTrack =
          value.substring(0, separator);

        playbackState =
          value.substring(separator + 3);
      }
      else {

        currentTrack = value;
      }

      displayDirty = true;
    }
  }
};

// ======================================================
// BUTTON HELPERS
// ======================================================

bool btnPressed(int pin, uint32_t &lastPress) {

  if (!digitalRead(pin)) {

    if (millis() - lastPress > DEBOUNCE_MS) {

      lastPress = millis();
      return true;
    }
  }

  return false;
}

// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(115200);

  // ====================================================
  // GPIO
  // ====================================================

  pinMode(2, OUTPUT);

  pinMode(CONFIRM_BTN, INPUT_PULLUP);
  pinMode(RIGHT_BTN,   INPUT_PULLUP);
  pinMode(LEFT_BTN,    INPUT_PULLUP);
  pinMode(BACK_BTN,    INPUT_PULLUP);

  // ====================================================
  // I2C
  // ====================================================

  Wire.begin(22, 21);

  // ====================================================
  // OLED INIT
  // ====================================================

  u8g2.begin();

  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_ncenB08_tr);

  u8g2.drawStr(10, 30, "Starting...");

  u8g2.sendBuffer();

  delay(1000);

  // ====================================================
  // BLE INIT
  // ====================================================

  BLEDevice::init(DEVICE_NAME);

  BLEServer* pServer = BLEDevice::createServer();

  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  
  BLEService* pTimeService = pServer->createService(SERVICE_TIME_UUID);

  // ====================================================
  // TRACK CHARACTERISTIC
  // Android -> ESP32
  // ====================================================

  trackCharacteristic =
    pService->createCharacteristic(
      TRACK_CHAR_UUID,

      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_READ
    );
  timeCharacteristic =
    pTimeService->createCharacteristic(
      TIME_CHAR_UUID,

      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_READ
    );

  trackCharacteristic->setCallbacks(
    new TrackCallbacks()
  );
  timeCharacteristic->setCallbacks(
    new TimeCallbacks()
  );

  BLE2901* trackDesc = new BLE2901();

  trackDesc->setDescription("Track Metadata");

  trackCharacteristic->addDescriptor(trackDesc);

  BLE2901* timeDesc = new BLE2901();

  timeDesc->setDescription("Time Information");

  timeCharacteristic->addDescriptor(timeDesc);

  // ====================================================
  // CONTROL CHARACTERISTIC
  // ESP32 -> Android
  // ====================================================

  controlCharacteristic =
    pService->createCharacteristic(
      CONTROL_CHAR_UUID,

      BLECharacteristic::PROPERTY_NOTIFY
    );

  controlCharacteristic->addDescriptor(
    new BLE2902()
  );

  BLE2901* controlDesc = new BLE2901();

  controlDesc->setDescription("Media Controls");

  controlCharacteristic->addDescriptor(controlDesc);

  // ====================================================
  // START SERVICE
  // ====================================================

  pService->start();
  pTimeService->start();

  // ====================================================
  // START ADVERTISING
  // ====================================================

  BLEAdvertising* pAdvertising =
    BLEDevice::getAdvertising();

  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->addServiceUUID(SERVICE_TIME_UUID);

  pAdvertising->start();

  Serial.println("BLE Advertising Started");

  // ====================================================
  // DRAW INITIAL HOME SCREEN
  // ====================================================

  currentState = STATE_HOME;
  displayDirty = true;
}

// ======================================================
// DISPLAY – PLACEHOLDER SCREENS
// ======================================================

// ======================================================
// DISPLAY – SETTINGS SCREEN  (sub-menu list)
// ======================================================

void drawSettingsScreen() {

  u8g2.clearBuffer();

  // ---- Header ----

  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 12, "Settings");
  u8g2.drawHLine(0, 15, 128);

  // ---- List items ----

  u8g2.setFont(u8g2_font_ncenB08_tr);

  for (int i = 0; i < SETTINGS_COUNT; i++) {

    int y = 30 + i * 14;

    if (i == settingsIndex) {

      // Highlight bar
      u8g2.drawBox(0, y - 10, 128, 13);
      u8g2.setDrawColor(0);
      u8g2.drawStr(6, y, settingsItems[i]);
      u8g2.setDrawColor(1);

    } else {

      u8g2.drawStr(6, y, settingsItems[i]);
    }
  }

  // ---- Hint ----

  u8g2.setFont(u8g2_font_5x7_tr);
  const char* hint = "OK=Select  Back=Exit";
  int hw = u8g2.getStrWidth(hint);
  u8g2.drawStr((128 - hw) / 2, 63, hint);

  u8g2.sendBuffer();
}

// ======================================================
// DISPLAY – INACTIVITY EDITOR
// ======================================================

void drawInactivityScreen() {

  u8g2.clearBuffer();

  // ---- Header ----

  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 12, "Inactivity");
  u8g2.drawHLine(0, 15, 128);

  // ---- Value (large, centered) ----

  u8g2.setFont(u8g2_font_ncenB14_tr);

  char buf[16];
  sprintf(buf, "%d sec", inactivityEdit);

  int vw = u8g2.getStrWidth(buf);
  u8g2.drawStr((128 - vw) / 2, 38, buf);

  // ---- Left / Right arrows ----

  u8g2.drawTriangle(20, 28, 12, 33, 20, 38);
  u8g2.drawTriangle(108, 28, 116, 33, 108, 38);

  // ---- Hint ----

  u8g2.setFont(u8g2_font_5x7_tr);
  const char* hint = "OK=Save  Back=Cancel";
  int hw = u8g2.getStrWidth(hint);
  u8g2.drawStr((128 - hw) / 2, 63, hint);

  u8g2.sendBuffer();
}

// ======================================================
// DISPLAY – PLACEHOLDER SCREENS
// ======================================================

void drawPlaceholderScreen(const char* title) {

  u8g2.clearBuffer();

  // ---- Title ----

  u8g2.setFont(u8g2_font_ncenB10_tr);

  int tw = u8g2.getStrWidth(title);
  u8g2.drawStr((128 - tw) / 2, 28, title);

  // ---- Coming soon ----

  u8g2.setFont(u8g2_font_5x7_tr);

  const char* sub = "Coming Soon";
  int sw = u8g2.getStrWidth(sub);
  u8g2.drawStr((128 - sw) / 2, 44, sub);

  // ---- Hint ----

  const char* hint = "Press Back";
  int hw = u8g2.getStrWidth(hint);
  u8g2.drawStr((128 - hw) / 2, 63, hint);

  u8g2.sendBuffer();
}

// ======================================================
// LOOP
// ======================================================

void loop() {

  // ====================================================
  // READ BUTTONS  (debounced taps)
  // ====================================================

  bool right   = btnPressed(RIGHT_BTN,   lastRightPress);
  bool left    = btnPressed(LEFT_BTN,    lastLeftPress);
  bool confirm = btnPressed(CONFIRM_BTN, lastConfirmPress);
  bool back    = btnPressed(BACK_BTN,    lastBackPress);

  // ---- Reset inactivity timer on any button press ----

  if (right || left || confirm || back
      || !digitalRead(RIGHT_BTN) || !digitalRead(LEFT_BTN)
      || !digitalRead(CONFIRM_BTN) || !digitalRead(BACK_BTN)) {

    lastActivityTime = millis();
  }

  // ====================================================
  // MUSIC: LEFT / RIGHT HOLD DETECTION
  // Tap   = Volume Up / Down
  // Hold  = Next / Previous track
  // ====================================================

  if (currentState == STATE_MUSIC) {

    // ---- RIGHT button ----

    if (!digitalRead(RIGHT_BTN)) {

      if (rightPressStart == 0) {
        rightPressStart = millis();
        rightHeld       = false;
      }

      if ((millis() - rightPressStart > HOLD_THRESHOLD_MS)
          && !rightHeld) {

        rightHeld = true;
        sendControlCommand("NX");

        Serial.println("Hold RIGHT -> Next");
      }
    }
    else {

      if (rightPressStart != 0) {

        if (!rightHeld) {
          sendControlCommand("VU");

          Serial.println("Tap RIGHT -> Vol Up");
        }

        rightPressStart = 0;
      }
    }

    // ---- LEFT button ----

    if (!digitalRead(LEFT_BTN)) {

      if (leftPressStart == 0) {
        leftPressStart = millis();
        leftHeld       = false;
      }

      if ((millis() - leftPressStart > HOLD_THRESHOLD_MS)
          && !leftHeld) {

        leftHeld = true;
        sendControlCommand("PV");

        Serial.println("Hold LEFT -> Prev");
      }
    }
    else {

      if (leftPressStart != 0) {

        if (!leftHeld) {
          sendControlCommand("VD");

          Serial.println("Tap LEFT -> Vol Down");
        }

        leftPressStart = 0;
      }
    }

    // ---- CONFIRM = Play/Pause ----

    if (confirm) {
      sendControlCommand("PP");
    }

    // ---- BACK = return to menu ----

    if (back) {

      rightPressStart = 0;
      leftPressStart  = 0;

      currentState = STATE_MENU;
      displayDirty = true;

      Serial.println("-> MENU");
    }
  }

  // ====================================================
  // STATE: HOME
  // ====================================================

  else if (currentState == STATE_HOME) {

    if (confirm) {

      currentState = STATE_MENU;
      menuIndex    = 0;
      displayDirty = true;

      Serial.println("-> MENU");
    }
  }

  // ====================================================
  // STATE: MENU
  // ====================================================

  else if (currentState == STATE_MENU) {

    if (right && menuIndex < MENU_COUNT - 1) {

      menuIndex++;
      displayDirty = true;

      Serial.print("Menu -> ");
      Serial.println(menuItems[menuIndex].label);
    }

    if (left && menuIndex > 0) {

      menuIndex--;
      displayDirty = true;

      Serial.print("Menu -> ");
      Serial.println(menuItems[menuIndex].label);
    }

    if (confirm) {

      currentState = menuItems[menuIndex].targetState;
      displayDirty = true;

      Serial.print("Open: ");
      Serial.println(menuItems[menuIndex].label);
    }

    if (back) {

      currentState = STATE_HOME;
      displayDirty = true;

      Serial.println("-> HOME");
    }
  }

  // ====================================================
  // STATE: SETTINGS  (sub-menu)
  // ====================================================

  else if (currentState == STATE_SETTINGS) {

    if (right && settingsIndex < SETTINGS_COUNT - 1) {
      settingsIndex++;
      displayDirty = true;
    }

    if (left && settingsIndex > 0) {
      settingsIndex--;
      displayDirty = true;
    }

    if (confirm) {

      if (settingsIndex == 0) {

        // Open Inactivity editor
        inactivityEdit = inactivityTimeout;
        currentState   = STATE_INACTIVITY;
        displayDirty   = true;

        Serial.println("-> INACTIVITY");
      }
    }

    if (back) {

      currentState = STATE_MENU;
      displayDirty = true;

      Serial.println("-> MENU");
    }
  }

  // ====================================================
  // STATE: INACTIVITY EDITOR
  // ====================================================

  else if (currentState == STATE_INACTIVITY) {

    // ---- Hold-based exponential adjustment ----
    // Step starts at 1, doubles every 600ms of holding

    // RIGHT held
    if (!digitalRead(RIGHT_BTN)) {

      if (inactRightStart == 0) {
        inactRightStart = millis();
        inactRepeatCount = 0;
        inactLastRepeat  = millis();
        inactivityEdit++;
        displayDirty = true;
      }
      else {
        uint32_t held = millis() - inactRightStart;
        // Compute step: doubles every 600ms
        int step = 1;
        int levels = held / 600;
        for (int i = 0; i < levels && i < 8; i++) step *= 2;

        // Repeat interval: faster as step grows
        uint32_t interval = (step <= 2) ? 200 : 100;

        if (millis() - inactLastRepeat >= interval) {
          inactivityEdit  += step;
          inactLastRepeat  = millis();
          displayDirty     = true;
        }
      }
    }
    else {
      inactRightStart = 0;
    }

    // LEFT held
    if (!digitalRead(LEFT_BTN)) {

      if (inactLeftStart == 0) {
        inactLeftStart  = millis();
        inactRepeatCount = 0;
        inactLastRepeat  = millis();
        if (inactivityEdit > 0) {
          inactivityEdit--;
          displayDirty = true;
        }
      }
      else {
        uint32_t held = millis() - inactLeftStart;
        int step = 1;
        int levels = held / 600;
        for (int i = 0; i < levels && i < 8; i++) step *= 2;

        uint32_t interval = (step <= 2) ? 200 : 100;

        if (millis() - inactLastRepeat >= interval) {
          inactivityEdit  -= step;
          if (inactivityEdit < 0) inactivityEdit = 0;
          inactLastRepeat  = millis();
          displayDirty     = true;
        }
      }
    }
    else {
      inactLeftStart = 0;
    }

    if (confirm) {

      // Save and go back to settings
      inactivityTimeout = inactivityEdit;
      currentState      = STATE_SETTINGS;
      displayDirty      = true;

      Serial.print("Inactivity saved: ");
      Serial.println(inactivityTimeout);
    }

    if (back) {

      // Discard and go back to settings
      currentState = STATE_SETTINGS;
      displayDirty = true;

      Serial.println("-> SETTINGS (cancelled)");
    }
  }

  // ====================================================
  // STATE: TIMER / WEATHER  (placeholders)
  // ====================================================

  else if (currentState == STATE_TIMER
        || currentState == STATE_WEATHER) {

    if (back) {

      currentState = STATE_MENU;
      displayDirty = true;

      Serial.println("-> MENU");
    }
  }

  // ====================================================
  // INACTIVITY TIMEOUT -> HOME
  // ====================================================

  if (inactivityTimeout > 0
      && currentState != STATE_HOME
      && (millis() - lastActivityTime
          >= (uint32_t)inactivityTimeout * 1000)) {

    currentState = STATE_HOME;
    displayDirty = true;

    Serial.println("Inactivity -> HOME");
  }

  // ====================================================
  // SCROLL TICKER
  // ====================================================

  if ((currentState == STATE_HOME || currentState == STATE_MUSIC)
      && millis() - lastScrollTime >= SCROLL_SPEED_MS) {

    u8g2.setFont(u8g2_font_ncenB08_tr);
    int trackW = u8g2.getStrWidth(currentTrack.c_str());

    if (trackW > SCROLL_MAX_PX) {

      scrollOffset++;
      lastScrollTime = millis();
      displayDirty   = true;
    }
  }

  // ====================================================
  // REFRESH DISPLAY
  // ====================================================

  if (displayDirty) {

    displayDirty = false;

    switch (currentState) {

      case STATE_HOME:
        drawHomeScreen();
        break;

      case STATE_MENU:
        drawMenuScreen();
        break;

      case STATE_MUSIC:
        drawMusicScreen();
        break;

      case STATE_SETTINGS:
        drawSettingsScreen();
        break;

      case STATE_INACTIVITY:
        drawInactivityScreen();
        break;

      case STATE_TIMER:
        drawPlaceholderScreen("Timer");
        break;

      case STATE_WEATHER:
        drawPlaceholderScreen("Weather");
        break;
    }
  }
}