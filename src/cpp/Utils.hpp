#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <vector>

#include "Version.hpp"

std::string version_string();

std::string basename(const std::string& path, const std::string& ext = "");

std::string qq(const std::string& s);

float fraction(const float& numerator, const float& denominator);
float percentage(const float& numerator, const float& denominator);
std::string percentage_string(const unsigned long long int& numerator, const unsigned long long int& denominator, const int &precision = 3, const std::string& prefix = " (", const std::string& suffix = "%)");

std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters = " ");
std::pair<std::string, std::string> split(const std::string& str, const std::string& delimiters);
bool sort_strings_numerically(const std::string& s1, const std::string& s2);

std::string iso8601_timestamp();

std::string wrap(std::string s, size_t length = 72, size_t indent=0, std::string delimiters = " \t\n.,-");

#endif  // UTIL_HPP
