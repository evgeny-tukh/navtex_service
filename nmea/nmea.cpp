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
    if (reason == DLL_PROCESS_ATTACH) {
        WSADATA data;
        WSAStartup (0x202, & data);
    }
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
    bool NMEA_API setConnectionType (CHANNEL channel, ConnectionType type) {
        if (channel) {
            ((Channel *) channel)->setType (type); return true;
        }
        return false;
    }
    ConnectionType NMEA_API getConnectionType (CHANNEL channel) {
        return channel ? ((Channel *) channel)->getType () : nmea::ConnectionType::UNKNOWN;
    }
    void NMEA_API deleteChannel (CHANNEL channel) {
        if (channel) {
            delete (Channel *) channel;
        }
    }
    void NMEA_API configureChannel (CHANNEL chn, uint32_t inPort, uint32_t outPort, const char *bindAddr) {
        Channel *channel = (Channel *) chn;
        auto reader = channel->getReader (nmea::ConnectionType::UDP);
        if (reader) {
            bool connected = reader->isConnected ();
            if (connected) reader->disconnect ();
            UdpReaderCfg *cfg = (UdpReaderCfg *) reader->getConfig ();
            cfg->inPort = inPort;
            cfg->outPort = outPort;
            cfg->bindTo = bindAddr;
            if (connected) reader->connect ();
        }
    }
    void NMEA_API configureChannel (CHANNEL chn, uint32_t inPort, uint32_t outPort, IN_ADDR bindAddr) {
        Channel *channel = (Channel *) chn;
        auto reader = channel->getReader (nmea::ConnectionType::UDP);
        if (reader) {
            bool connected = reader->isConnected ();
            if (connected) reader->disconnect ();
            UdpReaderCfg *cfg = (UdpReaderCfg *) reader->getConfig ();
            cfg->inPort = inPort;
            cfg->outPort = outPort;
            cfg->bindTo = bindAddr.S_un.S_addr == 0 ? "" : inet_ntoa (bindAddr);
            if (connected) reader->connect ();
        }
    }

    void NMEA_API configureChannel (CHANNEL chn, int port, int baud, const char *params) {
        Channel *channel = (Channel *) chn;
        auto reader = channel->getReader (nmea::ConnectionType::SERIAL);
        if (reader) {
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
        auto reader = channel->getReader (nmea::ConnectionType::SERIAL);
        if (reader) {
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
            auto reader = channel->getReader (getConnectionType (chn));
            return reader && reader->connect ();
        }
        return false;
    }
    void NMEA_API disconnectChannel (CHANNEL chn) {
        if (chn) {
            Channel *channel = (Channel *) chn;
            auto reader = channel->getReader (getConnectionType (chn));
            if (reader) reader->disconnect ();
        }
    }
    bool NMEA_API isChannelConnected (CHANNEL chn) {
        if (chn) {
            Channel *channel = (Channel *) chn;
            auto reader = channel->getReader (getConnectionType (chn));
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
    bool NMEA_API isAnySentenceFieldOmitted (SENTENCE snt, size_t index1, size_t index2) {
        for (auto i = index1; i <= index2; ++ i) {
            if (((Sentence *) snt)->omitted (i)) return true;
        }
        return false;
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

        if (sentence->isProprietary ()) return firstField.length () > 6 ? firstField.substr (4, 3) : "";
        return firstField.length () > 5 ? firstField.substr (3, 3) : "";
    }
    std::string NMEA_API getSentenceTalkerID (SENTENCE snt) {
        auto sentence = (Sentence *) snt;
        auto firstField = sentence->getAt (0);

        if (sentence->isProprietary ()) return firstField.substr (2, 2);
        return firstField.substr (1, 2);
    }
    bool NMEA_API writeToMedia (CHANNEL chn, ConnectionType type, char *data) {
        Channel *channel = (Channel *) chn;
        Reader *reader = channel->getReader (type);
        if (reader) return reader->write (data);
        return false;
    }
}
