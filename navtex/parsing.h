#pragma once

#include <string>
#include <tuple>

#include "../nmea/nmea.h"

typedef std::tuple<bool, double, double> OptionalPos;
typedef std::tuple<char *, double, double> PosExtractResult;
typedef std::pair<double, size_t> CoordExtractResult;
typedef std::tuple<double, size_t, bool> CoordExtractStatus;
typedef std::tuple<bool, char, char, uint8_t> LineID;

bool translateControlCharacters (const char *source, std::string& dest);
bool parseNxNrx (nmea::SENTENCE sentence);
bool parseCrNrx (nmea::SENTENCE sentence);
bool onNewSentence (nmea::SENTENCE sentence);
void extractPositions (struct MsgInfo *msgInfo, char *source);
LineID parseLineID (const char *source);

struct MsgInfo {
    char subject;
    char station;
    uint32_t seqNo;
    time_t receivedAt, sentAt;
    std::string msg;
    std::vector<std::pair<double, double>> positions;

    MsgInfo (
        char _subject,
        char _station,
        uint32_t _seqNo,
        time_t _receivedAt,
        time_t _sentAt,
        const char *_msg
    ): subject (_subject), station (_station), receivedAt (_receivedAt), msg (_msg), seqNo (_seqNo), sentAt (_sentAt) {
        extractPositions (this, (char *) _msg);
    }

    void addPosition (double lat, double lon) {
        auto& newPos = positions.emplace_back ();
        newPos.first = lat;
        newPos.second = lon;
    }

    static MsgInfo *parseNativeMsg (const char *source, time_t sentAt, bool useHeaderAndTail, const char *msgID);
};

bool isCharCodeValid (char subject);
bool isSubjectEnabled (char subject);
