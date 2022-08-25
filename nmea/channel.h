#pragma once

#include <thread>
#include <functional>

#include "nmea.h"
#include "reader.h"
#include "serial_reader.h"
#include "udp_reader.h"

class Channel {
    public:
        Channel (nmea::ConnectionType _type = nmea::ConnectionType::SERIAL, nmea::Cb _cb = nullptr);
        virtual ~Channel ();
        void setCb (nmea::Cb _cb) { cb = _cb; }
        void activate (bool flag) { active = flag; }
        void stop () { running = false; }
        nmea::ConnectionType getType () { return type; }
        Reader *getReader (nmea::ConnectionType);

    private:
        std::thread *worker;
        bool running, active;
        nmea::Cb cb;
        nmea::ConnectionType type;
        std::map<nmea::ConnectionType, Reader *> readers;

        void workerProc ();
};
