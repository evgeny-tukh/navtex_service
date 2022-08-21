#include <stdlib.h>
#include <string>
#include <cstdint>
#include <time.h>
#include <Windows.h>
#include "../tools/tools.h"
#include "message.h"
#include "../nmea/nmea.h"
#include "db.h"
#include "parsing.h"
#include "navtex.h"

extern MsgCb msgAddCb;
extern MsgCb2 msgAddCb2;
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
    auto msgInfo = MsgInfo::parseNativeMsg (source, false);
    if (msgInfo) {
        auto msgID = addMessage (db, msgInfo);
        std::wstring idString = std::to_wstring (msgID);
        if (msgAddCb) {
            msgAddCb ((wchar_t *) idString.c_str ());
        }
        if (msgAddCb2) {
            wchar_t text [1000];
            MultiByteToWideChar (CP_ACP, 0, msgInfo->msg.c_str (), -1, text, 1000);
            msgAddCb2 ((wchar_t *) idString.c_str (), text);
        }
        delete msgInfo;
    }
}

bool parseCrNrx (nmea::SENTENCE sentence) {
    if (nmea::getSentenceFieldsNumber (sentence) > 13) {
        auto expectedLines = nmea::getSentenceFieldAsIntAt (sentence, 1);
        auto lineNo = nmea::getSentenceFieldAsIntAt (sentence, 2);
        auto seqNo = nmea::isSentenceFieldOmitted (sentence, 3) ? 0 : nmea::getSentenceFieldAsIntAt (sentence, 3);
        auto prefix = nmea::getSentenceFieldAt (sentence, 4);
        std::string text;

        if (lineNo == 1 && nmea::getSentenceFieldAsCharAt (sentence, 12) != 'A') return false;

        translateControlCharacters (nmea::getSentenceFieldAt (sentence, 13).c_str (), text);

        if (lineNo == 1) {
            text.insert (text.begin (), prefix.begin (), prefix.end ());
        }

        auto& msg = checkMessage (seqNo, expectedLines, lineNo, text.c_str ());

        if (msg.completed ()) {
            processNativeMsg (msg.composeText ().c_str ());
            dropMessage (seqNo);
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
            dropMessage (seqNo);
        }
    }

    return true;
}

bool onNewSentence (nmea::SENTENCE sentence) {
    auto type = nmea::getSentenceType (sentence);
    if (type.empty ()) {
        return false;
    }
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

MsgInfo *MsgInfo::parseNativeMsg (const char *source, bool useHeaderAndTail) {
    std::string msgText;
    if (useHeaderAndTail) {
        const char *header = strstr (source, MSG_HEAD);
        const char *tail = header ? strstr (header + MSG_HEAD_SIZE, MSG_TAIL) : nullptr;
        if (!tail) return nullptr;
        msgText.insert (msgText.begin (), header + MSG_HEAD_SIZE, tail);
    } else {
        msgText = source;
    }
    while (!msgText.empty () && msgText.front () <= ' ' || msgText.front () == '*') msgText.erase (msgText.front ());
    if (!isCharCodeValid (msgText.front ()) || !isCharCodeValid (msgText [1]) || !isdigit (msgText [2]) || !isdigit (msgText [3])) {
        return new MsgInfo (MSG_SUSPICIOUS, *source, atoi (source + 2), time (nullptr), source + 4);
    } else if (isSubjectEnabled (msgText [1])) {
        return new MsgInfo (msgText [1], msgText.front (), atoi (msgText.c_str () + 2), time (nullptr), msgText.c_str () + 4);
    }
    return nullptr;
}

