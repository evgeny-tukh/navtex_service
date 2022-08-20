#include <map>
#include <vector>
#include <cstdint>
#include <string>
#include "../nmea/nmea.h"

struct Message {
    uint32_t seqNo, numOfExpectedSentences, numOfReceivedSentences, lastReceived;
    std::string text;
    std::vector<bool> isReceived;
    std::vector<std::string> parts;

    Message (uint32_t _seqNo, uint32_t _numOfExpectedSentences, uint32_t _sentenceNum, const char *_text);
    void onReceived (uint32_t _sentenceNum, const char *_text);
    bool completed ();
    std::string composeText ();

    static Message noMessage;
};

Message& checkMessage (uint32_t seqNo, uint32_t numOfExpectedSentences, uint32_t sentenceNum, const char *text);