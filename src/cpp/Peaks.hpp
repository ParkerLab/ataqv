//
// Copyright 2015 The Parker Lab at the University of Michigan
//
// Licensed under Version 3 of the GPL or any later version
//

#ifndef PEAK_HPP
#define PEAK_HPP

#include <iostream>
#include <string>
#include <vector>

#include "Types.hpp"

class Peak {
public:
    std::string chromosome;
    ull start = 0;
    ull end = 0;
    std::string name;
    ull overlapping_reads = 0;
    double fraction_of_peak_territory;
    friend bool operator< (const Peak &a, const Peak &b);
    ull size();
};

std::ostream& operator<<(std::ostream& os, const Peak& peak);
std::istream& operator>>(std::istream& is, Peak& peak);
bool peak_overlapping_reads_descending_comparator(const Peak& p1, const Peak& p2);

typedef std::vector<Peak> peak_vector;

class PeakMetrics {
public:
    peak_vector peaks;
    Metrics metrics;
    ull top_peak_count = 0;
    ull top_10_peak_count = 0;
    ull top_100_peak_count = 0;
    ull top_1000_peak_count = 0;
    ull top_10000_peak_count = 0;
    ull reads_in_peaks = 0;


    // Constructors
    PeakMetrics(const Metrics& metrics);
    PeakMetrics(const Metrics& metrics, const std::string& peak_filename, const std::string& peak_overlap_filename, const bool verbose = false);

    void load_peaks(const std::string& peak_filename, const std::string& peak_overlap_filename, const bool verbose = false);
    void write_peak_metrics(std::ostream& os);
    void write_table_column_headers(std::ostream& os);
    void write_table_columns(std::ostream& os);
};

std::ostream& operator<<(std::ostream& os, const PeakMetrics& peak_metrics);

#endif  // PEAK_HPP
