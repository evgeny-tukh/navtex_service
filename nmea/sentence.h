#pragma once

#include <vector>
#include <string>
#include <cstdint>

class Sentence {
    public:
        void parse (const char *source);

        std::string getAt (size_t field) { return fields.size () > field ? fields [field] : "" }
        int getAsIntAt (size_t field) { return atoi (getAt (field).c_str ()); }
        double getAsDoubleAt (size_t field) { return atof (getAt (field).c_str ()); }
        char getAsCharAt (size_t field) { return getAt (field).front (); }
        bool omitted (size_t field) { return getAt (field).empty (); }
        size_t numOfFields () { return fields.size (); }

    protected:
        std::vector<std::string> fields;

        uint8_t calcCRC (const char *source);
};