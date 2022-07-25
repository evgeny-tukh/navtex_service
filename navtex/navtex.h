#pragma once

extern "C" {

int __declspec (dllexport) StartNavtexReceiver (wchar_t *pathToDb);
void __declspec (dllexport) StopNavtexReceiver ();
void __declspec( dllexport) ReloadSettings ();
void __declspec (dllexport) StartLog (wchar_t *file);
void __declspec (dllexport) StopLog ();

}
