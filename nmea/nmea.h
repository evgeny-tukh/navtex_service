#pragma once

#include <string>

#include "defs.h"

namespace nmea {
    enum ConnectionType { UNKNOWN = 0, SERIAL, UDP, };
    typedef void *CHANNEL;
    typedef void *SENTENCE;
    typedef void (*Cb) (char *, size_t);
    typedef void (*SentenceCb) (SENTENCE);

    CHANNEL NMEA_API createChannel (ConnectionType type, Cb cb);
    void NMEA_API deleteChannel (CHANNEL);
    void NMEA_API configureChannel (CHANNEL channel, int port, int baud, const char *params);
    void NMEA_API configureChannel (CHANNEL chn, int port, int baud, int byteSize, int parity, int stopBits);
    bool NMEA_API connectChannel (CHANNEL);
    void NMEA_API disconnectChannel (CHANNEL);
    bool NMEA_API isChannelConnected (CHANNEL);
    void NMEA_API activateChannel (CHANNEL, bool);
    void NMEA_API extractAndParseAll (char *, SentenceCb);
    
    size_t NMEA_API getSentenceFieldsNumber (SENTENCE);
    std::string NMEA_API getSentenceFieldAt (SENTENCE, size_t);
    int NMEA_API getSentenceFieldAsIntAt (SENTENCE, size_t);
    double NMEA_API getSentenceFieldAsDoubleAt (SENTENCE, size_t);
    char NMEA_API getSentenceFieldAsCharAt (SENTENCE, size_t);
    bool NMEA_API isSentenceFieldOmitted (SENTENCE, size_t);
    bool NMEA_API isAnySentenceFieldOmitted (SENTENCE snt, size_t index1, size_t index2);
    std::string NMEA_API getSentenceType (SENTENCE);
    std::string NMEA_API getSentenceTalkerID (SENTENCE);
    bool NMEA_API isSentenceSixBitEncoded (SENTENCE);
}
