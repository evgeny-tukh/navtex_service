#pragma once

#ifdef _NMEA_INTERNAL_
    #define NMEA_API __declspec (dllexport)
#else
    #define NMEA_API __declspec (dllimport)
#endif
