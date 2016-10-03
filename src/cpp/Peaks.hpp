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
    unsigned long long int overlapping_hqaa = 0;
};

std::ostream& operator<<(std::ostream& os, const Peak& peak);
std::istream& operator>>(std::istream& is, Peak& peak);
bool peak_overlapping_hqaa_descending_comparator(const Peak& p1, const Peak& p2);

struct peak_tree_default_ordering {
    bool operator() (const std::string& p1, const std::string& p2) const {
        return sort_strings_numerically(p1, p2);
    }
};


class ReferencePeakCollection {
public:
    unsigned long long int start = 0;
    unsigned long long int end = 0;
    std::vector<Peak> peaks = {};

    void add(Peak& peak);
    bool overlaps(const Feature& feature);
    void sort_peaks();
};

class PeakTree {
private:
    std::map<std::string, ReferencePeakCollection, peak_tree_default_ordering> tree = {};

public:
    void add(Peak& peak);
    bool empty();
    ReferencePeakCollection* get_reference_peaks(const std::string& reference_name);
    void increment_overlapping_hqaa(const Feature& hqaa);
    std::vector<Peak> list_peaks();
    std::vector<Peak> list_peaks_by_overlapping_hqaa_descending();
    void print_reference_peak_counts();
    size_t size() const;
    void sort_peaks();
};

#endif  // PEAKS_HPP
