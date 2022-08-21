#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "defs.h"

typedef void (*MsgCb) (wchar_t *);
typedef void (*MsgCb2) (wchar_t *, wchar_t *);

int NAVTEX_API StartNavtexReceiver (wchar_t *pathToDb);
void NAVTEX_API StopNavtexReceiver ();
void NAVTEX_API ReloadSettings ();
void NAVTEX_API StartLog (wchar_t *file);
void NAVTEX_API StopLog ();
void NAVTEX_API SetMsgAddCb (MsgCb cb);
void NAVTEX_API SetMsgAddCb2 (MsgCb2 cb);
void NAVTEX_API SetMsgRemoveCb (MsgCb cb);

#ifdef __cplusplus
}
#endif

#define MSG_HEAD                "ZCZC"
#define MSG_TAIL                "NNNN"
#define MSG_HEAD_SIZE           4
#define MSG_TAIL_SIZE           4

// Area numbers
#define AREA_UK             1
#define AREA_FRANCE         2
#define AREA_SPAIN          3
#define AREA_USA_EAST       4
#define AREA_BRAZIL         5
#define AREA_ARGENTINA      6
#define AREA_SOUTH_AFRICA   7
#define AREA_INDIA          8
#define AREA_PAKISTAN       9
#define AREA_AUSTRALIA      10
#define AREA_JAPAN          11
#define AREA_USA_WEST       12
#define AREA_RUSSIA         13
#define AREA_NEW_ZEALAND    14
#define AREA_CHILE          15
#define AREA_PERU           16

// International messages            
#define MSG_NAV_WARNING                 'A' // Navigational warnings 
#define MSG_METEO_WARNING               'B' // Meteorological warnings 
#define MSG_ICE_REPORT                  'C' // Ice reports 
#define MSG_SEARCH_AND_RESQUE           'D' // Search and rescue information 
#define MSG_METEO_FORECAST              'E' // Meteorological forecasts 
#define MSG_PILOT_SERVICE               'F' // Pilot service messages 
#define MSG_DECCA                       'G' // DECCA messages 
#define MSG_LORAN                       'H' // LORAN messages 
#define MSG_AVAILABLE                   'I' // Available 
#define MSG_SATNAV                      'J' // SATNAV messages 
#define MSG_OTHER_ELECTONIC_NAV_AID     'K' // Other electronic navigational aid messages 
#define MSG_ADDITIONAL_NAV_WARNING      'L' // Navigational warnings in addition to A 
#define MSG_SPEC_SERVICE_V              'V' // Special services 
#define MSG_SPEC_SERVICE_W              'W' // Special services 
#define MSG_SPEC_SERVICE_X              'X' // Special services 
#define MSG_SPEC_SERVICE_Y              'Y' // Special services 
#define MSG_NO_MESSAGES_ON_HAND         'Z' // No messages on hand 

// UK specific
#define MSG_MOB_DRILLING_RIG_MOVEMENT   'A' // includes mobile drilling rig movements 
#define MSG_PIRACY_WARNING              'D' // includes piracy and armed robbery warnings 
#define MSG_SUBFACTS_AND_GUNFACTS       'L' // includes Subfacts (submarine exercise) and Gunfacts (firing range) information 
#define MSG_NAV_WARNING_AMPLIFYING      'V' // Amplifying navigational warning information initially announced under A 

// USA specific
#define MSG_NO_PILOT_INFO               'F' // No pilot information is transmitted 
#define MSG_NOTICE_TO_FISHERMEN         'V' // Notices to fishermen 
#define MSG_ENVIRONMENTAL               'W' // Environmental 

// Iceland specific
#define MSG_ICELAND_SPECIFIC            'X' // In Icelandic – weather bulletins, navigational warnings, notices to fishermen and telecomms traffic 

// Suspicious messages
#define MSG_SUSPICIOUS                  '['
