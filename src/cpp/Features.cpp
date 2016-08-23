//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#include <iostream>
#include <sstream>

#include "Features.hpp"


bool operator< (const Feature& a, const Feature& b) {
    return a.start < b.start;
}

unsigned long long int Feature::size() const {
    return end - start;
}

std::ostream& operator<<(std::ostream& os, const Feature& feature) {
    os << feature.reference << '\t' << feature.start << '\t' << feature.end << '\t' << feature.name;
    return os;
}

std::istream& operator>>(std::istream& is, Feature& feature) {
    std::string feature_string;
    std::stringstream feature_stream;
    getline(is, feature_string);
    feature_stream.str(feature_string);
    feature_stream >> feature.reference >> feature.start >> feature.end >> feature.name;
    return is;
}

bool Feature::overlaps(const Feature& other) {
    return reference == other.reference && ((start <= other.start && other.start <= end) || (start <= other.end && other.end <= end));
}
