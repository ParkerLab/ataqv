//
// Copyright 2015 The Parker Lab at the University of Michigan
//
// Licensed under Version 3 of the GPL or any later version
//

#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <vector>

#include "Types.hpp"


float fraction(const float& numerator, const float& denominator);
float percentage(const float& numerator, const float& denominator);
std::string percentage_string(const ull& numerator, const ull& denominator, const std::string& prefix = " (", const std::string& suffix = "%)");

template<typename ... Types>
std::string join(const std::string& separator, std::string first, Types ... rest);

std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters = " ");
bool sort_strings_numerically(const std::string& s1, const std::string& s2);
#endif  // UTIL_HPP
