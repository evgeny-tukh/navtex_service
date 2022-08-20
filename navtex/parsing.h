#pragma once

#include <string>

#include "../nmea/nmea.h"

bool translateControlCharacters (const char *source, std::string& dest);
bool parseNxNrx (nmea::SENTENCE sentence);
bool parseCrNrx (nmea::SENTENCE sentence);
bool onNewSentence (nmea::SENTENCE sentence);

struct MsgInfo {
    char subject;
    char station;
    time_t receivedAt;
    std::string msg;
    std::vector<std::pair<double, double>> positions;

    MsgInfo (
        char _subject,
        char _station,
        time_t _receivedAt,
        const char *_msg
    ): subject (_subject), station (_station), receivedAt (_receivedAt), msg (_msg) {
    }

    void addPosition (double lat, double lon) {
        auto& newPos = positions.emplace_back ();
        newPos.first = lat;
        newPos.second = lon;
    }

    static MsgInfo *parseNativeMsg (const char *source);
};

