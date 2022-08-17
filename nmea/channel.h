#pragma once

#include <thread>
#include <functional>

#include "nmea.h"
#include "reader.h"
#include "serial_reader.h"

class Channel {
    public:
        Reader *getReader () { return reader; }

        Channel (nmea::ConnectionType _type = nmea::ConnectionType::SERIAL, nmea::Cb _cb = nullptr);
        virtual ~Channel ();
        void setCb (nmea::Cb _cb) { cb = _cb; }
        void activate (bool flag) { active = flag; }
        void stop () { running = false; }

    private:
        std::thread *worker;
        bool running, active;
        nmea::Cb cb;
        nmea::ConnectionType type;
        Reader *reader;

        void workerProc ();
};
