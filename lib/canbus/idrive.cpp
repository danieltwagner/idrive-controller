#include "idrive.h"

iDrive::iDrive(CanBus &bus) : canBus(bus), deviceID(-1) {
    canBus.subscribe(this);
};

void iDrive::subscribe(iDriveCallback *callback) {
    callbacks.push_back(callback);
}

// Examples

// main menu
// enqueue({id, 8, {0xF0, 0x7F, 0,0,0,0,0,0}});

// ok, fine, with limits on both sides, position set
// enqueue({0x1AE, 8, {0xF5, 0x2A, 0x0A, 0x06, 0x04, 0x00, 0xFF, 0x7F}});

// Spinning: in the phone numbers menu
// enqueue({0x1AE, 8, {0xF4, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}});

// in the phone canciones menu - jumps
// enqueue({0x1AE, 8, {0xF3, 0x7F, 0x0A, 0x06, 0x00, 0x00, 0x35, 0x7F}});

// idrive_haptic_rough
// enqueue({0x1AE, 8, {0xF1, 0x7F, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00}});

// idrive_haptic_fine
// enqueue({0x1AE, 8, {0xF1, 0x7F, 0x0A, 0xFF, 0x00, 0x00, 0x20, 0x7F}});

// Custom
// enqueue({0x1AE, 8, {0xF2, 0x7F, 0x20, 0x08, 0x00, 0x00, 0x20, 0x7F}});
// enqueue({0x1AA, 8, {0xF2, 0x7F, 0x20, 0x08, 0x00, 0x00, 0x20, 0x7F}});

// Dimmer menu/user buttons to minimum
// enqueue({0x202, 2, {0x00, 0xFF}});

// Dimmer menu/user buttons to maximum
// enqueue({0x202, 2, {0xEF, 0xFF}});

bool iDrive::configureHaptics(
    HapticMode mode,
    uint8_t clickHardness,
    uint8_t stepDistance,
    uint8_t numSteps,
    uint8_t startPos,
    uint8_t hillSize,
    uint8_t endResistance
) {
    this->mode = mode;
    this->clickHardness = clickHardness;
    this->stepDistance = stepDistance;
    this->numSteps = numSteps;
    this->lastStepPos = startPos;
    this->hillSize = hillSize;
    this->endResistance = endResistance;

    return sendHapticConfig();
}

bool iDrive::sendHapticConfig() {
    if (this->deviceID < 0) {
        return false;
    }

    canBus.enqueue({
        this->deviceID,
        8,
        {
            static_cast<uint8_t>(this->mode),
            this->clickHardness,
            static_cast<uint8_t>(this->stepDistance >> 4),
            this->numSteps,
            this->lastStepPos,
            0x00, // what does this do?
            this->hillSize,
            this->endResistance
        }
    });
    return true;
}

void iDrive::handleMessage(int id, int size, uint8_t *data, bool incoming) {
    int function_code = id & 0x780; // 4 MSB
    int cob_id = id & 0x7F; // 7 LSB

    switch(function_code) {
        case 0x180:
            if (cob_id == 0x38 || cob_id == 0x40) {
                // COB-ID 0x38: front iDrive rotation
                // COB-ID 0x40: rear iDrive rotation

                // TODO: Something still isn't right with IDs.
                // Maybe other message types don't use  this type of addressing? e.g.
                // 1AA -> 180 0x2A message (from front BZM) to set haptics for front
                // 1AE -> 180 0x2E message (from rear BZM) to set haptics for rear
                // 1B8 -> 180 0x38 rotation from front iDrive controller
                // 1C0 -> 180 0x40 rotation from rear iDrive controller
                // but:
                // 4E7 -> 480 0x67 handshake from front iDrive controller
                // 4E8 -> 480 0x68 handshake from rear iDrive controller
                // 5E7 -> 580 0x67 info for how to send haptic control msg (from front iDrive)
                // 5E8 -> 580 0x68 info for how to send haptic control msg (from rear iDrive)

                int16_t newRotation = (int16_t)(data[3] << 8) + data[2];
                if (stepDistance) {
                    int newStepPos = (newRotation + stepDistance/2)/stepDistance;
                    newStepPos = min((int)numSteps - 1, max(0, newStepPos));
                    if (newStepPos != lastStepPos) {
                    sendStepsChanged(lastStepPos, newStepPos);
                    lastStepPos = newStepPos;
                    }
                } else {
                    sendRotationChanged(newRotation);
                }

                uint8_t newButtonStatus = data[1];
                if (newButtonStatus != lastButtonStatus) {

                    // wheel depressed
                    if (((newButtonStatus & 0xC1) == 0xC1) && ((lastButtonStatus & 0xC1) != 0xC1)) {
                        sendKnobChanged(true);
                    } else if (((newButtonStatus & 0xC1) != 0xC1) && ((lastButtonStatus & 0xC1) == 0xC1)) {
                        sendKnobChanged(false);
                    }

                    // menu button
                    if (((newButtonStatus & 0xC4) == 0xC4) && ((lastButtonStatus & 0xC4) != 0xC4)) {
                        sendMenuButtonChanged(true);
                    } else if (((lastButtonStatus & 0xC4) == 0xC4) && ((newButtonStatus & 0xC4) != 0xC4)) {
                        sendMenuButtonChanged(false);
                    }

                    // <> (user button)
                    if (((newButtonStatus & 0xD0) == 0xD0) && ((lastButtonStatus & 0xD0) != 0xD0)) {
                        sendUserButtonChanged(true);
                    } else if (((lastButtonStatus & 0xD0) == 0xD0) && ((newButtonStatus & 0xD0) != 0xD0)) {
                        sendUserButtonChanged(false);
                    }

                    lastButtonStatus = newButtonStatus;
                }
            }
            break;

        case 0x580:
            this->deviceID = (data[1] << 8) + data[2];
            sendHapticConfig();
            break;
    }
}

void iDrive::sendStepsChanged(int oldStepPos, int newStepPos) {
    for (std::list<iDriveCallback *>::iterator it=callbacks.begin(); it != callbacks.end(); ++it) {
        (*it)->stepsChanged(oldStepPos, newStepPos);
    }
}

void iDrive::sendRotationChanged(int newRotation) {
    for (std::list<iDriveCallback *>::iterator it=callbacks.begin(); it != callbacks.end(); ++it) {
        (*it)->rotationChanged(newRotation);
    }
}

void iDrive::sendKnobChanged(bool pressed) {
    for (std::list<iDriveCallback *>::iterator it=callbacks.begin(); it != callbacks.end(); ++it) {
        (*it)->knobChanged(pressed);
    }
}

void iDrive::sendMenuButtonChanged(bool pressed) {
    for (std::list<iDriveCallback *>::iterator it=callbacks.begin(); it != callbacks.end(); ++it) {
        (*it)->menuButtonChanged(pressed);
    }
}

void iDrive::sendUserButtonChanged(bool pressed) {
    for (std::list<iDriveCallback *>::iterator it=callbacks.begin(); it != callbacks.end(); ++it) {
        (*it)->userButtonChanged(pressed);
    }
}
