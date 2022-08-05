#pragma once

#include "../sqlite/sqlite3.h"

const int CUR_VERSION = 1;

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
};

void checkDb (sqlite3 *db);
void addMessage (sqlite3 *db, MsgInfo *msg);
