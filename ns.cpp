#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <windows.h>
#include <Shlwapi.h>
#include <string>

const char *SVC_NAME = "NavtexService";
const char *SVC_DISPLAY_NAME = "Navtex Service";
const int SVC_ERROR = 1000;

SERVICE_STATUS svcStatus; 
SERVICE_STATUS_HANDLE svcStatusHandle; 
HANDLE svcStopEvent = nullptr;

bool isAdmin () {
    BOOL admin = false;
    unsigned long error = ERROR_SUCCESS;
    PSID adminGroup = 0;

    // Allocate and initialize a SID of the administrators group.
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid (& ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, & adminGroup)) {
        error = GetLastError ();
    } else {
        // Determine whether the SID of administrators group is enabled in 
        // the primary access token of the process.
        if (!CheckTokenMembership (0, adminGroup, & admin)) {
            error = GetLastError ();
        }
    }

    // Centralized cleanup for all allocated resources.
    if (adminGroup) {
        FreeSid (adminGroup);
        adminGroup = 0;
    }

    // Throw the error if something failed in the function.
    if (ERROR_SUCCESS != error) throw error;

    return admin;
}

bool checkElevate (bool *exiting, char *cmdLine) {
    bool result;

    if (isAdmin ()) {
        if (exiting) *exiting = false;
        
        result = true;
    } else {
        char path [MAX_PATH];

        GetModuleFileName (0, path, sizeof (path));

        // Launch itself as admin
        SHELLEXECUTEINFO info;
        
        memset (& info, 0, sizeof (info));

        info.cbSize = sizeof (info);
        info.lpVerb = "runas";
        info.lpFile = path;
        info.lpParameters = cmdLine;
        info.nShow = SW_NORMAL;
        
        result = ShellExecuteEx (& info) == ERROR_SUCCESS;

        if (exiting) *exiting = true;

        exit (0);
    }

    return result;
}

void log (char *string) {
    printf (string);
}

void log (char *fmt, unsigned long arg) {
    printf (fmt, arg);
}

void log (char *fmt, const char *arg) {
    printf (fmt, arg);
}

void logLastError (char *fmt) {
    std::string fmtStr { fmt };
    printf ((fmtStr + ", error %d\n").c_str (), GetLastError ());
}

