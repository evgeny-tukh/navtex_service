#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <tuple>
#include <optional>
#include "db.h"
#include "logger_helper.h"

struct GetParamCtx {
    const char *column;
    std::optional<std::string> value;
};

int getParamCb (void *param, int numOfCols, char **values, char **columns) {
    GetParamCtx *ctx = (GetParamCtx *) param;
    for (int i = 0; i < numOfCols; ++ i) {
        if (stricmp (columns [i], ctx->column) == 0) {
            ctx->value = values [i];
            return 0;
        }
    }
    return 1;
}

std::tuple<bool, std::string> getDbParam (sqlite3 *db, char *paramName) {
    GetParamCtx ctx { "version" };
    char query [500];
    sprintf (query, "select prop_value from settings where prop_name='%s'", paramName);
    if (sqlite3_exec (db, query, getParamCb, & ctx, nullptr) == SQLITE_ABORT || !ctx.value.has_value ()) {
        return std::tuple<bool, std::string> (false, "");
    } else {
        return std::tuple<bool, std::string> (true, ctx.value.value ());
    }
}

int getDbVersion (sqlite3 *db) {
    auto result = getDbParam (db, "version");
    int version = std::get<0> (result) ? std::stoi (std::get<1> (result)) : 0;
    _logger (L"Db version is %d", version);
    return version;
}

bool setDbParam (sqlite3 *db, char *paramName, char *paramValue) {
    char query [500];
    if (std::get<0> (getDbParam (db, paramName))) {
        sprintf (query, "update settings set prop_value='%s' where prop_name='%s'", paramValue, paramName);
    } else {
        sprintf (query, "insert into settings(prop_name,prop_value) values('%s','%s')", paramName, paramValue);
    }
    bool result = sqlite3_exec (db, query, nullptr, nullptr, nullptr) != SQLITE_ABORT;
    _logger (L"Setting '%s' to '%s': %s", paramName, paramValue, result ? "ok" : "fail");
    return result;
}

void upgradeDb (sqlite3 *db, int version) {
    _logger (L"Upgrading the db...");
    if (version < 1) {
        sqlite3_exec (
            db,
            "create table if not exists messages ("
            " id integer primary key,"
            " subject integer(1) not null,"
            " station integer(1) not null,"
            " read integer(1) not null,"
            " received real not null,"
            " text text not null"
            ")",
            nullptr,
            nullptr,
            nullptr
        );
        sqlite3_exec (db, "create unique index if not exists received on messages (received, id)", nullptr, nullptr, nullptr);
        sqlite3_exec (db, "create unique index if not exists by_subject on messages (subject, received, id)", nullptr, nullptr, nullptr);
        sqlite3_exec (
            db,
            "create table if not exists objects ("
            " id integer primary key,"
            " msg_id integer not null,"
            " number integer(2) not null,"
            " lat real not null,"
            " lon real not null"
            ")",
            nullptr,
            nullptr,
            nullptr
        );
        sqlite3_exec (db, "create unique index if not exists objects_main on objects (msg_id, number)", nullptr, nullptr, nullptr);
        sqlite3_exec (
            db,
            "create table if not exists settings ("
            " id integer primary key,"
            " prop_name text not null,"
            " prop_value text not null"
            ")",
            nullptr,
            nullptr,
            nullptr
        );
        sqlite3_exec (db, "create unique index if not exists settings_main on settings (prop_name)", nullptr, nullptr, nullptr);
    }
    if (version < 2) {
        sqlite3_exec (db, "alter table messages add column source integer(1) not null default 1", nullptr, nullptr, nullptr);
        sqlite3_exec (db, "alter table messages add column priority integer(1) not null default 1", nullptr, nullptr, nullptr);
        sqlite3_exec (db, "alter table messages add column in_force integer(1) not null default 1", nullptr, nullptr, nullptr);
        sqlite3_exec (db, "alter table messages add column cancelled real", nullptr, nullptr, nullptr);
        sqlite3_exec (db, "alter table messages add column sent integer(1) not null default 0", nullptr, nullptr, nullptr);
    }
}

void checkDb (sqlite3 *db) {
    auto version = getDbVersion (db);
    if (version < CUR_VERSION) {
        _logger (L"Db is out of date.");
        upgradeDb (db, version);
        setDbParam (db, "version", std::to_string (CUR_VERSION).data ());
    }
}

void addMessage (sqlite3 *db, MsgInfo *msg) {
    static const char *MSG_FMT { "insert into messages(subject,station,read,received,text,source,priority,in_force,cancelled,sent) values(%d,%d,0,%lld,'%s',1,1,1,null,0)" };
    static const char *POS_FMT { "insert into objects(msg_id,number,lat,lon) values(%d,%d,%f,%f)" };
    char query [500];
    sprintf (query, MSG_FMT, msg->subject, msg->station, msg->receivedAt, msg->msg.c_str ());
    sqlite3_exec (db, query, nullptr, nullptr, nullptr);
    auto msgID = sqlite3_last_insert_rowid (db);
    for (size_t i = 0; i < msg->positions.size (); ++ i) {
        auto& pos = msg->positions [i];
        sprintf (query, POS_FMT, msgID, (int) i, pos.first, pos.second);
        sqlite3_exec (db, query, nullptr, nullptr, nullptr);
    }
}
