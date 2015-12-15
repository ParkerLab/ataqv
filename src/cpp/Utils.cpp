//
// Copyright 2015 The Parker Lab at the University of Michigan
//
// Licensed under Version 3 of the GPL or any later version
//

#include <iostream>
#include <sstream>

#include "Utils.hpp"

std::string version_string() {
    std::stringstream ss;
    ss << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH;
    return ss.str();
}

std::string basename(const std::string& path) {
    return path.substr(path.find_last_of("/\\") + 1);
}

float fraction(const float& numerator, const float& denominator) {
    return(denominator == 0 ? 0.0 : (numerator / denominator));
}

float percentage(const float& numerator, const float& denominator) {
    return(denominator == 0 ? 0.0 : (100.0 * numerator / denominator));
}

std::string percentage_string(const ull& numerator, const ull& denominator, const int& precision, const std::string& prefix, const std::string& suffix) {
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
    for (i1 = t1.begin(), i2 = t2.begin(); i1 != t1.end() && i2 != t2.end(); i1++, i2++) {
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
};

std::string record_to_string(const bam_hdr_t* header, const bam1_t* record) {
        kstring_t record_string;
        record_string.l = record_string.m = 0;
        record_string.s = NULL;
        sam_format1(header, record, &record_string);
        return std::string(record_string.s);
}
