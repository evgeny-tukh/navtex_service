#pragma once

#include <tuple>
#include <vector>
#include <string>
#include <map>

namespace tools {
    typedef std::vector<std::string> strings;
    typedef std::map<std::string, uint8_t> NumMap;

    uint8_t mapStr2Num (NumMap& map, const char *str);

    std::string constructPortName (int port);
    strings& getSerialPortsList (strings& ports);
    std::string getFirstAvailablePort ();
    int getFirstAvailablePortNo ();
    int serialPortFromUnc (const char *uncPortName);

    std::tuple<bool, std::string> loadFile (const wchar_t *filePath);

    uint8_t str2parity (const char *parityStr);
    uint8_t str2stopBits (const char *stopBitsStr);
    uint8_t char2parity (char parityStr);
    uint8_t char2stopBits (char stopBitsStr);

    uint8_t char2hex (char chr);
    uint8_t twoChars2hex (char *chars);
    uint8_t twoChars2int (char *chars);

    time_t hhmmss2time (char *source);
    time_t ddmmyyyy2time (char *source);
    time_t ddmmyyyy2time (unsigned day, unsigned month, unsigned year);
}
