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


std::string fraction_string(const double& numerator, const double& denominator, const int& precision) {
    std::stringstream ss;
    ss.precision(precision);
    ss << std::fixed;
    if (denominator == 0) {
        ss << "undefined";
    } else {
        ss << (numerator / denominator);
    }
    return ss.str();
}


std::string percentage_string(const double& numerator, const double& denominator, const int& precision, const std::string& prefix, const std::string& suffix) {
    std::stringstream ss;
    ss.precision(precision);
    ss << prefix << std::fixed;
    if (denominator == 0) {
        ss << "undefined";
    } else {
        ss << (100.0 * numerator / denominator);
    }
    ss << suffix;
    return ss.str();
}


std::pair<std::string, std::string> split(const std::string& str, const std::string& delimiters) {
    size_t split_position = str.find_first_of(delimiters);
    if (split_position == std::string::npos) {
        return std::pair<std::string, std::string>(str, "");
    } else {
        return std::pair<std::string, std::string>(str.substr(0, split_position), str.substr(split_position + 1));
    }
}


std::vector<std::string> tokenize(const std::string& s, const std::string& delimiters) {
    std::vector<std::string> tokens;

    if (!s.empty()) {
        if (delimiters.empty() || s.find_first_of(delimiters) == std::string::npos) {
            tokens.push_back(s);
        } else {
            const char& front = s[0];
            size_t front_in_delimiters = delimiters.find(&front);
            bool in_delimiter = (front_in_delimiters != std::string::npos);

            std::string token;
            auto last = --s.cend();
            for (auto it = s.cbegin(); it != s.cend(); ++it) {
                char c = *it;
                if (delimiters.find(c) == std::string::npos) {
                    if (!in_delimiter) {
                        token.push_back(c);
                    } else {
                        if (!token.empty()) {
                            tokens.push_back(token);
                        }
                        token = c;
                    }
                    in_delimiter = false;
                } else {
                    if (in_delimiter) {
                        token.push_back(c);
                    } else {
                        if (!token.empty()) {
                            tokens.push_back(token);
                        }
                        token = c;
                    }
                    in_delimiter = true;
                }

                if (!token.empty() && it == last) {
                    tokens.push_back(token);
                }
            }
        }
    }

    return tokens;
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

    if (s1.find_first_of(digits) != std::string::npos &&
        s2.find_first_of(digits) != std::string::npos) {

        std::vector<std::string> tokens1 = tokenize(s1, digits);
        std::vector<std::string> tokens2 = tokenize(s2, digits);

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
    return s.substr(start, (end - start));
}


std::string wrap(const std::string& s, size_t length, size_t indent) {
    std::string whitespace = " \t\r\n";

    std::stringstream wrapped;

    std::vector<std::string> words = tokenize(s, whitespace);

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
