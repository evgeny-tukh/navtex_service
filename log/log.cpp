#include <Windows.h>
#include <Shlwapi.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "log.h"

std::map<std::wstring, Logger> Logger::loggers;

void Logger::addLogPrefix () {
    if (handle != INVALID_HANDLE_VALUE) {
        SetFilePointer (handle, 0, nullptr, SEEK_END);
        wchar_t buffer [100];
        time_t timestamp = time (nullptr);
        tm *now = localtime (& timestamp);
        _swprintf (
            buffer,
            L"[%02d.%02d.%04d %02d:%02d:%02d] ",
            now->tm_mday,
            now->tm_mon + 1,
            now->tm_year + 1900,
            now->tm_hour,
            now->tm_min,
            now->tm_sec
        );
        addToLog (buffer);
    }
}

void Logger::stop () {
    if (handle != INVALID_HANDLE_VALUE) {
        addLogRecord (L"log session stopped");
        CloseHandle (handle);

        handle = INVALID_HANDLE_VALUE;
    }
}

void Logger::start () {
    wchar_t defPath [MAX_PATH];
    if (filename.empty ()) {
        GetModuleFileNameW (nullptr, defPath, MAX_PATH);
        PathRenameExtensionW (defPath, L".log");
        filename = defPath;
    }

    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle (handle);
    }

    handle = CreateFileW (filename.c_str (), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, 0, nullptr);

    if (handle != INVALID_HANDLE_VALUE) {
        addLogRecord (L"log session started");
    }
}

Logger *Logger::registerLogger (const wchar_t *filename) {
    auto pos = loggers.find (filename);

    if (pos == loggers.end ()) {
        pos = loggers.emplace (std::pair<std::wstring, Logger> (filename, Logger (filename))).first;
    }

    return & pos->second;
}