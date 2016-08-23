//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#ifndef PEAKS_HPP
#define PEAKS_HPP

#include <iostream>
#include <string>

#include "Features.hpp"

class Peak : public Feature {
public:
    unsigned long long int overlapping_reads = 0;
    unsigned long long int overlapping_hqaa = 0;
};

std::ostream& operator<<(std::ostream& os, const Peak& peak);
std::istream& operator>>(std::istream& is, Peak& peak);
bool peak_overlapping_reads_descending_comparator(const Peak& p1, const Peak& p2);
bool peak_overlapping_hqaa_descending_comparator(const Peak& p1, const Peak& p2);


#endif  // PEAKS_HPP
