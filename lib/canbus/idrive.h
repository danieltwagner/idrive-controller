#ifndef IDRIVE_H
#define IDRIVE_H

#include <Arduino.h>
#include <list>
#include "canbus.h"

class iDriveCallback {
    public:
        virtual void rotationChanged(int oldStepPos, int newStepPos);
        virtual void knobChanged(bool pressed);
        virtual void menuButtonChanged(bool pressed);
        virtual void userButtonChanged(bool pressed);
};

class iDrive: public CanBusCallback {
    public:
        iDrive(CanBus &bus);

        void subscribe(iDriveCallback *callback);
        bool configureHapticFeedback(uint8_t numSteps, uint8_t startPos);
        void handleMessage(int id, int size, uint8_t *data, bool incoming);

        uint8_t numSteps;

    private:
        void sendRotationChanged(int oldStepPos, int newStepPos);
        void sendKnobChanged(bool pressed);
        void sendMenuButtonChanged(bool pressed);
        void sendUserButtonChanged(bool pressed);

        CanBus &canBus;
        int deviceID;
        std::list<iDriveCallback *> callbacks;
        uint8_t deltaStepToStep;
        int16_t lastStepPos = -1;
        uint8_t lastButtonStatus = 0;
};

#endif // IDRIVE_H
