#include <stdlib.h>
#include <string>
#include <cstdint>
#include "../tools/tools.h"
#include "message.h"
#include "../nmea/nmea.h"
#include "db.h"
#include "parsing.h"

extern struct sqlite3 *db;

bool translateControlCharacters (const char *source, std::string& dest) {
    dest.clear ();

    if (!source || !*source) return false;

    for (; *source; ++ source) {
        if (*source == '^') {
            uint8_t code = tools::twoChars2hex ((char *) source + 1);
            
            switch (code) {
                case 10: dest += "\r\n"; break;
                default: dest += (char) (code & 255);
            }

            source += 2;
        } else {
            dest += *source;
        }
    }

    return true;
}

void processNativeMsg (const char *source) {
    auto msgInfo = MsgInfo::parseNativeMsg (source);
    if (msgInfo) {
        addMessage (db, msgInfo);
        delete msgInfo;
    }
}

bool parseCrNrx (nmea::SENTENCE sentence) {
    if (nmea::getSentenceFieldsNumber (sentence) > 13) {
        if (nmea::getSentenceFieldAsCharAt (sentence, 12) != 'A') return false;

        auto expectedLines = nmea::getSentenceFieldAsIntAt (sentence, 1);
        auto lineNo = nmea::getSentenceFieldAsIntAt (sentence, 2);
        auto seqNo = nmea::isSentenceFieldOmitted (sentence, 3) ? 0 : nmea::getSentenceFieldAsIntAt (sentence, 3);
        std::string text;

        translateControlCharacters (nmea::getSentenceFieldAt (sentence, 13).c_str (), text);

        auto& msg = checkMessage (seqNo, expectedLines, lineNo, text.c_str ());

        if (msg.completed ()) {
            processNativeMsg (msg.composeText ().c_str ());
        }
    }
    return true;
}

bool parseNxNrx (nmea::SENTENCE sentence) {
    if (nmea::getSentenceFieldsNumber (sentence) > 10) {
        auto expectedLines = nmea::getSentenceFieldAsIntAt (sentence, 3);
        auto lineNo = nmea::getSentenceFieldAsIntAt (sentence, 2);
        auto seqNo = nmea::isSentenceFieldOmitted (sentence, 1) ? 0 : nmea::getSentenceFieldAsIntAt (sentence, 1);
        std::string text;

        translateControlCharacters (nmea::getSentenceFieldAt (sentence, 10).c_str (), text);

        auto& msg = checkMessage (seqNo, expectedLines, lineNo, text.c_str ());

        if (msg.completed ()) {
            processNativeMsg (msg.composeText ().c_str ());
        }
    }

    return true;
}

bool onNewSentence (nmea::SENTENCE sentence) {
    auto type = nmea::getSentenceType (sentence);
    auto talkerID = nmea::getSentenceTalkerID (sentence);
    if (type.compare ("NRX") == 0) {
        if (talkerID.compare ("NX") == 0) {
            return parseNxNrx (sentence);
        } else if (talkerID.compare ("CR") == 0) {
            return parseCrNrx (sentence);
        }
    }
    return false;
}

MsgInfo *MsgInfo::parseNativeMsg (const char *source) {
    return nullptr;
}

