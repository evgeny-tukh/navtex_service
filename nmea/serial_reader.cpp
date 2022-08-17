#include "serial_reader.h"

bool SerialReader::connect () {
    SerialReaderCfg *config = (SerialReaderCfg *) cfg;
    port = CreateFile (tools::constructPortName (config->port).c_str (), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    connected = port != INVALID_HANDLE_VALUE;
    return connected;
}

bool SerialReader::disconnect () {
    if (port != INVALID_HANDLE_VALUE) {
        CloseHandle (port);
        port = INVALID_HANDLE_VALUE;
    }
    connected = false;
    return true;
}

size_t SerialReader::dataAvailable () {
    COMSTAT       commState;
    unsigned long errorFlags, bytesRead, errorCode;
    bool          overflow;
    unsigned char buffer [0xFFFF];

    ClearCommError (port, & errorFlags, & commState);

    overflow = (errorFlags & (CE_RXOVER | CE_OVERRUN)) != 0L;
    return commState.cbInQue;
}

bool SerialReader::getAvailableData (Buffer& result) {
    bool ok = false;
    auto bytesAvailable = dataAvailable ();
    if (bytesAvailable > 0) {
        result.resize (bytesAvailable);
        unsigned long bytesRead;
        ReadFile (port, result.data (), bytesAvailable, & bytesRead, nullptr);
        ok = GetLastError () == 0;
    } else {
        result.clear ();
    }
    return true;
}
