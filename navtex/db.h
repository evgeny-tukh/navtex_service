#pragma once

#include <tuple>

#include "../sqlite/sqlite3.h"

const int CUR_VERSION = 1;

extern struct sqlite3 *db;

void checkDb (sqlite3 *db);
uint64_t addMessage (sqlite3 *db, struct MsgInfo *msg);
std::tuple<int, int> removeExpiredMessages (sqlite3 *db, int numOfHours);
