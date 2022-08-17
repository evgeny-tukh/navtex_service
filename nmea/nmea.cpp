#include <stdlib.h>
#include <Windows.h>
#include <Shlwapi.h>

#include "nmea.h"
#include "channel.h"
#include "serial_reader.h"
#include "../tools/tools.h"

BOOL WINAPI DllMain (HINSTANCE dll, unsigned long reason, void *) {
    return TRUE;
}

namespace nmea {
    CHANNEL NMEA_API createChannel (ConnectionType type, Cb cb) {
        return new Channel (type, cb);
    }
    void NMEA_API deleteChannel (CHANNEL channel) {
        if (channel) {
            delete (Channel *) channel;
        }
    }
    void NMEA_API configureChannel (CHANNEL chn, int port, int baud, const char *params) {
        Channel *channel = (Channel *) chn;
        auto reader = channel->getReader ();
        if (reader && reader->getType () == ConnectionType::SERIAL) {
            bool connected = reader->isConnected ();
            if (connected) reader->disconnect ();
            SerialReaderCfg *cfg = (SerialReaderCfg *) reader->getConfig ();
            cfg->baud = baud;
            cfg->port = port;
            cfg->byteSize = params [0] - '0';
            cfg->parity = tools::char2parity (params [1]);
            cfg->stopBits = tools::char2stopBits (params [2]);
            if (connected) reader->connect ();
        }
    }
    void NMEA_API configureChannel (CHANNEL chn, int port, int baud, int byteSize, int parity, int stopBits) {
        Channel *channel = (Channel *) chn;
        auto reader = channel->getReader ();
        if (reader && reader->getType () == ConnectionType::SERIAL) {
            bool connected = reader->isConnected ();
            if (connected) reader->disconnect ();
            SerialReaderCfg *cfg = (SerialReaderCfg *) reader->getConfig ();
            cfg->baud = baud;
            cfg->port = port;
            cfg->byteSize = byteSize;
            cfg->parity = parity;
            cfg->stopBits = stopBits;
            if (connected) reader->connect ();
        }
    }
    void NMEA_API activateChannel (CHANNEL chn, bool flag) {
        if (chn) {
            Channel *channel = (Channel *) chn;
            channel->activate (flag);
        }
    }
    bool NMEA_API connectChannel (CHANNEL chn) {
        if (chn) {
            Channel *channel = (Channel *) chn;
            auto reader = channel->getReader ();
            return reader && reader->connect ();
        }
        return false;
    }
    void NMEA_API disconnectChannel (CHANNEL chn) {
        if (chn) {
            Channel *channel = (Channel *) chn;
            auto reader = channel->getReader ();
            if (reader) reader->disconnect ();
        }
    }
    bool NMEA_API isChannelConnected (CHANNEL chn) {
        if (chn) {
            Channel *channel = (Channel *) chn;
            auto reader = channel->getReader ();
            return reader && reader->isConnected ();
        }
        return false;
    }
}
