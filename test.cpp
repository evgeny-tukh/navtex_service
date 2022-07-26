#include <windows.h>
#include <Shlwapi.h>
#include "navtex/navtex.h"
#include <stdio.h>
#include <chrono>
#include <thread>

extern "C" void onMsgRead (wchar_t *ids, wchar_t *text);

void onMsgRead (wchar_t *ids, wchar_t *text) {
    wprintf (L"Messages received: %s\n\n===========================================\n%s\n===========================================", ids, text);
}

int main (int argCount, char *args []) {
    wchar_t path [MAX_PATH];
    GetModuleFileNameW (nullptr, path, MAX_PATH);
    PathRemoveFileSpecW (path);
    PathAppendW (path, L"store.db");

    printf ("Navtex test tool\n");

    ReloadSettings ();
    SetMsgAddCb2 (onMsgRead);
    StartNavtexReceiver (path);

    while (true) {
        std::this_thread::sleep_for (std::chrono::seconds (5));
    }

    exit (0);
}