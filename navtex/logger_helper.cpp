#include <Windows.h>
#include "logger_helper.h"
#include "../log/log.h"

void _logger (wchar_t *msg) {
    if (logger) logger->addLogRecord (msg);
}

void _logger (wchar_t *fmt, wchar_t *arg) {
    if (logger) logger->addLogRecord (fmt, arg);
}

void _logger (wchar_t *fmt, int arg) {
    if (logger) logger->addLogRecord (fmt, arg);
}

void _logger (wchar_t *fmt, int arg1, int arg2) {
    if (logger) logger->addLogRecord (fmt, arg1, arg2);
}

void _logger (wchar_t *fmt, char *arg1, char *arg2) {
    wchar_t arg1U [1000];
    wchar_t arg2U [1000];
    MultiByteToWideChar (CP_ACP, 0, arg1, -1, arg1U, 1000);
    MultiByteToWideChar (CP_ACP, 0, arg2, -1, arg2U, 1000);
    if (logger) logger->addLogRecord (fmt, arg1U, arg2U);
}

void _logger (wchar_t *fmt, char *arg1, char *arg2, char *arg3) {
    wchar_t arg1U [1000];
    wchar_t arg2U [1000];
    wchar_t arg3U [1000];
    MultiByteToWideChar (CP_ACP, 0, arg1, -1, arg1U, 1000);
    MultiByteToWideChar (CP_ACP, 0, arg2, -1, arg2U, 1000);
    MultiByteToWideChar (CP_ACP, 0, arg3, -1, arg3U, 1000);
    if (logger) logger->addLogRecord (fmt, arg1U, arg2U, arg3U);
}

void _logger (wchar_t *fmt, char *arg) {
    wchar_t argU [1000];
    MultiByteToWideChar (CP_ACP, 0, arg, -1, argU, 1000);
    if (logger) logger->addLogRecord (fmt, argU);
}

