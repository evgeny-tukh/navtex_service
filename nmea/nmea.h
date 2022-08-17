#pragma once

#include "defs.h"

namespace nmea {
    enum ConnectionType { UNKNOWN = 0, SERIAL, UDP, };
    typedef void *CHANNEL;
    typedef void *SENTENCE;
    typedef void (*Cb) (char *, size_t);

    CHANNEL NMEA_API createChannel (ConnectionType type, Cb cb);
    void NMEA_API deleteChannel (CHANNEL);
    void NMEA_API configureChannel (CHANNEL channel, int port, int baud, const char *params);
    void NMEA_API configureChannel (CHANNEL chn, int port, int baud, int byteSize, int parity, int stopBits);
    bool NMEA_API connectChannel (CHANNEL);
    void NMEA_API disconnectChannel (CHANNEL);
    bool NMEA_API isChannelConnected (CHANNEL);
    void NMEA_API activateChannel (CHANNEL, bool);
}
