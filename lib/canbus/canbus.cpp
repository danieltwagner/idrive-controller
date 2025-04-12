#include "canbus.h"
#include <CAN.h>

uint8_t buf[8];

bool operator==(const cmd& lhs, const cmd& rhs)
{
    return (
        lhs.id == rhs.id && 
        lhs.length == rhs.length && 
        std::equal(std::begin(lhs.data), std::end(lhs.data), std::begin(rhs.data))
    );
}

CanBus::CanBus(int rxPin, int txPin) {
    Serial.println("CAN Receiver");

    // start the CAN bus at 100 kbps
    CAN.setPins(rxPin, txPin);
}

void CanBus::begin() {
    if (!CAN.begin(100E3)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }
}

void CanBus::loop() {
    // try to parse packet
    int packetSize = CAN.parsePacket();

    if (packetSize) {
        CAN.readBytes(buf, packetSize);
        handleMessage(CAN.packetId(), packetSize, buf, true);

    } else if (!commandsToSend.empty()) {
        cmd toSend = *commandsToSend.begin();
        if (sendPacket(toSend)) {
            commandsToSend.pop_front();
            // play it back to ourselves
            handleMessage(toSend.id, toSend.length, toSend.data, false);
        }
    } else if (millis() - lastHandshakeTs > 1000) {
        // start a new handshake from the 00 device
        startNewHandshake();
    }
}

void CanBus::enqueue(cmd what, bool allowDuplicates) {
    if (!allowDuplicates) {
        for (std::list<cmd>::iterator it=commandsToSend.begin(); it != commandsToSend.end(); ++it) {
            if (*it == what) {
                return;
            }
        }
    }
    commandsToSend.push_back(what);
}

bool CanBus::sendPacket(cmd what, boolean print) {
    if (print) {
        Serial.print("Sending ");
        Serial.print(what.id, HEX);
        Serial.print(": ");
    }
    CAN.beginPacket(what.id);
    for (int i = 0; i < what.length; i++) {
        CAN.write(what.data[i]);
        if (print) {
            Serial.print(what.data[i], HEX);
            Serial.print(" ");
        }
    }

    bool result = CAN.endPacket();

    if (print) {
        Serial.println(result ? "" : "error sending CAN message.");
    }
    return result;
}

void CanBus::handleMessage(int id, int size, uint8_t *data, bool incoming) {
    int function_code = id & 0x780; // 4 MSB
    int cob_id = id & 0x7F; // 7 LSB

    if (function_code == 0x480) {
        handleHandshake(cob_id, size, data);
    }

    for (std::list<CanBusCallback *>::iterator it=callbacks.begin(); it != callbacks.end(); ++it) {
        (*it)->handleMessage(id, size, data, incoming);
    }
}

void CanBus::subscribe(CanBusCallback *callback) {
    this->callbacks.push_back(callback);
}

// ---------------------- handshake ----------------------

void CanBus::startNewHandshake() {
  lastTaggedNodeId = 0x00;

  switch (handshakeState) {
    case Handshake::PoweringUp:
      sendHandshake(0x00, 0x00, 0x01, false);    
      break;
    
    case Handshake::Announced:
    case Handshake::SteadySate:
      sendHandshake(0x00, 0x62, 0x02, true);
      break;
  }
}

void CanBus::handleHandshake(int nodeId, int size, uint8_t *data) {
    bool isControllerTalking = nodeId != 0x00 && nodeId != 0x62;
    if (isControllerTalking) {
        // this must be our controller
        controllerNodeId = nodeId;
    }

    uint8_t taggedNode = data[0];
    uint8_t tag = data[1];
    lastTaggedNodeId = taggedNode;

    if (nodeId == taggedNode && tag == 0x01) {
        announcedNodes.insert(nodeId);
        if (announcedNodes.size() > 2) {
            // TODO: Support more than one device for handshakes
            handshakeState = Handshake::Announced;
            startNewHandshake();
            return;
        }
    }

    if (taggedNode == 0x00 && (tag == 0x12 || tag == 0x52)) {
        if (handshakeState != Handshake::SteadySate) {
            handshakeState = Handshake::SteadySate;
            startNewHandshake();
        }
        return;
    }

    switch (handshakeState) {
        case Handshake::PoweringUp:
            if (nodeId == 0x00) {
                // emulate the BZM powering up
                sendHandshake(0x62, 0x62, 0x01, false);
            }
            break;

        case Handshake::Announced:
            if (taggedNode == 0x62) {
                // emulate the BZM chaining
                sendHandshake(0x62, controllerNodeId, 0x12, true);
            }
            break;

        case Handshake::SteadySate:
            if (taggedNode == 0x62) {
                // emulate the BZM chaining
                sendHandshake(0x62, controllerNodeId, 0x52, true);
            } else if (tag != 0x52) {
                announcedNodes.erase(nodeId);
                handshakeState = Handshake::PoweringUp;
                startNewHandshake();
            }
            break;
    }
}

void CanBus::sendHandshake(uint8_t nodeId, uint8_t nextNodeId, uint8_t tag, bool ready) {
    enqueue({
        0x480 + nodeId, 
        8, 
        {nextNodeId, tag, 0xFF, (uint8_t)(ready ? 0x01 : 0xFF), 0xFF, 0xFF, 0xFF, 0xFF}
    });
    lastHandshakeTs = millis();
}
