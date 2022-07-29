#pragma once

#ifdef _NAVTEX_INTERNAL_
    #define NAVTEX_API __declspec (dllexport)
#else
    #define NAVTEX_API __declspec (dllimport)
#endif
