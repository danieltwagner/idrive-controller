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

uint8_t stepPos = 0;
int rotation = 0;

int myMode = 0;
const char *presetName = "";
HapticMode mode = HapticMode::STEPS_BETWEEN_SPRINGS;
uint8_t clickHardness = 0x7F;
uint8_t stepDistance = 0xA0;
uint8_t numSteps = 10;
uint8_t hillSize = 0x7F;
uint8_t endResistance = 0x7F;

void printSettings();
void printState();

void myModeJumps() {
    presetName = "Jumps";
    mode = HapticMode::STEPS_BETWEEN_SPRINGS;
    clickHardness = 0x7F;
    stepDistance = 0xA0;
    numSteps = 6;
    stepPos = 0;
    hillSize = 0x35;
    endResistance = 0x7F;
}

void myModeSteps() {
    presetName = "Steps";
    mode = HapticMode::STEPS_BETWEEN_SPRINGS;
    clickHardness = 0x7F;
    stepDistance = 0x50;
    numSteps = 10;
    stepPos = 0;
    hillSize = 0;
    endResistance = 0x30;
}

void myModeStepsLessHard() {
    presetName = "Steps less hard";
    mode = HapticMode::STEPS_BETWEEN_SPRINGS;
    clickHardness = 0x30;
    stepDistance = 0x50;
    numSteps = 10;
    stepPos = 0;
    hillSize = 0;
    endResistance = 0x30;
}

void myModeSmallSteps() {
    presetName = "Small steps";
    mode = HapticMode::STEPS_BETWEEN_SPRINGS;
    clickHardness = 0x7F;
    stepDistance = 0x20;
    numSteps = 100;
    stepPos = 0;
    hillSize = 0;
    endResistance = 0xFF;
}

void myModeF5() {
    presetName = "F5 preset";
    mode = HapticMode::UNKNOWN_F5;
    clickHardness = 0x2A;
    stepDistance = 0;
    numSteps = 20;
    stepPos = 0;
    hillSize = 0x35;
    endResistance = 0;
}
void myModeF5variant() {
    presetName = "F5 preset 2";
    mode = HapticMode::UNKNOWN_F5;
    clickHardness = 0x2A;
    stepDistance = 0x40;
    numSteps = 20;
    stepPos = 0;
    hillSize = 0xFF;
    endResistance = 0x7F;
}

void myModeF4MovesTheDial() {
    presetName = "F4 preset, moves dial";
    mode = HapticMode::UNKNOWN_F4;
    clickHardness = 0x2A;
    stepDistance = 0x40;
    numSteps = 20;
    stepPos = 10;
    hillSize = 0x35;
    endResistance = 0xFF;
}

void stepsWithHills() {
    presetName = "Steps + big hills";
    mode = HapticMode::STEPS_BETWEEN_HILLS;
    clickHardness = 0x7F;
    stepDistance = 0x40;
    numSteps = 7;
    stepPos = 4;
    hillSize = 0x80;
    endResistance = 0x7F;
}

void stepsWithSmallHills() {
    presetName = "Steps + small hills";
    mode = HapticMode::STEPS_BETWEEN_HILLS;
    clickHardness = 0x7F;
    stepDistance = 0x40;
    numSteps = 7;
    stepPos = 4;
    hillSize = 0x20;
    endResistance = 0x7F;
}

void stepsWithSmallHillsFeelsTheSame() {
    presetName = "Steps + small hills hard";
    mode = HapticMode::STEPS_BETWEEN_HILLS;
    clickHardness = 0x7F;
    stepDistance = 0x40;
    numSteps = 7;
    stepPos = 4;
    hillSize = 0x20;
    endResistance = 0xFF;
}

const int NUM_MODES = 10;
void applyMode() {
    switch (myMode) {
        case 0:
            myModeJumps();
            break;
        case 1:
            myModeSteps();
            break;
        case 2: 
            myModeStepsLessHard();
            break;
        case 3:
            myModeSmallSteps();
            break;
        case 4:
            myModeF4MovesTheDial();
            break;
        case 5:
            myModeF5();
            break;
        case 6:
            myModeF5variant();
            break;
        case 7:
            stepsWithHills();
            break;
        case 8:
            stepsWithSmallHills();
            break;
        case 9:
            stepsWithSmallHillsFeelsTheSame();
            break;
    }
    idrive.configureHaptics(mode, clickHardness, stepDistance, numSteps, stepPos, hillSize, endResistance);
    printSettings();
    printState();
}

class iDriveHandler : public iDriveCallback {

    void stepsChanged(int oldStepPos, int newStepPos) {
        stepPos = newStepPos;
        printState();
    }
    
    void rotationChanged(int newRotation) {
        rotation = newRotation;
        printState();
    }
    
    void knobChanged(bool pressed) {
        // No action
    }
    
    void menuButtonChanged(bool pressed) {
        if (pressed) {
            myMode = (myMode + 1) % NUM_MODES;
            applyMode();
        }
    }
    
    void userButtonChanged(bool pressed) {
        if (pressed) {
            myMode = (myMode + NUM_MODES - 1) % NUM_MODES;
            applyMode();
        }
    }
};

iDriveHandler *handler = new iDriveHandler();

void setup() {
    Serial.begin(115200);
    
    setupTft();
    printSettings();
    printState();
    
    idrive.subscribe(handler);
    canBus.begin();

    // Apply our initial settings
    applyMode();
}

void printSettings() {
    String modeString = "Unknown";
    switch (mode) {
        case HapticMode::HAPTIC_DISABLED:
        modeString = "Disabled";
        break;
        case HapticMode::STEPS_NO_ENDSTOPS:
        modeString = "Steps no endstops";
        break;
        case HapticMode::STEPS_BETWEEN_HILLS:
        modeString = "Steps big notch then free";
        break;
        case HapticMode::STEPS_BETWEEN_SPRINGS:
        modeString = "Steps springy ends";
        break;
    }
    
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.print(presetName);
    tft.setCursor(5, 25);
    tft.printf("Haptic mode: \n  %s", modeString.c_str());
    tft.setCursor(5, 45);
    tft.printf("Step distance: %d", stepDistance);
    tft.setCursor(5, 55);
    tft.printf("Click hardness: %d", clickHardness);
    tft.setCursor(5, 65);
    tft.printf("Hill size: %d", hillSize);
    tft.setCursor(5, 75);
    tft.printf("End resistance: %d", endResistance);
}

void printState() {
    
    // Selection
    tft.fillRect(0, 95, 160, 24, ST77XX_BLACK);
    tft.setCursor(5, 95);
    tft.setTextSize(3);
    if (stepDistance == 0) {
        tft.printf("%d", rotation);
    } else {
        tft.printf("%d / %d", stepPos, numSteps - 1);
    }
}

void loop() {
    canBus.loop();
}
