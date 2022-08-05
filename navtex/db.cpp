#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include "db.h"
#include "logger_helper.h"

struct GetParamCtx {
    const char *column;
    char value [100];
};

int getParamCb (void *param, int numOfCols, char **values, char **columns) {
    GetParamCtx *ctx = (GetParamCtx *) param;
    for (int i = 0; i < numOfCols; ++ i) {
        if (stricmp (columns [i], ctx->column) == 0) {
            strncpy (ctx->value, values [i], sizeof (ctx->value));
            return 0;
        }
    }
    return 1;
}

int getDbVersion (sqlite3 *db) {
    GetParamCtx ctx { "version", "" };
    int result;
    if (sqlite3_exec (db, "select version from parameters", getParamCb, "version", nullptr) == SQLITE_ABORT) {
        result = 0;
    } else {
        result = atoi (ctx.value);
    }
    _logger (L"Db version is %d", result);
    return result;
}

void upgradeDb (sqlite3 *db) {
    _logger (L"Upgrading the db...");
    sqlite3_exec (
        db,
        "create table if not exists messages ("
        " id integer primary key,"
        " subject integer not null,"
        " station integer not null,"
        " read integer not null,"
        " received real not null,"
        " text text not null"
        ")",
        nullptr,
        nullptr,
        nullptr
    );
    sqlite3_exec (
        db,
        "create unique index if not exists received on messages (received, id)",
        nullptr,
        nullptr,
        nullptr
    );
    sqlite3_exec (
        db,
        "create unique index if not exists by_subject on messages (subject, received, id)",
        nullptr,
        nullptr,
        nullptr
    );
    sqlite3_exec (
        db,
        "create table if not exists objects ("
        " id integer primary key,"
        " msg_id integer not null,"
        " number integer not null,"
        " lat real not null,"
        " lon real not null"
        ")",
        nullptr,
        nullptr,
        nullptr
    );
    sqlite3_exec (
        db,
        "create unique index if not exists objects_main on objects (msg_id, number)",
        nullptr,
        nullptr,
        nullptr
    );
}

void checkDb (sqlite3 *db) {
    if (getDbVersion (db) < CUR_VERSION) {
        _logger (L"Db is out of date.");
        upgradeDb (db);
    }
}

void addMessage (sqlite3 *db, MsgInfo *msg) {
    char query [500];
    sprintf (query, "insert into messages(subject,station,read,received,text) values(%d,%d,0,%lld,'%s')", msg->subject, msg->station, msg->receivedAt, msg->msg.c_str ());
    sqlite3_exec (db, query, nullptr, nullptr, nullptr);
}
