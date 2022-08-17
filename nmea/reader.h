#pragma once

#include <stdlib.h>
#include <vector>
#include <string>
#include <cstdint>

#include "nmea.h"

struct ReaderCfg {
    size_t size;
};

class Reader {
    protected:
        ReaderCfg *cfg;
        bool connected;

    public:
        typedef std::vector<uint8_t> Buffer;

        Reader (): connected (false), cfg (nullptr) {}
        virtual ~Reader () {
            if (cfg) free (cfg);
        }
        virtual nmea::ConnectionType getType () { return nmea::ConnectionType::UNKNOWN; }
        virtual bool connect () {
            connected = true;
            return true;
        }
        virtual bool disconnect () {
            connected = false;
            return true;
        }
        bool isConnected () { return connected; }
        virtual size_t dataAvailable () { return 0; }
        virtual bool getAvailableData (Buffer& result) {
            result.clear ();
            return true;
        }
        virtual void configure (ReaderCfg *_cfg) {
            if (!cfg) cfg = (ReaderCfg *) malloc (_cfg->size);
            memcpy (cfg, _cfg, _cfg->size);
        }
        ReaderCfg *getConfig () { return cfg; }
};
