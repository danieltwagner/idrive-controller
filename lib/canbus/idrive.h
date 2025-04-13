#ifndef IDRIVE_H
#define IDRIVE_H

#include <Arduino.h>
#include <list>
#include "canbus.h"

class iDriveCallback {
    public:
        virtual void stepsChanged(int oldStepPos, int newStepPos);
        virtual void rotationChanged(int newRotation);
        virtual void knobChanged(bool pressed);
        virtual void menuButtonChanged(bool pressed);
        virtual void userButtonChanged(bool pressed);
};

enum class HapticMode : uint8_t {
    
    // constant braking force
    HAPTIC_DISABLED = 0xF0,
    
    // no end stops
    STEPS_NO_ENDSTOPS = 0xF1,
    
    // will spin freely after crossing plateau
    STEPS_BETWEEN_HILLS = 0xF2,
    
    // will return to nearest step
    STEPS_BETWEEN_SPRINGS = 0xF3,

    UNKNOWN_F4 = 0xF4,
    UNKNOWN_F5 = 0xF5
};

class iDrive: public CanBusCallback {
    public:
        iDrive(CanBus &bus);

        void subscribe(iDriveCallback *callback);
        bool configureHaptics(HapticMode mode, uint8_t clickHardness, uint8_t stepDistance, uint8_t numSteps, uint8_t startPos, uint8_t hillSize, uint8_t endResistance);
        void handleMessage(int id, int size, uint8_t *data, bool incoming);

    private:
        bool sendHapticConfig();
        void sendStepsChanged(int oldStepPos, int newStepPos);
        void sendRotationChanged(int newRotation);
        void sendKnobChanged(bool pressed);
        void sendMenuButtonChanged(bool pressed);
        void sendUserButtonChanged(bool pressed);

        CanBus &canBus;
        int deviceID;
        std::list<iDriveCallback *> callbacks;
        uint8_t lastButtonStatus = 0;

        // default haptic config
        HapticMode mode = HapticMode::STEPS_BETWEEN_SPRINGS;
        uint8_t numSteps = 10;
        uint8_t lastStepPos = 0;
        uint8_t stepDistance = 0xA0;
        uint8_t clickHardness = 0x7F;
        uint8_t hillSize = 0;
        uint8_t endResistance = 0x7F;
};

#endif // IDRIVE_H
