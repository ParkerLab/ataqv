//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iostream>
#include <map>
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


long double fraction(const long double& numerator, const long double& denominator) {
    return denominator == 0 ? std::nan("") : numerator / denominator;
}


std::string fraction_string(const long double& numerator, const long double& denominator, const int& precision) {
    std::stringstream ss;
    ss.precision(precision);
    ss << std::fixed;
    ss << fraction(numerator, denominator);
    return ss.str();
}


long double percentage(const long double& numerator, const long double& denominator) {
    return 100.0 * fraction(numerator, denominator);
}


std::string percentage_string(const long double& numerator, const long double& denominator, const int& precision, const std::string& prefix, const std::string& suffix) {
    std::stringstream ss;
    ss.precision(precision);
    ss << prefix << std::fixed;
    ss << percentage(numerator, denominator);
    ss << suffix;
    return ss.str();
}


std::vector<std::string> split(const std::string& str, const std::string& delimiters, bool keep_delimiters) {
    std::vector<std::string> result;
    result.reserve(10);

    size_t start = 0;
    while (true) {
        size_t split_position = str.find_first_of(delimiters, start);
        std::string token = slice(str, start, split_position);
        if (!token.empty()) {
            result.push_back(token);
        }
        if (split_position == std::string::npos) {
            break;
        }
        start = str.find_first_not_of(delimiters, split_position);
        if (keep_delimiters) {
            token = slice(str, split_position, start);
            if (!token.empty()) {
                result.push_back(token);
            }
        }
    }
    return result;
}


bool is_only_digits(const std::string& s)
{
    return !s.empty() && s.find_first_not_of("0123456789") == std::string::npos;
}


bool is_only_whitespace(const std::string& s)
{
    return (s.find_first_not_of(" \t\r\n") == std::string::npos);
}


bool sort_strings_numerically(const std::string& s1, const std::string& s2) {
    if (s1 == s2) {
        return false;
    }

    if (s1.empty()) {
        if (s2.empty()) {
            return false;
        } else {
            return true;
        }
    }

    std::string digits = "0123456789";

    std::vector<std::string> tokens1 = split(s1, digits, true);
    std::vector<std::string> tokens2 = split(s2, digits, true);

    for (int i = 0, l1 = tokens1.size(), l2 = tokens2.size(); i < l1; i++) {
        if (i >= l2) {
            return false;
        }

        std::string t1 = tokens1[i];
        std::string t2 = tokens2[i];

        if (is_only_digits(t1) && is_only_digits(t2)) {
            unsigned long long int d1 = std::stol(t1);
            unsigned long long int d2 = std::stol(t2);
            if (d1 != d2) {
                return d1 < d2;
            }
        } else {
            if (t1 != t2) {
                return t1 < t2;
            }
        }
    }

    return s1 < s2;
}


const std::string iso8601_timestamp(std::time_t* t) {
    char timestamp[22];
    std::time_t time = t ? *t : std::time(nullptr);
    std::strftime(timestamp, sizeof(timestamp), "%FT%TZ", std::gmtime(&time));
    return std::string(timestamp);
}


std::string slice(const std::string& s, size_t start, size_t end) {
    size_t cut = start <= s.size() ? start : s.size();
    size_t count = (end < s.size() ? end : std::string::npos) - cut;
    return s.substr(cut, count);
}


std::string wrap(const std::string& s, size_t length, size_t indent) {
    std::string whitespace = " \t\r\n";

    std::stringstream wrapped;

    std::vector<std::string> words = split(s, whitespace);
    size_t line_length = length - indent;
    size_t count = 0;
    for (size_t i = 0; i < indent; i++) {
        wrapped.put(' ');
    }
    count += indent;

    for (auto it = words.cbegin(); it != words.cend(); it++) {
        auto word = *it;

        if (is_only_whitespace(word)) {
            continue;
        }

        if (count + word.length() > line_length) {
            wrapped << '\n';
            count = 0;

            for (size_t i = 0; i < indent; i++) {
                wrapped.put(' ');
            }

            count += indent;
        }

        wrapped << (count == indent ? "" : " ") << word;
        count += word.length() + 1;
    }

    std::string result = wrapped.str();
    if (result.back() != '\n') {
        result.push_back('\n');
    }
    return result;
}


// Roman numerals.  #bioinfirmatics

std::vector<std::pair<std::string, int>> roman_to_integer_conversions = {
    {"M", 1000},
    {"CM", 900},
    {"D", 500},
    {"CD", 400},
    {"C", 100},
    {"XC", 90},
    {"X", 50},
    {"XL", 40},
    {"X", 10},
    {"IX", 9},
    {"V", 5},
    {"IV", 4},
    {"I", 1}
};

std::string integer_to_roman(int i) {
    std::string roman;
    for (auto t : roman_to_integer_conversions) {
        while (i >= t.second) {
            roman += t.first;
            i -= t.second;
        }
    }
    return roman;
}

int roman_to_integer(const std::string& roman) {
    int integer = 0;
    int p = 0;
    for (auto t : roman_to_integer_conversions) {
        int rnl = t.first.length();
        while (roman.substr(p, rnl) == t.first) {
            integer += t.second;
            p += rnl;
        }
    }
    return integer;
}

bool is_roman_numeral(std::string s) {
    return roman_to_integer(s) > 0;
}

bool sort_strings_with_roman_numerals(const std::string& s1, const std::string& s2) {
    if (s1 == s2) {
        return false;
    }

    if (s1.empty()) {
        if (s2.empty()) {
            return false;
        } else {
            return true;
        }
    }

    std::string digits = "0123456789CDILMVX";

    std::vector<std::string> tokens1 = split(s1, digits, true);
    std::vector<std::string> tokens2 = split(s2, digits, true);

    for (int i = 0, l1 = tokens1.size(), l2 = tokens2.size(); i < l1; i++) {
        if (i >= l2) {
            return false;
        }

        std::string t1 = tokens1[i];
        std::string t2 = tokens2[i];

        if (is_roman_numeral(t1) && is_roman_numeral(t2)) {
            int d1 = roman_to_integer(t1);
            int d2 = roman_to_integer(t2);
            if (d1 != d2) {
                return d1 < d2;
            }
        } else if (is_only_digits(t1) && is_only_digits(t2)) {
            unsigned long long int d1 = std::stol(t1);
            unsigned long long int d2 = std::stol(t2);
            if (d1 != d2) {
                return d1 < d2;
            }
        } else {
            if (t1 != t2) {
                return t1 < t2;
            }
        }
    }

    return s1 < s2;
}
