#pragma once

#include "../log/log.h"

extern Logger *logger;

void _logger (wchar_t *msg);
void _logger (wchar_t *fmt, wchar_t *arg);
void _logger (wchar_t *fmt, int arg);
void _logger (wchar_t *fmt, int arg1, int arg2);
void _logger (wchar_t *fmt, char *arg);
void _logger (wchar_t *fmt, char *arg1, char *arg2);
void _logger (wchar_t *fmt, char *arg1, char *arg2, char *arg3);
