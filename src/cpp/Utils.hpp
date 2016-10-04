#ifndef UTIL_HPP
#define UTIL_HPP

#include <ctime>
#include <string>
#include <vector>

#include "Version.hpp"

std::string version_string();

std::string basename(const std::string& path, const std::string& ext = "");

std::string qq(const std::string& s);

std::string fraction_string(const double& numerator, const double& denominator, const int& precision = 3);
std::string percentage_string(const double& numerator, const double& denominator, const int &precision = 3, const std::string& prefix = " (", const std::string& suffix = "%)");

std::pair<std::string, std::string> split(const std::string& str, const std::string& delimiters = " ");
std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters = " ");

bool is_only_digits(const std::string& s);
bool is_only_whitespace(const std::string& s);

bool sort_strings_numerically(const std::string& s1, const std::string& s2);

const std::string iso8601_timestamp(std::time_t* t = nullptr);

std::string slice(std::string s, size_t start, size_t end = std::string::npos);

std::string wrap(std::string s, size_t length = 72, size_t indent=0);

#endif  // UTIL_HPP
