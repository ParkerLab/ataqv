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
#include "Utils.hpp"

class Peak : public Feature {
public:
    unsigned long long int overlapping_hqaa = 0;
    using Feature::Feature;
};

bool operator< (const Peak& p1, const Peak& p2);
bool operator== (const Peak& p1, const Peak& p2);

std::ostream& operator<<(std::ostream& os, const Peak& peak);
std::istream& operator>>(std::istream& is, Peak& peak);
bool peak_overlapping_hqaa_descending_comparator(const Peak& p1, const Peak& p2);

class ReferencePeakCollection {
public:
    std::string reference = "";
    std::vector<Peak> peaks = {};

    unsigned long long int start = 0;
    unsigned long long int end = 0;

    void add(const Peak& peak);
    bool overlaps(const Feature& feature) const;
    void sort();
};


class PeakTree {
private:
    std::map<std::string, ReferencePeakCollection, numeric_string_comparator> tree = {};

public:
    unsigned long long int total_peak_territory = 0;

    unsigned long long int duplicates_in_peaks = 0;
    unsigned long long int duplicates_not_in_peaks = 0;

    unsigned long long int ppm_in_peaks = 0;
    unsigned long long int ppm_not_in_peaks = 0;

    unsigned long long int hqaa_in_peaks = 0;
    unsigned long long int top_peak_hqaa_read_count = 0;
    unsigned long long int top_10_peak_hqaa_read_count = 0;
    unsigned long long int top_100_peak_hqaa_read_count = 0;
    unsigned long long int top_1000_peak_hqaa_read_count = 0;
    unsigned long long int top_10000_peak_hqaa_read_count = 0;

    void add(Peak& peak);
    void determine_top_peaks();
    bool empty();
    ReferencePeakCollection* get_reference_peaks(const std::string& reference_name);
    void record_alignment(const Feature& aligment, bool is_hqaa, bool is_duplicate);
    std::vector<Peak> list_peaks();
    std::vector<Peak> list_peaks_by_overlapping_hqaa_descending();
    std::vector<Peak> list_peaks_by_size_descending();
    void print_reference_peak_counts(std::ostream* os = nullptr);
    size_t size() const;
};

#endif  // PEAKS_HPP