void installService (bool autoMode) {
    SC_HANDLE scmHandle;
    SC_HANDLE serviceHandle;
    char binaryPath [MAX_PATH];

    if (!GetModuleFileName (nullptr, binaryPath, sizeof (binaryPath))) {
        logLastError ("Cannot install service");
        exit (0);
    }

    scmHandle = OpenSCManager (nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
 
    if (!scmHandle) {
        logLastError ("Unable to open SCM");
        exit (0);
    }

    PathQuoteSpaces (binaryPath);

    serviceHandle = CreateService (
        scmHandle,                                              // SCM database 
        SVC_NAME,                                               // name of service 
        SVC_DISPLAY_NAME,                                       // service name to display 
        SERVICE_ALL_ACCESS,                                     // desired access 
        SERVICE_WIN32_OWN_PROCESS,                              // service type 
        autoMode ? SERVICE_AUTO_START : SERVICE_DEMAND_START,   // start type 
        SERVICE_ERROR_NORMAL,                                   // error control type 
        binaryPath,                                             // path to service's binary 
        nullptr,                                                // no load ordering group 
        nullptr,                                                // no tag identifier 
        nullptr,                                                // no dependencies 
        nullptr,                                                // LocalSystem account 
        nullptr                                                 // no password 
    );                      
 
    if (!serviceHandle) {
        logLastError ("CreateService failed"); 
        CloseServiceHandle (scmHandle);
        exit (0);
    }

    CloseServiceHandle (serviceHandle); 
    CloseServiceHandle (scmHandle);

    log ("Service has been installed%s.\n", autoMode ? " in auto mode" : "");
}

void uninstallService () {
    auto manager = OpenSCManager (nullptr, nullptr, SC_MANAGER_ALL_ACCESS);

    if (manager) {
        auto service = OpenService (manager, SVC_NAME, SC_MANAGER_ALL_ACCESS);

        if (service) {
            DeleteService (service);
            CloseServiceHandle (service);
            log ("Service has been uninstalled.\n");
        } else {
            logLastError ("Unable to delete service");
        }
        CloseServiceHandle (manager);
    } else {
        logLastError ("Unable to open SCM");
    }
}

void startService () {
    printf ("Service has been started.\n");
}

void stopService () {
    printf ("Service has been stopped.\n");
}

void showUsageAndExit () {
    printf (
        "\nUSAGE:\n"
        "\tNS options\n"
        "where options could be:\n"
        "\t-h\t shows the help\n"
        "\t-i[a]\t installs the service (in auto mode if 'a' present)\n"
        "\t-u\t uinstalls the service\n"
        "\t-s\t starts the service\n"
        "\t-p\t stops the service\n"
    );
    exit (0);
}

void ReportSvcStatus (unsigned long currentState, unsigned long win32ExitCode, unsigned long waitHint) {
    static unsigned long checkPoint = 1;

    svcStatus.dwCurrentState = currentState;
    svcStatus.dwWin32ExitCode = win32ExitCode;
    svcStatus.dwWaitHint = waitHint;

    if (currentState == SERVICE_START_PENDING) {
        svcStatus.dwControlsAccepted = 0;
    } else {
        svcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }

    if (currentState == SERVICE_RUNNING || currentState == SERVICE_STOPPED) {
        svcStatus.dwCheckPoint = 0;
    } else {
        svcStatus.dwCheckPoint = checkPoint ++;
    }

    SetServiceStatus (svcStatusHandle, & svcStatus);
}

void WINAPI svcCtrlHandler (unsigned long ctrl) {
    switch (ctrl) {  
        case SERVICE_CONTROL_STOP: {
            ReportSvcStatus (SERVICE_STOP_PENDING, NO_ERROR, 0);
            SetEvent (svcStopEvent);
            ReportSvcStatus (svcStatus.dwCurrentState, NO_ERROR, 0);
            break;
        }
        case SERVICE_CONTROL_INTERROGATE: {
            break; 
        }
        default: {
            break;
        }
   } 
}

void svcInit (unsigned long argCount, char *args []) {
    svcStopEvent = CreateEvent (nullptr, TRUE, FALSE, nullptr);

    if  (!svcStopEvent) {
        ReportSvcStatus (SERVICE_STOPPED, GetLastError (), 0);
        return;
    }

    ReportSvcStatus (SERVICE_RUNNING, NO_ERROR, 0);

    while (true) {
        WaitForSingleObject (svcStopEvent, INFINITE);
        ReportSvcStatus (SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }
}

void WINAPI svcMain (unsigned long argCount, char *args []) {
    auto handler = RegisterServiceCtrlHandler (SVC_NAME, svcCtrlHandler);

    if (!handler) {
        logLastError ("Unable to register service control handler, error %d\n");
        exit (0);
    }

    svcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
    svcStatus.dwServiceSpecificExitCode = 0;    

    ReportSvcStatus (SERVICE_START_PENDING, NO_ERROR, 3000);

    svcInit (argCount, args);
}

void svcReportEvent (char *function) 
{ 
    HANDLE eventSource;
    const char *strings [2];
    char buffer [80];

    eventSource = RegisterEventSource (nullptr, SVC_NAME);

    if (eventSource) {
        sprintf (buffer, "%s failed with %d", function, GetLastError ());

        strings [0] = SVC_NAME;
        strings [1] = buffer;

        ReportEvent (
            eventSource,                // event log handle
            EVENTLOG_ERROR_TYPE,        // event type
            0,                          // event category
            SVC_ERROR,                  // event identifier
            nullptr,                    // no security identifier
            2,                          // size of lpszStrings array
            0,                          // no binary data
            strings,                    // array of strings
            nullptr                     // no binary data
        );               
        DeregisterEventSource (eventSource);
    }
}

int main (int argCount, char *args []) {
    bool exiting;

    //checkElevate (& exiting, GetCommandLine ());

    printf ("Navtex ServuiceTool\n");

    if (argCount < 2) showUsageAndExit ();

    for (int i = 1; i < argCount; ++ i) {
        char *arg = args [i];

        if (arg [0] != '-' && arg [0] != '/') showUsageAndExit ();

        switch (toupper (arg [1])) {
            case 'I': installService (toupper (arg[2]) == 'A'); exit (0);
            case 'U': uninstallService (); exit (0);
            case 'S': startService (); break;
            case 'P': stopService (); break;
            case 'H':
            default:  showUsageAndExit ();
        }
    }

    SERVICE_TABLE_ENTRY dispatchTable [] { 
        { (char *) SVC_NAME, (SERVICE_MAIN_FUNCTION *) svcMain }, 
        { nullptr, nullptr } 
    }; 

    if (!StartServiceCtrlDispatcher (dispatchTable)) { 
        log ("Unable to start StartServiceCtrlDispatcher");
        exit (0);
    }

    return 0;
}
