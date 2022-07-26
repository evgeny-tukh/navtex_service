#include <Windows.h>
#include <Shlwapi.h>
#include <thread>
#include <string>
#include <chrono>
#include "../log/log.h"
#include "navtex.h"
#include "settings.h"
#include "../sqlite/sqlite3.h"
#include "db.h"
#include "logger_helper.h"
#include "../nmea/nmea.h"
#include "parsing.h"

const wchar_t *SETTINGS_FILE = L"NavtexSettings.json";

Logger *logger = nullptr;
bool running = true;
bool stopNow = false;
std::thread *worker = nullptr;
Settings *settings = nullptr;
MsgCb msgAddCb = nullptr;
MsgCb2 msgAddCb2 = nullptr;
MsgCb msgRemoveCb = nullptr;
sqlite3 *db = nullptr;
nmea::CHANNEL reader = nullptr;
std::vector<bool> subjectFilter;
std::string nativeMsg;

bool isCharCodeValid (char subject) {
    return subject >= 'A' && subject <= 'Z';
}

bool isSubjectEnabled (char subject) {
    if (subjectFilter.empty ()) subjectFilter.insert (subjectFilter.begin (), 26, true);
    return isCharCodeValid (subject) ? subjectFilter [subject - 'A'] : false;
}

void NAVTEX_API SetMsgAddCb (MsgCb cb) {
    msgAddCb = cb;
}

void NAVTEX_API SetMsgAddCb2 (MsgCb2 cb) {
    msgAddCb2 = cb;
}

void NAVTEX_API SetMsgRemoveCb (MsgCb cb) {
    msgRemoveCb = cb;
}

void doIteration () {
    static time_t lastExpiredRecsCheck = 0;
    static const time_t EXPIRED_RECS_CHECK_PERIOD = 30;
    time_t now = time (nullptr);
    
    if ((now - lastExpiredRecsCheck) >= EXPIRED_RECS_CHECK_PERIOD) {
        lastExpiredRecsCheck = now;
        auto result = removeExpiredMessages (db, settings->expiryHours);
        _logger (L"Expired records check: deleted %d messages, %d objects", std::get<0> (result), std::get<1> (result));
    }
    #if 0
    static time_t lastImitation = 0;
    static uint32_t lastMsgID = 1;

    time_t now = time (nullptr);

    if (db && (now - lastImitation) > 30) {
        lastImitation = now;
        static int count = 1;
        std::string msgText { "HI THERE" };
        auto msg = new MsgInfo ('A', 'A', count, time (nullptr), (msgText + std::to_string (count ++)).c_str ());
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
    #endif
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


void onSentenceParsed (nmea::SENTENCE sentence) {
    onNewSentence (sentence);
}

void processNativeProtocolLine (char *buffer, size_t size) {
    auto parseNativeMsg = [] () {
        std::string translatedMsg;
        translateControlCharacters (nativeMsg.c_str (), translatedMsg);
        while (
            !translatedMsg.empty () && (
                translatedMsg.front () == ' ' || 
                translatedMsg.front () == '\t' ||
                translatedMsg.front () == '\r' ||
                translatedMsg.front () == '\n'
            )
        ) {
            translatedMsg.erase (translatedMsg.begin ());
        }
        auto whenSent = extractUtc (translatedMsg.data ());
        processNativeMsg (translatedMsg.c_str (), translatedMsg.substr (0, 4).c_str (), whenSent);
        nativeMsg.clear ();
    };
    std::string input (buffer, size);
    if (nativeMsg.empty ()) {
        auto headerPos = input.find (MSG_HEAD);
        if (headerPos != std::string::npos) {
            auto tailPos = input.find (MSG_TAIL, headerPos + MSG_HEAD_SIZE);
            if (tailPos == std::string::npos) {
                nativeMsg = input.substr (headerPos + MSG_HEAD_SIZE);
            } else {
                nativeMsg = input.substr (headerPos + MSG_HEAD_SIZE, tailPos - headerPos);
                parseNativeMsg ();
                input.erase (input.begin (), input.begin () + (tailPos + MSG_TAIL_SIZE));
                if (!input.empty ()) processNativeProtocolLine (input.data (), input.length ());
            }
        }
    } else {
        auto tailPos = input.find (MSG_TAIL);
        if (tailPos == std::string::npos) {
            nativeMsg += input;
        } else {
            nativeMsg += input.substr (0, tailPos);
            parseNativeMsg ();
            input.erase (input.begin (), input.begin () + (tailPos + MSG_TAIL_SIZE));
            if (!input.empty ()) processNativeProtocolLine (input.data (), input.length ());
        }
    }
}

void onSentenceReceived (char *buffer, size_t size) {
    if (settings->useNativeProtocol) {
        processNativeProtocolLine (buffer, size);
    } else {
        std::string rawData (buffer, size);
        nmea::extractAndParseAll (rawData.data (), onSentenceParsed);
    }
    if (reader && nmea::getConnectionType (reader) == nmea::ConnectionType::SERIAL) {
        nmea::writeToMedia (reader, nmea::ConnectionType::UDP, buffer);
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
    if (!reader) reader = nmea::createChannel (nmea::ConnectionType::SERIAL, onSentenceReceived);
    nmea::configureChannel (reader, tools::serialPortFromUnc (settings->serialPort.c_str ()), settings->baud, settings->byteSize, settings->parity, settings->stopBits);
    nmea::configureChannel (reader, settings->inPort, settings->outPort, settings->bindAddr);
    nmea::setConnectionType (reader, settings->useSerial ? nmea::ConnectionType::SERIAL : nmea::ConnectionType::UDP);
    nmea::connectChannel (reader);
    nmea::activateChannel (reader, true);
    return 0;
}

void NAVTEX_API StopNavtexReceiver () {
    _logger (L"StopNavtexReceiver");
    if (reader) {
        nmea::activateChannel (reader, false);
        if (nmea::isChannelConnected (reader)) nmea::disconnectChannel (reader);
        nmea::deleteChannel (reader);
        reader = nullptr;
    }
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