#pragma once

#include <Windows.h>
#include <stdio.h>

#include <map>
#include <string>

struct Logger {
    HANDLE handle;
    std::wstring filename;

    Logger (const wchar_t *_filename): filename (_filename), handle (INVALID_HANDLE_VALUE) {}
    virtual ~Logger () {
        if (handle != INVALID_HANDLE_VALUE) CloseHandle (handle);
    }
    void addToLog (wchar_t *string) {
        unsigned long bytesWritten;
        WriteFile (handle, string, wcslen (string) * sizeof (wchar_t), & bytesWritten, nullptr);
    }
    void addLogRecord (wchar_t *record) {
        addLogPrefix ();
        addToLog (record);
        addToLog (L"\r\n");
    }
    void addLogPrefix ();
    void addLogRecord (wchar_t *recordFmt, wchar_t *arg) {
        wchar_t buffer [500];
        _swprintf (buffer, recordFmt, arg);
        addLogRecord (buffer);
    }
    void stop ();
    void start ();

    static std::map<std::wstring, Logger> loggers;
    static Logger *registerLogger (const wchar_t *filename);
};




