#include <map>
#include "message.h"
#include "../nmea/nmea.h"
#include "parsing.h"

std::map<uint8_t, Message> messages;

Message *findMessage (uint8_t seqNo) {
    auto pos = messages.find (seqNo);
    if (pos == messages.end ()) return nullptr;
    return & pos->second;
}

void dropMessage (uint32_t seqNo) {
    auto pos = messages.find (seqNo);
    if (pos != messages.end ()) messages.erase (pos);
}

Message *checkMessage (uint32_t seqNo, uint32_t numOfExpectedSentences, uint32_t sentenceNum, const char *text, time_t whenSent, const char *lineID) {
    auto msg = findMessage (seqNo);
    if (msg) {
        msg->onReceived (sentenceNum, text);
        if (!msg->whenSent && whenSent > 0) msg->whenSent = whenSent;
    } else if (sentenceNum == 1) {
        msg = & messages.emplace (std::pair<uint8_t, Message> (seqNo, Message (seqNo, numOfExpectedSentences, sentenceNum, text, whenSent, lineID))).first->second;
    }
    return msg;
}

Message::Message (uint32_t _seqNo, uint32_t _numOfExpectedSentences, uint32_t _sentenceNum, const char *_text, time_t _whenSent, const char *_lineID):
    seqNo (_seqNo),
    numOfExpectedSentences (_numOfExpectedSentences),
    numOfReceivedSentences (1),
    lastReceived (_sentenceNum),
    whenSent (_whenSent),
    lineID (_lineID) {
    isReceived.insert (isReceived.begin (), numOfExpectedSentences, false);
    parts.clear ();
    parts.resize (numOfExpectedSentences);
    onReceived (_sentenceNum, _text);
}

void Message::onReceived (uint32_t _sentenceNum, const char *_text) {
    isReceived [_sentenceNum-1] = true;
    lastReceived = _sentenceNum;
    parts [_sentenceNum-1] = _text;
}

bool Message::completed () {
    for (auto recvd: isReceived) {
        if (!recvd) return false;
    }
    return true;
}

std::string Message::composeText () {
    std::string result;
    for (auto& part: parts) result += part;
    return result;
}


