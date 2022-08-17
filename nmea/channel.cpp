#include <thread>
#include <chrono>

#include "nmea.h"
#include "channel.h"

Channel::Channel (nmea::ConnectionType _type, nmea::Cb _cb): type (_type), reader (nullptr), cb (_cb), running (true), active (false), worker (nullptr) {
    switch (_type) {
        case nmea::ConnectionType::SERIAL: reader = new SerialReader (); break;
    }
    worker = new std::thread ([this] () { workerProc (); });
}

Channel::~Channel () {
    if (reader) {
        reader->disconnect ();
        delete reader;
    }
    running = false;
    if (worker) {
        if (worker->joinable ()) worker->join ();
        delete worker;
    }
}

void Channel::workerProc () {
    while (running) {
        if (active) {
            Reader::Buffer buffer;
            if (reader && reader->isConnected ()) {
                reader->getAvailableData (buffer);
                if (!buffer.empty () && cb) {
                    cb ((char *) buffer.data (), buffer.size ());
                }
            }
        }
        std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }
}