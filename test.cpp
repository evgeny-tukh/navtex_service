#include <windows.h>
#include "navtex/navtex.h"
#include <stdio.h>
#include <chrono>
#include <thread>

extern "C" void onMsgRead (wchar_t *ids);

void onMsgRead (wchar_t *ids) {
    wprintf (L"Messages received: %s\n", ids);
}

int main (int argCount, char *args []) {
    printf ("Navtex test tool\n");

    ReloadSettings ();
    SetMsgCb (onMsgRead);

    while (true) {
        std::this_thread::sleep_for (std::chrono::seconds (5));
        printf ("5 sec passed...\n");
    }

    exit (0);
}