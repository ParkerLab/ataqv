#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <vector>

#include "HTS.hpp"
#include "Version.hpp"

std::string version_string();

std::string basename(const std::string& path, const std::string& ext = "");

float fraction(const float& numerator, const float& denominator);
float percentage(const float& numerator, const float& denominator);
std::string percentage_string(const unsigned long long int& numerator, const unsigned long long int& denominator, const int &precision = 3, const std::string& prefix = " (", const std::string& suffix = "%)");

template<typename ... Types>
std::string join(const std::string& separator, std::string first, Types ... rest);

std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters = " ");
bool sort_strings_numerically(const std::string& s1, const std::string& s2);
std::string get_qname(const bam1_t* record);
std::string record_to_string(const bam_hdr_t* header, const bam1_t* record);

std::string iso8601_timestamp();
#endif  // UTIL_HPP
