#include <stdlib.h>
#include <Windows.h>
#include <Shlwapi.h>

#include <optional>
#include <functional>

#include "nmea.h"
#include "channel.h"
#include "sentence.h"
#include "serial_reader.h"
#include "../tools/tools.h"

BOOL WINAPI DllMain (HINSTANCE dll, unsigned long reason, void *) {
    return TRUE;
}

namespace nmea {
    void NMEA_API extractAndParseAll (char *source, SentenceCb cb) {
        Sentence sentence;
        sentence.extractAndParseAll (source, [&sentence, cb] (void *) {
            cb (& sentence);
        });
    }
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
    size_t NMEA_API getSentenceFieldsNumber (SENTENCE snt) {
        return ((Sentence *) snt)->numOfFields ();
    }
    std::string NMEA_API getSentenceFieldAt (SENTENCE snt, size_t index) {
        return ((Sentence *) snt)->getAt (index);
    }
    int NMEA_API getSentenceFieldAsIntAt (SENTENCE snt, size_t index) {
        return ((Sentence *) snt)->getAsIntAt (index);
    }
    double NMEA_API getSentenceFieldAsDoubleAt (SENTENCE snt, size_t index) {
        return ((Sentence *) snt)->getAsDoubleAt (index);
    }
    char NMEA_API getSentenceFieldAsCharAt (SENTENCE snt, size_t index) {
        return ((Sentence *) snt)->getAsCharAt (index);
    }
    bool NMEA_API isSentenceFieldOmitted (SENTENCE snt, size_t index) {
        return ((Sentence *) snt)->omitted (index);
    }
    bool NMEA_API isSentenceSixBitEncoded (SENTENCE snt) {
        return ((Sentence *) snt)->isSixBitEncoded ();
    }
    bool NMEA_API isSentenceProprietary (SENTENCE snt) {
        return ((Sentence *) snt)->isProprietary ();
    }
    std::string NMEA_API getSentenceType (SENTENCE snt) {
        auto sentence = (Sentence *) snt;
        auto firstField = sentence->getAt (0);

        if (sentence->isProprietary ()) return firstField.substr (4, 3);
        return firstField.substr (3, 3);
    }
    std::string NMEA_API getSentenceTalkerID (SENTENCE snt) {
        auto sentence = (Sentence *) snt;
        auto firstField = sentence->getAt (0);

        if (sentence->isProprietary ()) return firstField.substr (2, 2);
        return firstField.substr (1, 2);
    }
}
