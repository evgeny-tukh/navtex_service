#include <thread>
#include <chrono>

#include "nmea.h"
#include "channel.h"

Channel::Channel (nmea::ConnectionType _type, nmea::Cb _cb): type (_type), cb (_cb), running (true), active (false), worker (nullptr) {
    readers.emplace (std::pair<nmea::ConnectionType, Reader *> (nmea::ConnectionType::SERIAL, new SerialReader ()));
    readers.emplace (std::pair<nmea::ConnectionType, Reader *> (nmea::ConnectionType::UDP, new UdpReader ()));

    worker = new std::thread ([this] () { workerProc (); });
}

Channel::~Channel () {
    for (auto reader: readers) {
        if (reader.second) {
            reader.second->disconnect ();
            delete reader.second;
        }
    }
    running = false;
    if (worker) {
        if (worker->joinable ()) worker->join ();
        delete worker;
    }
}

Reader *Channel::getReader (nmea::ConnectionType type) {
    auto pos = readers.find (type);
    return pos == readers.end () ? nullptr : pos->second;
}

void Channel::workerProc () {
    while (running) {
        if (active) {
            Reader::Buffer buffer;
            auto reader = getReader (type);
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
