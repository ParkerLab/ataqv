#ifndef UTILS_HPP
#define UTILS_HPP

#include <ctime>
#include <string>
#include <vector>

#include "Version.hpp"

std::string version_string();

std::string basename(const std::string& path, const std::string& ext = "");

std::string qq(const std::string& s);

long double fraction(const long double& numerator, const long double& denominator);
std::string fraction_string(const long double& numerator, const long double& denominator, const int& precision = 3);

long double percentage(const long double& numerator, const long double& denominator);
std::string percentage_string(const long double& numerator, const long double& denominator, const int &precision = 3, const std::string& prefix = " (", const std::string& suffix = "%)");

std::vector<std::string> split(const std::string& str, const std::string& delimiters = " ", bool keep_delimiters = false);

bool is_only_digits(const std::string& s);
bool is_only_whitespace(const std::string& s);

bool sort_strings_numerically(const std::string& s1, const std::string& s2);

// comparison object for containers
struct numeric_string_comparator {
    bool operator() (const std::string& p1, const std::string& p2) const {
        return sort_strings_numerically(p1, p2);
    }
};

const std::string iso8601_timestamp(std::time_t* t = nullptr);

std::string slice(const std::string& s, size_t start, size_t end = std::string::npos);

std::string wrap(const std::string& s, size_t length = 72, size_t indent=0);

std::string integer_to_roman(int i);
int roman_to_integer(const std::string& roman);
bool is_roman_numeral(std::string s);
bool sort_strings_with_roman_numerals(const std::string& s1, const std::string& s2);

#endif  // UTILS_HPP
