#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "defs.h"

typedef void (*MsgCb) (wchar_t *);

int NAVTEX_API StartNavtexReceiver (wchar_t *pathToDb);
void NAVTEX_API StopNavtexReceiver ();
void NAVTEX_API ReloadSettings ();
void NAVTEX_API StartLog (wchar_t *file);
void NAVTEX_API StopLog ();
void NAVTEX_API SetMsgAddCb (MsgCb cb);
void NAVTEX_API SetMsgRemoveCb (MsgCb cb);

#ifdef __cplusplus
}
#endif
