//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#include <ctime>
#include <iostream>
#include <sstream>
#include <string>

#include "Utils.hpp"

std::string version_string() {
    std::stringstream ss;
    ss << VERSION;
    return ss.str();
}

std::string basename(const std::string& path, const std::string& ext) {
    std::string result(path.substr(path.find_last_of("/\\") + 1));
    if (!ext.empty()) {
        result = result.substr(0, result.rfind(ext));
    }
    return result;
}

float fraction(const float& numerator, const float& denominator) {
    return(denominator == 0 ? 0.0 : (numerator / denominator));
}

float percentage(const float& numerator, const float& denominator) {
    return(denominator == 0 ? 0.0 : (100.0 * numerator / denominator));
}

std::string percentage_string(const unsigned long long int& numerator, const unsigned long long int& denominator, const int& precision, const std::string& prefix, const std::string& suffix) {
    std::stringstream s;
    s.precision(precision);
    s << numerator << prefix << std::fixed << percentage(numerator, denominator) << suffix;
    return s.str();
}

template<typename ... Types>
std::string join(const std::string& separator, std::string first, Types ... rest) {
    return first + separator + join(separator, rest...);
}

std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters) {
    std::vector<std::string> tokens;
    std::string token;

    size_t start = 0;
    size_t end = str.find_first_of(delimiters);
    //
    while (start != std::string::npos || end != std::string::npos) {
        token = str.substr(start, end);
        tokens.push_back(token);
        start = end;
        end = str.find_first_not_of(delimiters, start);
    }
    return tokens;
}

bool sort_strings_numerically(const std::string& s1, const std::string& s2) {
    long double d1, d2;
    std::vector<std::string> t1 = tokenize(s1, "0123456789");
    std::vector<std::string> t2 = tokenize(s2, "0123456789");

    std::vector<std::string>::const_iterator i1;
    std::vector<std::string>::const_iterator i2;
    for (i1 = t1.begin(), i2 = t2.begin(); i1 != t1.end() && i2 != t2.end(); ++i1, ++i2) {
        try {
            d1 = std::stold(*i1);
            d2 = std::stold(*i2);
            if (d1 < d2) {
                return true;
            }
            if (d1 > d2) {
                return false;
            }
        } catch (...) {
            if (*i1 < *i2) {
                return true;
            }
            if (*i1 > *i2) {
                return false;
            }
        }
    }
    return s1 < s2;
}

std::string get_qname(const bam1_t* record) {
    return std::string(record && record->data ? ((char *)(record)->data) : "");
}

std::string record_to_string(const bam_hdr_t* header, const bam1_t* record) {
    std::string reference_name(record->core.tid >= 0 ? header->target_name[record->core.tid] : "*");
    std::string mate_reference_name(record->core.mtid >= 0 ? header->target_name[record->core.mtid] : "*");

    std::stringstream s;
    s << get_qname(record)
      << "\t" << record->core.flag
      << "\t" << reference_name
      << "\t" << record->core.pos + 1
      << "\t" << mate_reference_name
      << "\t" << record->core.mpos + 1
      << "\t" << record->core.isize;
    return s.str();
}

std::string iso8601_timestamp() {
    char timestamp[22];
    std::time_t time = std::time(nullptr);
    std::strftime(timestamp, sizeof(timestamp), "%FT%TZ", std::gmtime(&time));
    return std::string(timestamp);
}
