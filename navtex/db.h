#pragma once

#include "../sqlite/sqlite3.h"

const int CUR_VERSION = 1;

void checkDb (sqlite3 *db);
uint64_t addMessage (sqlite3 *db, struct MsgInfo *msg);
