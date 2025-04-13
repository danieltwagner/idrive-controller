#ifndef CANBUS_H
#define CANBUS_H

#include <Arduino.h>
#include <list>
#include <set>

typedef struct {
    int id;
    int length;
    uint8_t data[8];
    // Data fields are:
    // - haptic mode
    // - click hardness
    // - step to step distance
    // - number of steps
    // - start position
    // - reserved 0x00 ?
    // - constant brake strength
    // - end stop force
} cmd;

enum class Handshake {
    PoweringUp,
    Announced,
    SteadySate,
};

class CanBusCallback {
    public:
        virtual void handleMessage(int id, int size, uint8_t *data, bool incoming);
};

class CanBus {
    public:
        CanBus(int rxPin, int txPin);
        void begin();
        void loop();
        void enqueue(cmd what, bool allowDuplicates = false);
        void subscribe(CanBusCallback *callback);

    private:
        void handleMessage(int id, int size, uint8_t *data, bool incoming);
        bool sendPacket(cmd what, boolean print=false);

        void startNewHandshake();
        void handleHandshake(int nodeId, int size, uint8_t *data);
        void sendHandshake(uint8_t nodeId, uint8_t nextNodeId, uint8_t tag, bool ready);

        std::list<CanBusCallback *> callbacks;
        long lastMessageTs = 0;
        std::list<cmd> commandsToSend;
        long lastHandshakeTs = 0;
        int lastTaggedNodeId = 0;
        std::set<uint8_t> announcedNodes;
        uint8_t controllerNodeId = 0;
        Handshake handshakeState = Handshake::PoweringUp;
};

#endif // CANBUS_H
