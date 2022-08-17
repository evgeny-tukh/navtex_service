#pragma once

#include <tuple>
#include <vector>
#include <string>
#include <map>

namespace tools {
    typedef std::vector<std::string> strings;
    typedef std::map<std::string, uint8_t> NumMap;

    uint8_t mapStr2Num (NumMap& map, const char *str);

    strings& getSerialPortsList (strings& ports);
    std::string getFirstAvailablePort ();

    std::tuple<bool, std::string> loadFile (const wchar_t *filePath);

    uint8_t str2parity (const char *parityStr);
    uint8_t str2stopBits (const char *stopBitsStr);

    uint8_t char2parity (const char parityStr);
    uint8_t char2stopBits (const char stopBitsStr);
}
