#include <Windows.h>
#include <Shlwapi.h>
#include <thread>
#include <chrono>
#include "../log/log.h"
#include "navtex.h"
#include "settings.h"
#include "../sqlite/sqlite3.h"
#include "db.h"
#include "logger_helper.h"

const wchar_t *SETTINGS_FILE = L"NavtexSettings.json";

Logger *logger = nullptr;
bool running = true;
bool stopNow = false;
std::thread *worker = nullptr;
Settings *settings = nullptr;
MsgCb msgAddCb = nullptr;
MsgCb msgRemoveCb = nullptr;
sqlite3 *db = nullptr;

void NAVTEX_API SetMsgAddCb (MsgCb cb) {
    msgAddCb = cb;
}

void NAVTEX_API SetMsgRemoveCb (MsgCb cb) {
    msgRemoveCb = cb;
}

void doIteration () {
    static time_t lastImitation = 0;
    static uint32_t lastMsgID = 1;

    time_t now = time (nullptr);

    if (db && (now - lastImitation) > 30) {
        lastImitation = now;
        static int count = 1;
        std::string msgText { "HI THERE" };
        auto msg = new MsgInfo ('A', 'A', time (nullptr), (msgText + std::to_string (count ++)).c_str ());
        for (auto i = 0; i < time (nullptr) % 10; ++ i) {
            msg->positions.emplace_back (59.0 + i * 0.001, 9.0 + i * 0.0001);
        }
        
        addMessage (db, msg);
        delete msg;
        if (msgAddCb) {
            std::wstring msg;
            for (uint32_t i = 0; i < 10; ++ i) {
                if (!msg.empty ()) msg += L',';
                msg += std::to_wstring (lastMsgID++);
            }
            msgAddCb ((wchar_t *) msg.c_str ());
        }
    }
}

void startWorker () {
    wchar_t settingsPath [MAX_PATH];
    GetModuleFileNameW (nullptr, settingsPath, MAX_PATH);
    PathRemoveFileSpecW (settingsPath);
    PathAppendW (settingsPath, SETTINGS_FILE);

    settings->setCfgFile (settingsPath);

    _logger (L"Worker is starting...");

    worker = new std::thread ([] {
        while (!stopNow) {
            settings->loadIfChanged ();
            if (running) doIteration ();
            std::this_thread::sleep_for (std::chrono::microseconds (100));
        }
    });
}

void stopWorker () {
    _logger (L"Worker is stopping...");
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
    char pathUtf8 [1000];
    WideCharToMultiByte (CP_UTF8, 0, pathToDb, -1, pathUtf8, sizeof (pathUtf8), nullptr, nullptr);
    _logger (L"StartNavtexReceiver; db path: %s", pathToDb);
    if (sqlite3_open (pathUtf8, & db) == SQLITE_OK) {
        _logger (L"Ok. Checking the version...");
        checkDb (db);
        
    } else {
        _logger (L"failed: %S", (char *) sqlite3_errmsg (db));
    }
    return 0;
}

void NAVTEX_API StopNavtexReceiver () {
    _logger (L"StopNavtexReceiver");
}

void __declspec( dllexport) ReloadSettings () {
    if (!settings) settings = new Settings;
    _logger (L"ReloadSettings");
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