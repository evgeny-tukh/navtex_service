#pragma once

#include "reader.h"
#include <Windows.h>
#include <cstdint>
#include "../tools/tools.h"

struct SerialReaderCfg: ReaderCfg {
    SerialReaderCfg (): baud (4800), byteSize (8), stopBits (ONESTOPBIT), parity (NOPARITY), port (tools::getFirstAvailablePortNo ()) {}
    uint32_t baud;
    uint8_t port;
    uint8_t byteSize;
    uint8_t stopBits;
    uint8_t parity;
};

class SerialReader: public Reader {
    private:
        HANDLE port;

    public:
        SerialReader (): port (INVALID_HANDLE_VALUE) {
            cfg = new SerialReaderCfg;
        }
        virtual ~SerialReader () {
            disconnect ();
        }
        virtual bool connect ();
        virtual bool disconnect ();
        virtual size_t dataAvailable ();
        virtual bool getAvailableData (Buffer& result);
        virtual nmea::ConnectionType getType () { return nmea::ConnectionType::SERIAL; }
};
