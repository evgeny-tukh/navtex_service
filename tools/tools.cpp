#include <Windows.h>
#include <string>
#include <tuple>
#include <map>
#include <vector>
#include <time.h>
#include "tools.h"

namespace tools {

    std::string constructPortName (int port) {
        return std::string ("\\\\.\\COM").append (std::to_string (port).c_str ());
    }

    strings& getSerialPortsList (strings& ports) {
        HKEY scomKey;
        int count = 0;
        DWORD error = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "Hardware\\DeviceMap\\SerialComm", 0, KEY_QUERY_VALUE, & scomKey);

        if (error == S_OK) {
            char valueName[100], valueValue[100];
            DWORD nameSize, valueSize, valueType;
            BOOL valueFound;

            do {
                nameSize = sizeof (valueName);
                valueSize = sizeof (valueValue);
                valueFound = RegEnumValue (scomKey, (DWORD) count ++, valueName, & nameSize, NULL, & valueType, (BYTE *) valueValue, & valueSize) == S_OK;

                if (valueFound) ports.push_back (valueValue);
            } while (valueFound);

            RegCloseKey (scomKey);
        }

        return ports;
    }

    std::string getFirstAvailablePort () {
        strings ports;
        std::string port;
        if (getSerialPortsList (ports).size () > 0) {
            for (auto& portName: ports) {
                auto portHandle = CreateFile (std::string ("\\\\.\\").append (portName).c_str (), GENERIC_READ, 0, nullptr, OPEN_ALWAYS, 0, nullptr);
                bool opened = portHandle != INVALID_HANDLE_VALUE;
                if (opened) {
                    CloseHandle (portHandle);
                    port = portName;
                    break;
                }
            }
        }
        return port;
    }

    int serialPortFromUnc (const char *uncPortName) {
        if (!uncPortName || !*uncPortName) return 0;

        char portName [1000];
        strcpy (portName, uncPortName);
        strupr (portName);
        
        char *com = strstr (portName, "COM");
        if (!com || !isdigit (com [3])) return 0;

        return atoi (com + 3);
    }

    int getFirstAvailablePortNo () {
        strings ports;
        int port;
        if (getSerialPortsList (ports).size () > 0) {
            for (auto& portName: ports) {
                auto portHandle = CreateFile (std::string ("\\\\.\\").append (portName).c_str (), GENERIC_READ, 0, nullptr, OPEN_ALWAYS, 0, nullptr);
                bool opened = portHandle != INVALID_HANDLE_VALUE;
                if (opened) {
                    CloseHandle (portHandle);
                    port = std::stoi (portName.substr (3));
                    break;
                }
            }
        }
        return port;
    }

    std::tuple<bool, std::string> loadFile (const wchar_t *filePath) {
        HANDLE fileHandle = CreateFileW (filePath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (fileHandle == INVALID_HANDLE_VALUE) return std::tuple<bool, std::string> (false, "");
        unsigned long size = GetFileSize (fileHandle, nullptr);
        char *buffer = (char *) malloc (size + 1);
        unsigned long bytesRead = 0;
        ReadFile (fileHandle, buffer, size, & bytesRead, nullptr);
        CloseHandle (fileHandle);
        buffer [bytesRead] = '\0';
        return std::tuple<bool, std::string> (true, std::string (buffer));
    }

    uint8_t mapStr2Num (NumMap& map, const char *str) {
        auto pos = map.find (str);
        return pos == map.end () ? map.begin ()->second : pos->second;
    }

    uint8_t charStr2Num (NumMap& map, char chr) {
        char str [] { chr, 0 };
        auto pos = map.find (str);
        return pos == map.end () ? map.begin ()->second : pos->second;
    }

    uint8_t str2parity (const char *parityStr) {
        static NumMap parities {
            { "None", NOPARITY },
            { "Odd", ODDPARITY },
            { "Even", EVENPARITY },
            { "Mark", MARKPARITY },
            { "Space", SPACEPARITY },
        };
        return mapStr2Num (parities, parityStr);
    }

    uint8_t str2stopBits (const char *stopBitsStr) {
        static NumMap stopBits {
            { "1", ONESTOPBIT },
            { "2", TWOSTOPBITS },
            { "1.5", ONE5STOPBITS },
        };
        return mapStr2Num (stopBits, stopBitsStr);
    }

    uint8_t char2parity (char parityStr) {
        static NumMap parities {
            { "N", NOPARITY },
            { "O", ODDPARITY },
            { "E", EVENPARITY },
            { "M", MARKPARITY },
            { "S", SPACEPARITY },
        };
        return charStr2Num (parities, parityStr);
    }

    uint8_t char2stopBits (char stopBitsStr) {
        static NumMap stopBits {
            { "1", ONESTOPBIT },
            { "2", TWOSTOPBITS },
            { "0", ONE5STOPBITS },
        };
        return charStr2Num (stopBits, stopBitsStr);
    }

    uint8_t char2hex (char chr) {
        if (chr >= '0' && chr <= '9') return chr - '0';
        if (chr >= 'A' && chr <= 'F') return chr - 'A' + 10;
        if (chr >= 'a' && chr <= 'f') return chr - 'a' + 10;
        return 0;
    }

    uint8_t twoChars2hex (char *chars) {
        return chars ? char2hex (chars [0]) * 16 + char2hex (chars [1]) : 0;
    }

    uint8_t twoChars2int (char *chars) {
        if (!chars || !isdigit (chars [0]) || !isdigit (chars [1])) return 0;
        return chars ? char2hex (chars [0]) * 10 + char2hex (chars [1]) : 0;
    }

    time_t hhmmss2time (char *source) {
        for (size_t i = 0; i < 6; ++ i) {
            if (!source [i] || !isdigit (source [i])) return -1;
        }
        auto hr = twoChars2int (source);
        auto min = twoChars2int (source + 2);
        auto sec = twoChars2int (source + 4);
        if (hr > 23 || min > 59 || sec > 59) return -1;
        return (time_t) hr * 3600 + (time_t) min * 60 + (time_t) sec;
    }

    time_t ddmmyyyy2time (unsigned day, unsigned month, unsigned year) {
        if (day < 1 || day > 31 || month < 1 || month > 12 || year < 1980) return -1;
        static int monSize [] { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, };
        uint64_t result = (year - 1970) * 365 + (year - 1970) / 4 - 1;
        for (auto i = 1; i < month; ++ i) {
            result += monSize [i-1];
        }
        if (month > 2 && (year % 4) == 0) result ++;
        result += (day - 1);
        return (time_t) (result * 3600 * 24);
    }
    time_t ddmmyyyy2time (char *source) {
        for (size_t i = 0; i < 6; ++ i) {
            if (!source [i] || !isdigit (source [i])) return -1;
        }
        auto day = twoChars2int (source);
        auto month = twoChars2int (source + 2);
        auto year = twoChars2int (source + 4);
        return ddmmyyyy2time (day, month, year);
    }
}