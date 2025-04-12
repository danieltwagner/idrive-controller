#include <list>
#include <set>
#include <Arduino.h>

// --------- CAN bus + iDrive ----------
#include "canbus.h"
#include "idrive.h"
#define PIN_CAN_RX  21
#define PIN_CAN_TX  19
CanBus canBus = CanBus(PIN_CAN_RX, PIN_CAN_TX);
iDrive idrive = iDrive(canBus);

// --------- TFT ----------

// | Display (Pin) | Comment                     | ESP32 Pin        |
// | ------------- | --------------------------- | ---------------- |
// | GND (1)       |                             | GND              |
// | VCC (2)       | 3.3V                        | 3.3V             |
// | SCK (3)       | Serial Clock (SCL)          | 14 (SCLK/GPIO14) |
// | SDA (4)       | Serial Data Input           | 13 (MOSI/GPIO13) |
// | RES (5)       | LCM Reset (high during use) | 26 (GPIO26)      |
// | RS (6)        | Data/Command Control        | 27 (GPIO27)      |
// | CS (7)        | Chip Select                 | 12 (GPIO12)      |
// | LEDA (8)      | Backlight (3.3V)            | 3.3V             |

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_ST7735.h>  // https://github.com/adafruit/Adafruit-ST7735-Library

#define TFT_PIN_RST  26
#define TFT_PIN_CS   12
#define TFT_PIN_DC   27

SPIClass hspi = SPIClass(HSPI);
Adafruit_ST7735 tft = Adafruit_ST7735(&hspi, TFT_PIN_CS, TFT_PIN_DC, TFT_PIN_RST);

void setupTft() {
    // Ensure the pins are in GPIO mode
    tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab

    // Our CS pin is also HSPI MISO (eep) but since we don't need MISO
    // we can force it back into a regular output. Alternatively we could
    // change the board layout and use a free GPIO (e.g. 25)
    pinMode(TFT_PIN_CS, OUTPUT);
    digitalWrite(TFT_PIN_CS, HIGH);

    tft.setRotation(1);
}

// --------- End TFT ----------

int stepPos = 0;
int steps = 10;

void printState();

class iDriveHandler : public iDriveCallback {

    void rotationChanged(int oldStepPos, int newStepPos) {
        stepPos = newStepPos;
        printState();
    }

    void knobChanged(bool pressed) {
        // No action
    }

    void menuButtonChanged(bool pressed) {
        // No action
    }

    void userButtonChanged(bool pressed) {
        // No action
    }
};

iDriveHandler *handler = new iDriveHandler();

void setup() {
    Serial.begin(115200);

    setupTft();
    printState();
    idrive.configureHapticFeedback(steps, 0);
    idrive.subscribe(handler);
    canBus.begin();
}

void printState() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(5, tft.height()/2 - 10);
  tft.setTextSize(3);
  tft.printf("%d / %d", stepPos, steps - 1);
}

void loop() {
    canBus.loop();
}
