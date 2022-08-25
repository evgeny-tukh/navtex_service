#pragma once

#include "reader.h"
#include <Windows.h>
#include <cstdint>
#include <string>
#include <winsock.h>
#include "../tools/tools.h"

struct UdpReaderCfg: ReaderCfg {
    UdpReaderCfg (): inPort (0), outPort (0), bindTo () {}
    uint32_t inPort, outPort;
    std::string bindTo;
};

class UdpReader: public Reader {
    private:
        SOCKET receiver;

    public:
        UdpReader (): receiver (INVALID_SOCKET) {
            cfg = new UdpReaderCfg;
        }
        virtual ~UdpReader () {
            disconnect ();
        }
        static bool forward (const char *data);
        virtual bool connect ();
        virtual bool disconnect ();
        virtual size_t dataAvailable ();
        virtual bool getAvailableData (Buffer& result);
        virtual nmea::ConnectionType getType () { return nmea::ConnectionType::UDP; }
        virtual bool write (char *);
};
