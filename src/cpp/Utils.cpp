//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#include <algorithm>
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


std::string qq(const std::string& s) {
    std::stringstream ss;
    for (auto c: s) {
        if (c == '"') {
            ss << "\\";
        }
        ss << c;
    }
    return ss.str();
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


std::pair<std::string, std::string> split(const std::string& str, const std::string& delimiters) {
    size_t split_position = str.find_first_of(delimiters);
    if (split_position == std::string::npos) {
        return std::pair<std::string, std::string>(str, nullptr);
    } else {
        return std::pair<std::string, std::string>(str.substr(0, split_position), str.substr(split_position + 1));
    }
}


std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters) {
    std::vector<std::string> tokens;
    std::string token;

    size_t start = 0;
    size_t end = str.find_first_of(delimiters);

    while (start != std::string::npos || end != std::string::npos) {
        token = str.substr(start, end);
        tokens.push_back(token);
        start = end;
        end = str.find_first_not_of(delimiters, start);
    }

    return tokens;
}


bool is_only_digits(const std::string& s)
{
    return !s.empty() && s.find_first_not_of("0123456789") == std::string::npos;
}

bool sort_strings_numerically(const std::string& s1, const std::string& s2) {
    unsigned long long int d1, d2;
    std::vector<std::string> tokens1 = tokenize(s1, "0123456789");
    std::vector<std::string> tokens2 = tokenize(s2, "0123456789");

    std::vector<std::string>::const_iterator i1;
    std::vector<std::string>::const_iterator i2;

    for (i1 = tokens1.begin(), i2 = tokens2.begin(); i1 != tokens1.end() && i2 != tokens2.end(); ++i1, ++i2) {
        if (is_only_digits(*i1) && is_only_digits(*i2)) {
            d1 = std::stol(*i1);
            d2 = std::stol(*i2);
            if (d1 != d2) {
                return d1 < d2;
            }
        } else {
            if (*i1 != *i2) {
                return *i1 < *i2;
            }
        }
    }
    return s1 < s2;
}


std::string iso8601_timestamp() {
    char timestamp[22];
    std::time_t time = std::time(nullptr);
    std::strftime(timestamp, sizeof(timestamp), "%FT%TZ", std::gmtime(&time));
    return std::string(timestamp);
}

std::string wrap(std::string s, size_t length, size_t indent, std::string delimiters) {
    std::stringstream ss;

    size_t substr_length = length - indent;
    size_t string_length = s.length();
    std::string chunk;
    for (size_t start = 0, cut = substr_length;
         start < string_length;
         start = cut + 1, cut += substr_length) {

        cut = s.find_last_of(delimiters, cut);

        if (cut == std::string::npos || (cut < start || start + substr_length >= string_length)) {
            cut = start + substr_length;
        }

        chunk = s.substr(start, cut - start);

        for (size_t i = 0; i < indent; i++) {
            ss << " ";
        }

        ss << chunk << std::endl;
    }
    return ss.str();
}
