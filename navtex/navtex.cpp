#include <Windows.h>
#include <Shlwapi.h>
#include "../log/log.h"
#include "navtex.h"

Logger *logger = nullptr;

void __declspec (dllexport) StartLog (wchar_t *file) {
    logger = Logger::registerLogger (file);
    logger->start ();
}

void __declspec (dllexport) StopLog () {
    if (logger) {
        logger->stop ();
        logger = nullptr;
    }
}

int __declspec (dllexport) StartNavtexReceiver (wchar_t *pathToDb) {
    if (logger) logger->addLogRecord (L"StartNavtexReceiver; db path: %s", pathToDb);
    return 0;
}

void __declspec (dllexport) StopNavtexReceiver () {
    if (logger) logger->addLogRecord (L"StopNavtexReceiver");
}

void __declspec( dllexport) ReloadSettings () {
    if (logger) logger->addLogRecord (L"ReloadSettings");
}

BOOL WINAPI DllMain (HINSTANCE dll, unsigned long reason, void *) {
    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            static wchar_t path [MAX_PATH];
            GetModuleFileNameW (nullptr, path, MAX_PATH);
            PathRemoveFileSpecW (path);
            PathAppendW (path, L"navtex.log");
            StartLog (path);
            break;
        }
        case DLL_PROCESS_DETACH: {
            StopLog (); break;
        }
    }
    return TRUE;
}