#pragma once

#include <vector>
#include <string>
#include <functional>
#include <cstdint>

class Sentence {
    public:
        Sentence (): sixBitEncoded (false), proprietary (false) {}
        bool parse (const char *source);

        std::string getAt (size_t field) {
            return fields.size () > field ? fields [field] : "";
        }
        int getAsIntAt (size_t field) {
            return omitted (field) ? 0 : atoi (getAt (field).c_str ());
        }
        double getAsDoubleAt (size_t field) {
            return omitted (field) ? 0.0 : atof (getAt (field).c_str ());
        }
        char getAsCharAt (size_t field) {
            return omitted (field) ? '\0' : getAt (field).front ();
        }
        bool omitted (size_t field) {
            return getAt (field).empty ();
        }
        size_t numOfFields () {
            return fields.size ();
        }

        char *extractAndParse (char *source);
        void extractAndParseAll (char *source, std::function<void (Sentence *)> cb);
        bool isSixBitEncoded () { return sixBitEncoded; }
        bool isProprietary () { return proprietary; }

    protected:
        std::vector<std::string> fields;
        bool sixBitEncoded, proprietary;

        uint8_t calcCRC (const char *source);
};