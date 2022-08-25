#include "serial_reader.h"
#include "udp_reader.h"

bool SerialReader::connect () {
    SerialReaderCfg *config = (SerialReaderCfg *) cfg;
    port = CreateFile (tools::constructPortName (config->port).c_str (), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    connected = port != INVALID_HANDLE_VALUE;

    if (connected) {
        COMMTIMEOUTS timeouts;
        DCB dcb;

        SetupComm (port, 4096, 4096);
        PurgeComm (port, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

        memset (& dcb, 0, sizeof (dcb));

        GetCommState (port, & dcb);

        dcb.BaudRate = config->baud;
        dcb.ByteSize = config->byteSize;
        dcb.StopBits = config->stopBits;
        dcb.Parity = config->parity;
        dcb.fBinary =
        dcb.fParity = 1;
        dcb.fOutxDsrFlow = 0; 
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
        dcb.fOutxCtsFlow = 0; 
        dcb.fRtsControl  = RTS_CONTROL_ENABLE;
        dcb.fInX  =
        dcb.fOutX = 1;
        dcb.XonChar = ASCII_CHARS::XON;
        dcb.XoffChar = ASCII_CHARS::XOFF;
        dcb.XonLim = 100;
        dcb.XoffLim = 100;

        SetCommState (port, & dcb);
        GetCommTimeouts (port, & timeouts);

        timeouts.ReadIntervalTimeout = 1000;
        timeouts.ReadTotalTimeoutMultiplier = 1;
        timeouts.ReadTotalTimeoutConstant = 3000;

        SetCommTimeouts (port, & timeouts);
        EscapeCommFunction (port, SETDTR);
    }

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
    return ok;
}

bool SerialReader::write (char *data) {
    if (port == INVALID_HANDLE_VALUE) return false;
    unsigned long bytesWritten;
    return WriteFile (port, data, strlen (data), & bytesWritten, nullptr) && bytesWritten > 0;
}