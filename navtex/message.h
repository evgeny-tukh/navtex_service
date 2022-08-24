#include <map>
#include <vector>
#include <cstdint>
#include <string>
#include "../nmea/nmea.h"
#include "parsing.h"

struct Message {
    uint32_t seqNo, numOfExpectedSentences, numOfReceivedSentences, lastReceived;
    std::string text, lineID;
    std::vector<bool> isReceived;
    std::vector<std::string> parts;
    time_t whenSent;

    Message (uint32_t _seqNo, uint32_t _numOfExpectedSentences, uint32_t _sentenceNum, const char *_text, time_t _whenSent, const char *_lineID);
    void onReceived (uint32_t _sentenceNum, const char *_text);
    bool completed ();
    std::string composeText ();

    static Message noMessage;
};

Message *checkMessage (uint32_t seqNo, uint32_t numOfExpectedSentences, uint32_t sentenceNum, const char *text, time_t whenSent, const char *lineID);
void dropMessage (uint32_t seqNo);