#include "sentence.h"
#include "../tools/tools.h"

uint8_t Sentence::calcCRC (const char *source) {
    if (!source || !*source) return 0;
    uint8_t crc = (uint8_t) source [1];
    for (char *chr = (char *) source + 2; *chr; ++ chr) {
        if (*chr == '*') {
            return crc;
        }
        crc ^= *chr;
    }
    return 0;
}

bool Sentence::parse (const char *source) {
    fields.clear ();
    fields.emplace_back ();
    for (auto chr = source; *chr; ++ chr) {
        if (*chr == '*') {
            uint8_t givenCrc = tools::twoChars2hex ((char *) chr + 1);
            if (givenCrc != calcCRC (source)) {
                return false;
            }
        } else if (*chr == ',') {
            fields.emplace_back ();
        } else if (*chr == '\r' || *chr == '\n') {
            return false;
        } else {
            fields.back () += *chr;
        }
    }
    return true;
}

char *Sentence::extractAndParse (char *source) {
    while (*source && *source != '!' && *source != '$') ++ source;
    if (!*source) return nullptr;
    sixBitEncoded = source [0] == '!';
    proprietary = source [1] == 'P';
    char *start = source;
    char *end = nullptr;
    while (*source && *source != '\r' && *source != '\n') end = (++source);
    std::string sentence;
    for (auto chr = start; chr != end; ++ chr) sentence += *chr;
    if (sentence.empty ()) {
        fields.clear ();
    } else {
        parse (sentence.c_str ());
    }
    return end;
}

void Sentence::extractAndParseAll (char *source, std::function<void (Sentence *)> cb) {
    if (!source) return;

    while (source) {
        source = extractAndParse (source);
        if (fields.size () > 0) cb (this);
    }
}
