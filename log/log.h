#pragma once

#include <Windows.h>
#include <stdio.h>

#include <map>
#include <string>

struct Logger {
    static const unsigned long MAX_SIZE = 100 * 1024;
    HANDLE handle;
    std::wstring filename;

    Logger (const wchar_t *_filename): filename (_filename), handle (INVALID_HANDLE_VALUE) {}
    virtual ~Logger () {
        if (handle != INVALID_HANDLE_VALUE) CloseHandle (handle);
    }
    void addToLog (wchar_t *string);
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
    void addLogRecord (wchar_t *recordFmt, wchar_t *arg1, wchar_t *arg2) {
        wchar_t buffer [500];
        _swprintf (buffer, recordFmt, arg1, arg2);
        addLogRecord (buffer);
    }
    void addLogRecord (wchar_t *recordFmt, wchar_t *arg1, wchar_t *arg2, wchar_t *arg3) {
        wchar_t buffer [500];
        _swprintf (buffer, recordFmt, arg1, arg2, arg3);
        addLogRecord (buffer);
    }
    void addLogRecord (wchar_t *recordFmt, int arg) {
        wchar_t buffer [500];
        _swprintf (buffer, recordFmt, arg);
        addLogRecord (buffer);
    }
    void addLogRecord (wchar_t *recordFmt, int arg1, int arg2) {
        wchar_t buffer [500];
        _swprintf (buffer, recordFmt, arg1, arg2);
        addLogRecord (buffer);
    }
    void stop ();
    void start ();

    static std::map<std::wstring, Logger> loggers;
    static Logger *registerLogger (const wchar_t *filename);
};




