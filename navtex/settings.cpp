#include <Windows.h>
#include <cstdint>
#include <string>
#include <functional>
#include <tuple>

#include "../json/json_lite.h"
#include "../log/log.h"
#include "settings.h"

extern Logger *logger;

uint32_t Settings::calcCrc (const wchar_t *file) {
    auto [exists, content] = tools::loadFile (file);
    if (!exists) return 0;
    uint32_t crc = content.front ();
    for (auto chr = content.begin () + 1; chr != content.end (); ++ chr) {
        crc += *chr;
    }
    return crc;
}

void Settings::loadIfChanged () {
    if (!path.empty ()) {
        auto curCrc = calcCrc (path.c_str ());
        if (curCrc != crc) {
            if (logger) logger->addLogRecord (L"Reloading changed config...");
            crc = curCrc;
            load (path.c_str ());
        }
    }
}

void Settings::load (const wchar_t *cfgFile) {
    auto [exists, content] = tools::loadFile (cfgFile);

    if (exists) {
        int nextChar = 0;
        auto json = json::parse (content.data (), nextChar);
        if (json != json::nothing) {
            json::hashNode *cfg = (json::hashNode *) json;
            json::booleanNode *useSerialNode = (json::booleanNode *) (*cfg) ["com_mode"];
            json::stringNode *serialPortNode = (json::stringNode *) (*cfg) ["serial_port"];
            json::numberNode *baudNode = (json::numberNode *) (*cfg) ["baud_rate"];
            json::numberNode *byteSizeNode = (json::numberNode *) (*cfg) ["byte_size"];
            json::stringNode *parityNode = (json::stringNode *) (*cfg) ["parity"];
            json::stringNode *stopBitsNode = (json::stringNode *) (*cfg) ["stop_bits"];
            json::booleanNode *forwardNode = (json::booleanNode *) (*cfg) ["forward"];
            json::numberNode *inPortNode = (json::numberNode *) (*cfg) ["udp_in"];
            json::numberNode *outPortNode = (json::numberNode *) (*cfg) ["udp_out"];
            json::stringNode *bindNode = (json::stringNode *) (*cfg) ["udp_bind"];
            json::booleanNode *protocolNode = (json::booleanNode *) (*cfg) ["native_protocol"];
            json::numberNode *expiryHoursNode = (json::numberNode *) (*cfg) ["expiry_hours"];

            useSerial = useSerialNode == json::nothing ? true : useSerialNode->getValue ();
            serialPort = serialPortNode == json::nothing ? 0 : serialPortNode->getValue ();
            baud = baudNode == json::nothing ? 4800 : (uint32_t) baudNode->getValue ();
            byteSize = byteSizeNode == json::nothing ? 8 : (uint8_t) byteSizeNode->getValue ();
            parity = parityNode == json::nothing ? NOPARITY : tools::str2parity (parityNode->getValue ());
            stopBits = stopBitsNode == json::nothing ? ONESTOPBIT : tools::str2stopBits (stopBitsNode->getValue ());;
            forward = forwardNode == json::nothing ? false : forwardNode->getValue ();
            inPort = inPortNode == json::nothing ? 0 : (uint32_t) inPortNode->getValue ();
            outPort = outPortNode == json::nothing ? 0 : (uint32_t) outPortNode->getValue ();
            useNativeProtocol = protocolNode == json::nothing ? false : protocolNode->getValue ();
            expiryHours = expiryHoursNode == json::nothing ? 0 : (int) expiryHoursNode->getValue ();
            
            bindAddr.S_un.S_addr = bindNode == json::nothing ? INADDR_ANY : inet_addr (bindNode->getValue ());

            delete json;
        }
    }
}