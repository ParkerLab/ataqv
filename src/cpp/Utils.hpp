#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <vector>

#include "HTS.hpp"
#include "Types.hpp"
#include "Version.hpp"

std::string version_string();

std::string basename(const std::string& path);

float fraction(const float& numerator, const float& denominator);
float percentage(const float& numerator, const float& denominator);
std::string percentage_string(const ull& numerator, const ull& denominator, const int &precision = 3, const std::string& prefix = " (", const std::string& suffix = "%)");

template<typename ... Types>
std::string join(const std::string& separator, std::string first, Types ... rest);

std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters = " ");
bool sort_strings_numerically(const std::string& s1, const std::string& s2);
std::string record_to_string(const bam_hdr_t* header, const bam1_t* record);
#endif  // UTIL_HPP
