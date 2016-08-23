//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#ifndef FEATURES_HPP
#define FEATURES_HPP

#include <string>

class Feature {
public:
    std::string reference = "";
    unsigned long long int start = 0;
    unsigned long long int end = 0;
    std::string name = "";
    friend bool operator< (const Feature &a, const Feature &b);
    unsigned long long int size() const;

    bool overlaps(const Feature& other);
};

std::ostream& operator<<(std::ostream& os, const Feature& feature);
std::istream& operator>>(std::istream& is, Feature& feature);

#endif // FEATURES_HPP
