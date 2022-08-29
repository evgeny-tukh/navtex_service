#include <stdlib.h>
#include <string>
#include <cstdint>
#include <time.h>
#include <tuple>
#include <string>
#include <Windows.h>
#include "../tools/tools.h"
#include "message.h"
#include "../nmea/nmea.h"
#include "db.h"
#include "parsing.h"
#include "navtex.h"

extern MsgCb msgAddCb;
extern MsgCb2 msgAddCb2;

typedef std::tuple<bool, double, double> OptionalPos;
typedef std::tuple<char *, double, double> PosExtractResult;
typedef std::pair<double, size_t> CoordExtractResult;
typedef std::tuple<double, size_t, bool> CoordExtractStatus;
typedef std::tuple<bool, char, char, uint8_t> LineID;
const OptionalPos NO_POSITION (false, 0.0, 0.0);
const PosExtractResult NO_MORE_POS (nullptr, 0.0, 0.0);
const CoordExtractResult INVALID_COORD (-10000.0, 0);

inline bool _isdigit (char ch) { return ch >= '0' && ch <= '9'; }

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

void processNativeMsg (const char *source, const char *msgID, time_t whenSent) {
    auto msgInfo = MsgInfo::parseNativeMsg (source, whenSent, false, msgID);
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

time_t extractUtc (char *source) {
    char *signaturePos = strstr (source, "UTC");

    if (!signaturePos) return -1;
    if (*(signaturePos - 1) != ' ') return -1;
    for (size_t i = 0; i < 6; ++ i) {
        if (!_isdigit (*(signaturePos - (i + 2)))) return -1;
    }
    time_t timePart = tools::hhmmss2time (signaturePos - 7);
    time_t timestamp = time (nullptr);
    tm *now = gmtime (& timestamp);
    if (signaturePos [3] == ' ') {
        int day = 0;
        int month = tools::getMonthNo (signaturePos + 4);
        int year = now->tm_year + 1900;

        if (month > 0 && month <= 12 && signaturePos [7] == ' ') {
            if (signaturePos [8] == '*') {
                day = tools::twoChars2int (signaturePos + 9);
            } else if (_isdigit (signaturePos [9])) {
                day = tools::twoChars2int (signaturePos + 8);
            }
            if (day > 0 && day <= tools::getMonthLen (month)) {
                return tools::ddmmyyyy2time (day, month, year) + timePart;
            }
        }
    }
    return -1;
}

LineID parseLineID (const char *source) {
    if (!source ||strlen (source) < 4) return LineID (false, 0, 0, 0);
    char subject = isCharCodeValid (source [1]) ? source [1] : MSG_SUSPICIOUS;
    char station = isCharCodeValid (source [0]) ? source [0] : 0;
    uint8_t serialNo = tools::twoChars2int ((char *) source + 2);
    return LineID (station != 0, subject, station, serialNo);
}

bool parseCrNrx (nmea::SENTENCE sentence) {
    if (nmea::getSentenceFieldsNumber (sentence) > 13) {
        auto expectedLines = nmea::getSentenceFieldAsIntAt (sentence, 1);
        auto lineNo = nmea::getSentenceFieldAsIntAt (sentence, 2);
        auto seqNo = nmea::isSentenceFieldOmitted (sentence, 3) ? 0 : nmea::getSentenceFieldAsIntAt (sentence, 3);
        auto prefix = nmea::getSentenceFieldAt (sentence, 4);
        std::string lineID = nmea::isSentenceFieldOmitted (sentence, 4) ? "" : nmea::getSentenceFieldAt (sentence, 4);
        std::string text;

        if (lineNo == 1 && nmea::getSentenceFieldAsCharAt (sentence, 12) != 'A') return false;

        translateControlCharacters (nmea::getSentenceFieldAt (sentence, 13).c_str (), text);

        if (lineNo == 1) {
            text.insert (text.begin (), prefix.begin (), prefix.end ());
        }

        auto msg = checkMessage (seqNo, expectedLines, lineNo, text.c_str (), extractUtc (text.data ()), lineID.c_str ());

        if (msg->completed ()) {
            processNativeMsg (msg->composeText ().c_str (), lineID.c_str ());
            dropMessage (seqNo);
        }
    }
    return true;
}

bool parseNxNrx (nmea::SENTENCE sentence) {
    if (nmea::getSentenceFieldsNumber (sentence) > 10) {
        auto expectedLines = nmea::getSentenceFieldAsIntAt (sentence, 3);
        auto lineNo = nmea::getSentenceFieldAsIntAt (sentence, 2);
        std::string lineID = nmea::isSentenceFieldOmitted (sentence, 1) ? "" : nmea::getSentenceFieldAt (sentence, 1);
        auto seqNo = lineID.length () > 3 ? atoi (nmea::getSentenceFieldAt (sentence, 1).substr (2).c_str ()) : 0;
        std::string text;
        time_t whenSent = 0;

        if (lineNo == 1 && !nmea::isAnySentenceFieldOmitted (sentence, 5, 8)) {
            auto day = nmea::getSentenceFieldAsIntAt (sentence, 5);
            auto month = nmea::getSentenceFieldAsIntAt (sentence, 6);
            auto year = nmea::getSentenceFieldAsIntAt (sentence, 7);
            auto utc = tools::hhmmss2time (nmea::getSentenceFieldAt (sentence, 8).data ());
            whenSent = tools::ddmmyyyy2time (day, month, year) + utc;
        }
        translateControlCharacters (nmea::getSentenceFieldAt (sentence, 10).c_str (), text);

        if (lineNo == 1 && !whenSent) {
            whenSent = extractUtc (text.data ());
        }

        auto msg = checkMessage (seqNo, expectedLines, lineNo, text.c_str (), whenSent, lineID.c_str ());

        if (msg && msg->completed ()) {
            processNativeMsg (msg->composeText ().c_str (), msg->lineID.c_str (), msg->whenSent);
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

inline bool _isdigitOrAsterisk (char ch) {
    return ch >= '0' && ch <= '9' || ch == '*';
}

inline bool isMinusOrAsterisk (char ch) {
    return ch == '-' || ch == '*';
}

inline bool isNorthSouthOrAsterisk (char ch) {
    return ch == 'N' || ch == 'S' || ch == '*';
}

inline bool isEastWestOrAsterisk (char ch) {
    return ch == 'E' || ch == 'W' || ch == '*';
}

CoordExtractResult queryLatValue_D_M_S (char *source) {
    bool invalidData = false;
    double lat;

    if (!source) return INVALID_COORD;

    if (
        _isdigitOrAsterisk (source [0]) && _isdigitOrAsterisk (source [1]) &&
        _isdigitOrAsterisk (source [3]) && _isdigitOrAsterisk (source [4]) &&
        _isdigitOrAsterisk (source [6]) && _isdigitOrAsterisk (source [7]) &&
        isMinusOrAsterisk (source [2]) && isMinusOrAsterisk (source [5]) &&
        isNorthSouthOrAsterisk (source [8])
    ) {
        for (auto i = 0; i < 9; ++ i) {
            if (source [i] == '*') {
                invalidData = true; break;
            }
        }

        if (invalidData) return CoordExtractResult (-10000.0, 9);

        lat = (double) atoi (source) + (double) atoi (source + 3) / 60.0 + (double) atoi (source + 6) / 3600.0;

        return CoordExtractResult ((source [8] == 'S') ? -lat : lat, 9);
    } else {
        return INVALID_COORD;
    }
}

CoordExtractResult queryLonValue_D_M_S (char *source) {
    bool invalidData = false;
    double lon;

    if (!source) return INVALID_COORD;

    if (
        _isdigitOrAsterisk (source [0]) && _isdigitOrAsterisk (source [1]) && _isdigitOrAsterisk (source [2]) &&
        _isdigitOrAsterisk (source [4]) && _isdigitOrAsterisk (source [5]) &&
        _isdigitOrAsterisk (source [7]) && _isdigitOrAsterisk (source [8]) &&
        isMinusOrAsterisk (source [3]) && isMinusOrAsterisk (source [6]) &&
        isEastWestOrAsterisk (source [9])
    ) {
        for (auto i = 0; i < 10; ++ i) {
            if (source [i] == '*') {
                invalidData = true; break;
            }
        }

        if (invalidData) return CoordExtractResult (-10000.0, 10);

        lon = (double) atoi (source) + (double) atoi (source + 4) / 60.0 + (double) atoi (source + 7) / 3600.0;

        return CoordExtractResult ((source [8] == 'W') ? -lon : lon, 10);
    }
    else {
        return INVALID_COORD;
    }
}

CoordExtractStatus queryLatValue (char *source)
{
    auto [lat, size] = queryLatValue_D_M_S (source);

    if (size > 0 && lat > -90.0 && lat < 90.0) return CoordExtractStatus (lat, size, true);

    if (
        // ddmm.sS case
        _isdigitOrAsterisk (source [0]) &&
        _isdigitOrAsterisk (source [1]) &&
        _isdigitOrAsterisk (source [2]) &&
        _isdigitOrAsterisk (source [3]) &&
        _isdigitOrAsterisk (source [5]) &&
        source [4] == '.' &&
        (source [6] == 'S' || source [6] == 'N')
    ) {
        lat = (double) tools::twoChars2int (source) + atof (source + 2) / 60.0;
        return CoordExtractStatus (source [6] == 'S' ? -lat : lat, 7, true);
    } else if (
        _isdigitOrAsterisk (source [0]) &&
        _isdigitOrAsterisk (source [1]) &&
        (source [2] == '-' || source [2] == ' ' || source [2] == '*') &&
        _isdigitOrAsterisk (source [3])
    ) {
        bool nsFound, invalidData;
        int deg = atoi (source);
        char hemisphere;
        size_t i;

        invalidData = source [0] == '*' || source [1] == '*' || source [2] == '*' || source [3] == '*';

        for (i = 4, nsFound = false; !nsFound && source [i]; ++ i) {
            switch (source [i]) {
                case '\r': case '\n':
                    if (nsFound) break;
                case '*':
                    invalidData = true; break;
                case 'N': case 'S':
                    hemisphere = source [i];
                    nsFound = true;
                    break;
                case ',':
                    source [i] = '.'; break;
                default:
                    if (!_isdigit (source [i])) source [i--] = '\0';
            }
        }

        if (invalidData) return CoordExtractStatus (-10000.0, i, true);

        lat = nsFound ? (double) deg + atof (source + 3) / 60.0 : -10000.0;

        return CoordExtractStatus (nsFound && hemisphere == 'S' ? -lat : lat, i, true);
    }

    return CoordExtractStatus (-10000.0, 0, false);
}

CoordExtractStatus queryLonValue (char *source)
{
    auto [lon, size] = queryLonValue_D_M_S (source);

    if (size > 0 && lon > -180.0 && lon < 180.0) return CoordExtractStatus (lon, size, true);

    if (
        // dddmm.sS case
        _isdigitOrAsterisk (source [0]) &&
        _isdigitOrAsterisk (source [1]) &&
        _isdigitOrAsterisk (source [2]) &&
        _isdigitOrAsterisk (source [3]) &&
        _isdigitOrAsterisk (source [4]) &&
        _isdigitOrAsterisk (source [6]) &&
        source [5] == '.' &&
        (source [7] == 'E' || source [7] == 'W')
    ) {
        lon = (double) tools::twoChars2int (source) + atof (source + 3) / 60.0;
        return CoordExtractStatus (source [6] == 'W' ? -lon : lon, 8, true);
    } else if (
        _isdigitOrAsterisk (source [0]) &&
        _isdigitOrAsterisk (source [1]) &&
        _isdigitOrAsterisk (source [2]) &&
        (source [3] == '-' || source [3] == ' ' || source [3] == '*') &&
        _isdigitOrAsterisk (source [4])
    ) {
        bool ewFound, invalidData, commaFound;
        int deg = atoi (source);
        size_t i;
        char hemisphere;

        invalidData = source [0] == '*' || source [1] == '*' || source [2] == '*' || source [3] == '*' || source [4] == '*';

        for (i = 5, ewFound = commaFound = false; !ewFound && source [i]; ++ i) {
            switch (source [i]) {
                case '*':
                    invalidData = true; break;
                case 'E':
                case 'W':
                    hemisphere = source [i];
                    ewFound = true;
                    break;
                case ',':
                    commaFound = true;
                    source [i] = '.'; break;
                default:
                    if (!_isdigit (source [i])) source [i] = '\0';
            }
        }

        if (invalidData) return CoordExtractStatus (-10000.0, i, true);

        double lon = ewFound ? (double) deg + atof (source + 4) / 60.0 : -10000.0;
        return CoordExtractStatus (ewFound && hemisphere == 'W' ? -lon : lon, i, true);
    }

    return CoordExtractStatus (-10000.0, 0, false);
}

void skipDate (char *& pos) {
    // Form yyyy-mm-dd
    //      0123456789
    if (
        pos [0] == '2' && pos [1] == '0' && _isdigit (pos [2]) && _isdigit (pos [3]) &&
        (pos [4] == ' ' || pos [4] == '-' || pos [4] == '/' || pos [4] == '.') &&
        (pos [7] == ' ' || pos [7] == '-' || pos [7] == '/' || pos [7] == '.') &&
        pos [4] == pos [7] /* the separator must be same! */ &&
        _isdigit (pos [5]) && _isdigit (pos [6]) && _isdigit (pos [8]) && _isdigit (pos [9]) &&
        (pos [10] == ' ' || pos [10] == '\t' || pos [10] == '\r' || pos [10] == '\n') /* any separatore after date */
    ) {
        pos += 10; return;
    }

    // Form yyyy-m-dd or yyyy-m-d (months jan-sep and days 1..9)
    //      0123456789
    if (
        pos [0] == '2' && pos [1] == '0' && _isdigit (pos [2]) && _isdigit (pos [3]) &&
        (pos [4] == ' ' || pos [4] == '-' || pos [4] == '/' || pos [4] == '.') &&
        (pos [6] == ' ' || pos [6] == '-' || pos [6] == '/' || pos [6] == '.') &&
        pos [4] == pos [6] /* the separator must be same! */ &&
        _isdigit (pos [5]) && _isdigit (pos [7])
    ) {
        pos += 10; return;
    }

    // Form dd-mm-yyyy
    //      0123456789
    if (
        pos [6] == '2' && pos [7] == '0' && _isdigit (pos [8]) && _isdigit (pos [9]) &&
        (pos [2] == ' ' || pos [2] == '-' || pos [2] == '/' || pos [2] == '.') &&
        (pos [5] == ' ' || pos [5] == '-' || pos [5] == '/' || pos [5] == '.') &&
        pos [2] == pos [5] /* the separator must be same! */ &&
        _isdigit (pos [3]) && _isdigit (pos [4]) && _isdigit (pos [0]) && _isdigit (pos [1])
    ) {
        pos += 10; return;
    }

    // Form dd-m-yyyy (months jan-sep only)
    //      0123456789
    if (
        pos [6] == '2' && pos [7] == '0' && _isdigit (pos [8]) && _isdigit (pos [9]) &&
        (pos [2] == ' ' || pos [2] == '-' || pos [2] == '/' || pos [2] == '.') &&
        (pos [4] == ' ' || pos [4] == '-' || pos [4] == '/' || pos [4] == '.') &&
        pos [2] == pos [4] /* the separator must be same! */ &&
        _isdigit (pos [3]) && _isdigit (pos [0]) && _isdigit (pos [1])
    ) {
        pos += 10; return;
    }

    // Form dd-mm-yy / mm-dd-yy
    //      01234567
    if (
        _isdigit (pos [0]) && _isdigit (pos [1]) &&
        (pos [2] == ' ' || pos [2] == '-' || pos [2] == '/' || pos [2] == '.') &&
        (pos [5] == ' ' || pos [5] == '-' || pos [5] == '/' || pos [5] == '.') &&
        pos [2] == pos [5] /* the separator must be same! */ &&
        _isdigit (pos [3]) && _isdigit (pos [4]) && _isdigit (pos [6]) && _isdigit (pos [7]) &&
        (pos [8] == ' ' || pos [8] == '\t' || 
         pos [8] == '\r' || pos [8] == '\n') /* any separatore after date, otherwise it may be latitude! */
    ) {
        pos += 8; return;
    }
}

void skipTime (char *& pos)
{
    // Form hh:mm:ss
    //      01234567
    if (
        _isdigit (pos [0]) && _isdigit (pos [1]) &&
        (pos [2] == '-' || pos [2] == '/' || pos [2] == ':') &&
        (pos [5] == '-' || pos [5] == '/' || pos [5] == ':') &&
        pos [2] == pos [5] /* the separator must be same! */ &&
        _isdigit (pos [3]) && _isdigit (pos [4]) && _isdigit (pos [6]) && _isdigit (pos [7]) &&
        (pos [8] == ' ' || pos [8] == '\t' || 
         pos [8] == '\r' || pos [8] == '\n') /* any separatore after date, otherwise it may be latitude! */
    ) {
        pos += 8; return;
    }

    // Form hh:mm
    //      01234
    if (
        _isdigit (pos [0]) && _isdigit (pos [1]) &&
        (pos [2] == '-' || pos [2] == '/' || pos [2] == ':') &&
        _isdigit (pos [3]) && _isdigit (pos [4]) &&
        (pos [5] == ' ' || pos [5] == '\t' || pos [5] == '\r' || pos [5] == '\n' /* some separator after time*/)
    ) {
        pos += 5; return;
    }
}

OptionalPos queryDirtyPosition (const char *start) {
    double lat, lon;
    char *lonPos;

    // Dirty and simplified position form ddH dddH
    if (_isdigit (start [0]) && _isdigit (start [1]) && (start [2] == 'N' || start [2] == 'S')) {
        lat = atof (start);
        if (start [2] == 'S')
            lat = - lat;
    } else {
        return NO_POSITION;
    }

    if (lat < -89.9 || lat > 89.9)
        return NO_POSITION;

    if (_isdigit (start [3])) {
        lonPos = (char *) start + 3;
    } else if (start [3] == ' ') {
        lonPos = (char *) start + 4;
    } else {
        return NO_POSITION;
    }

    if (_isdigit (lonPos [0]) && _isdigit (lonPos [1]) && _isdigit (lonPos [2]) && (lonPos [3] == 'E' || lonPos [3] == 'W')) {
        lon = atof (lonPos);

        if (lonPos [3] == 'W') lon = - lon;
    } else if (_isdigit (lonPos [0]) && _isdigit (lonPos [1]) && (lonPos [2] == 'E' || lonPos [2] == 'W')) {
        lon = atof (lonPos);

        if (lonPos [2] == 'W') lon = - lon;
    } else {
        return NO_POSITION;
    }

    if (lon < -180.0 || lon > 180.0) return NO_POSITION;

    return OptionalPos (true, lat, lon);
}

PosExtractResult extractPos (char *source) {
    if (*source < ' ' || *source > '~') {
        return NO_MORE_POS;
    }

    if (source) {
        char *start = source;
        char *startSave;
        std::string textSave;
        char data;
        bool latPassed = false;
        bool lonPassed = false;
        bool latFailed;
        double lat, lon;

        do {
            data = *start;

            if (data >= '0' && data <= '9') {
                // Skip timestamps
                skipDate (start);
                skipTime (start);

                latFailed = false;

                if (!latPassed && !lonPassed) {
                    auto result = queryDirtyPosition (start);
                    bool exists = std::get<0> (result);
                    lat = std::get<1> (result);
                    lat = std::get<2> (result);
                    if (exists) {
                        return PosExtractResult (start + 8, lat, lon);
                    }
                }

                if (!latPassed) {
                    char *startSave2 = start;
                    
                    textSave = start;

                    auto result = queryLatValue (start);
                    lat = std::get<0> (result);
                    size_t size = std::get<1> (result);
                    latPassed = std::get<2> (result);

                    if (latPassed && fabs (lat) < 90.0) {
                        startSave = start;
                        start += size - 1 /* because start will be incremented now but longitude may go immediately! */;
                    } else {
                        latPassed = false;
                        start = startSave2;

                        strcpy (start, textSave.c_str ());
                        textSave.clear ();
                        latFailed = true;
                    }
                } else if (!lonPassed) {
                    int   nSize;
                    char *startSave2 = start;
                    
                    auto result = queryLonValue (start);
                    lon = std::get<0> (result);
                    size_t size = std::get<1> (result);
                    lonPassed = std::get<2> (result);

                    if (lonPassed && fabs (lon) < 180.0) {
                        return PosExtractResult (start + size, lat, lon);
                    } else {
                        latPassed = lonPassed = false;
                        start = startSave2 + 1;
                    }
                }
            }

            if (!latPassed && !lonPassed) {
                if (data == 'N' || data == 'S') {
                    // Specific case N dd dd.ddd E ddd dd.dd
                    // N dd dd.ddE ddd dd.dd
                    // 01234567890123456789012
                    if (
                        start [1] == ' ' && start [4] == ' ' && start [11] == ' ' && start [15] == ' ' &&
                        start [7] == '.' && start [18] == '.' &&
                        (start [10] == 'E' || start [10] == 'W') &&
                        _isdigit (start [2]) && _isdigit (start [3]) && _isdigit (start [5]) && _isdigit (start [6]) &&
                        _isdigit (start [8]) && _isdigit (start [9]) &&
                        _isdigit (start [12]) && _isdigit (start [13]) && _isdigit (start [14]) &&
                        _isdigit (start [16]) && _isdigit (start [17]) &&
                        _isdigit (start [19]) && _isdigit (start [20])
                    ) {
                        lat  = (double) atoi (start + 2) + atof (start + 5) / 60.0;
                        lon = (double) atoi (start + 12) + atof (start + 16) / 60.0;

                        if (start [0] == 'S') lat = - lat;
                        if (start [11] == 'W') lon = - lon;

                        return PosExtractResult (start + 20, lat, lon);
                    } else {
                        ++ start;
                    }
                } else if (start [8] == '/' && start [4] == '.'  && start [14] == '.') {
                    // Specific case ddmm.mmN/dddmm.mmE
                    bool isCorrect = (start [17] == 'E' || start [17] == 'W') && (start [7] == 'N' || start [7] == 'S');
                    size_t i;

                    // ddmm.mmN/dddmm.mmE
                    // 012345678901234567
                    for (i = 0; i < 18 && isCorrect; ++ i) {
                        if (i != 4 && i != 7 && i != 8 && i != 14 && i != 17) {
                            if (!_isdigit (start [i])) {
                                isCorrect = false; break;
                            }
                        }
                    }

                    if (isCorrect) {
                        char latDeg [3];
                        char lonDeg [4];
                        char latMin [6];
                        char lonMin [6];

                        memset (latDeg, 0, sizeof (latDeg));
                        memset (lonDeg, 0, sizeof (lonDeg));
                        memset (latMin, 0, sizeof (latMin));
                        memset (lonMin, 0, sizeof (lonMin));
                        memcpy (latDeg, start, 2);
                        memcpy (latMin, start + 2, 5);
                        memcpy (lonDeg, start + 9, 3);
                        memcpy (lonMin, start + 12, 5);

                        lat  = (double) atoi (latDeg) + atof (latMin) / 60.0;
                        lon = (double) atoi (lonDeg) + atof (lonMin) / 60.0;

                        if (start [7] == 'S') lat = - lat;
                        if (start [17] == 'W') lon = - lon;

                        return PosExtractResult (start + 18, lat, lon);
                    } else {
                        ++ start;
                    }
                } else {
                    ++ start;
                }
            } else {
                ++ start;
            }
        }
        while (*start);

        if (latPassed && !lonPassed)
        {
            source = startSave;

            strcpy (startSave, textSave.c_str ());
        }
    }

    return NO_MORE_POS;
}

void extractPositions (MsgInfo *msgInfo, char *source) {
    if (!msgInfo || !source || !*source) return;

    while (source && *source) {
        auto [next, lat, lon] = extractPos (source);

        if (next) {
            msgInfo->addPosition (lat, lon);
        }

        source = next;
    }
}

MsgInfo *MsgInfo::parseNativeMsg (const char *source, time_t sentAt, bool useHeaderAndTail, const char *msgID) {
    std::string msgText;
    if (useHeaderAndTail) {
        const char *header = strstr (source, MSG_HEAD);
        const char *tail = header ? strstr (header + MSG_HEAD_SIZE, MSG_TAIL) : nullptr;
        if (!tail) return nullptr;
        msgText.insert (msgText.begin (), header + MSG_HEAD_SIZE, tail);
    } else {
        msgText = source;
    }
    while (!msgText.empty () && (msgText.front () <= ' ' || msgText.front () == '*')) {
        msgText.erase (msgText.begin ());
    }
    if (!msgID) msgID = source;
    LineID lineID = parseLineID (msgID);
    if (std::get<0> (lineID)) {
        char subject = std::get<1> (lineID);
        char station = std::get<2> (lineID);
        uint8_t serialNo = std::get<3> (lineID);
        if (isSubjectEnabled (subject)) {
            return new MsgInfo (subject, station, serialNo,  time (nullptr), sentAt, msgText.c_str ());
        }
    }
    return nullptr;
}

