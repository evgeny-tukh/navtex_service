#include <Windows.h>
#include <Shlwapi.h>
#include <thread>
#include <chrono>
#include "../log/log.h"
#include "navtex.h"
#include "settings.h"

const wchar_t *SETTINGS_FILE = L"NavtexSettings.json";

Logger *logger = nullptr;
bool running = true;
bool stopNow = false;
std::thread *worker = nullptr;
Settings *settings = nullptr;
MsgCb msgCb = nullptr;

void NAVTEX_API SetMsgCb (MsgCb cb) {
    msgCb = cb;
}

void doIteration () {
    static time_t lastImitation = 0;
    static uint32_t lastMsgID = 1;

    time_t now = time (nullptr);

    if ((now - lastImitation) > 5) {
        lastImitation = now;
        if (msgCb) {
            std::wstring msg;
            for (uint32_t i = 0; i < 10; ++ i) {
                if (!msg.empty ()) msg += L',';
                msg += std::to_wstring (lastMsgID++);
            }
            msgCb ((wchar_t *) msg.c_str ());
        }
    }
}

void startWorker () {
    wchar_t settingsPath [MAX_PATH];
    GetModuleFileNameW (nullptr, settingsPath, MAX_PATH);
    PathRemoveFileSpecW (settingsPath);
    PathAppendW (settingsPath, SETTINGS_FILE);

    settings->setCfgFile (settingsPath);

    if (logger) logger->addLogRecord (L"Worker is starting...");
    worker = new std::thread ([] {
        while (!stopNow) {
            settings->loadIfChanged ();
            if (running) doIteration ();
            std::this_thread::sleep_for (std::chrono::microseconds (100));
        }
    });
}

void stopWorker () {
    if (logger) logger->addLogRecord (L"Worker is stopping...");
    if (worker && worker->joinable ()) {
        stopNow = true;
        worker->join ();
    }
    worker = nullptr;
}

void NAVTEX_API StartLog (wchar_t *file) {
    if (!file) {
        static wchar_t path [MAX_PATH];
        GetModuleFileNameW (nullptr, path, MAX_PATH);
        PathRemoveFileSpecW (path);
        PathAppendW (path, L"navtex.log");
        file = path;
    }
    logger = Logger::registerLogger (file);
    logger->start ();
}

void NAVTEX_API StopLog () {
    if (logger) {
        logger->stop ();
        logger = nullptr;
    }
}

int NAVTEX_API StartNavtexReceiver (wchar_t *pathToDb) {
    if (logger) logger->addLogRecord (L"StartNavtexReceiver; db path: %s", pathToDb);
    return 0;
}

void NAVTEX_API StopNavtexReceiver () {
    if (logger) logger->addLogRecord (L"StopNavtexReceiver");
}

void __declspec( dllexport) ReloadSettings () {
    if (!settings) settings = new Settings;
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
            settings = new Settings;
            startWorker ();
            break;
        }
        case DLL_PROCESS_DETACH: {
            StopLog ();
            stopWorker ();
            if (settings) delete settings;
            break;
        }
    }
    return TRUE;
}