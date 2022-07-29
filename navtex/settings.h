#pragma once

#include <cstdint>
#include <Windows.h>
#include "defs.h"
#include "tools.h"

struct Settings {
    bool useSerial, forward;
    std::string serialPort;
    uint32_t baud;
    uint8_t byteSize, parity, stopBits;
    uint32_t inPort, outPort;
    IN_ADDR bindAddr;
    uint64_t crc;
    std::wstring path;

    static uint32_t calcCrc (const wchar_t *file);

    void setCfgFile (const wchar_t *file) {
        path = file;
        loadIfChanged ();
    }

    void load (const wchar_t *cfgFile);
    void loadIfChanged ();

    Settings (): 
        crc (0),
        path (),
        useSerial (true),
        serialPort (),
        baud (4800),
        byteSize (8),
        parity (NOPARITY),
        stopBits (ONESTOPBIT),
        inPort (0),
        outPort (0) {
        bindAddr.S_un.S_addr = INADDR_ANY;
        serialPort = tools::getFirstAvailablePort ();
    }
};
