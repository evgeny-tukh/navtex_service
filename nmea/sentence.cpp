#include "sentence.h"

uint8_t Sentence::calcCRC (const char *source) {
    if (!source || !*source) return 0;
    uint8_t crc = (uint8_t) *source;
    for (char *chr = (char *) source + 1; *chr; ++ chr) {
        if (*chr == '*') return crc;
        crc ^= *chr;
    }
    return 0;
}

void Sentence::parse (const char *source) {
    fields.clear ();
    fields.emplace_back ();
    for (auto chr = source; *chr; ++ chr) {
        if (*chr == ',') {
            fields.emplace_back ();
        } else {
            fields.back () += *chr;
        }
    }
}
