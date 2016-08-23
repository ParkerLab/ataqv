//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#include <iostream>
#include <sstream>

#include "Peaks.hpp"


std::ostream& operator<<(std::ostream& os, const Peak& peak) {
    os << peak.reference << '\t' << peak.start << '\t' << peak.end << '\t' << peak.name << "\t";
    return os;
}

std::istream& operator>>(std::istream& is, Peak& peak) {
    std::string peak_string;
    std::stringstream peak_stream;
    getline(is, peak_string);
    peak_stream.str(peak_string);
    peak_stream >> peak.reference >> peak.start >> peak.end >> peak.name;
    peak.overlapping_reads = 0;
    peak.overlapping_hqaa = 0;
    return is;
}

bool peak_overlapping_reads_descending_comparator(const Peak& p1, const Peak& p2) {
    return p1.overlapping_reads > p2.overlapping_reads;
}

bool peak_overlapping_hqaa_descending_comparator(const Peak& p1, const Peak& p2) {
    return p1.overlapping_hqaa > p2.overlapping_hqaa;
}
