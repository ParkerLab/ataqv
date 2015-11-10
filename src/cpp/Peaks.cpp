//
// Copyright 2015 The Parker Lab at the University of Michigan
//
// Licensed under Version 3 of the GPL or any later version
//

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "Exceptions.hpp"
#include "Metrics.hpp"
#include "Peaks.hpp"
#include "Types.hpp"
#include "Utils.hpp"


bool operator< (const Peak& a, const Peak& b) {
    return a.start < b.start;
}

ull Peak::size() {
    return end - start;
}

std::ostream& operator<<(std::ostream& os, const Peak& peak) {
    os << peak.chromosome << '\t' << peak.start << '\t' << peak.end << '\t' << peak.name;
    return os;
}

std::istream& operator>>(std::istream& is, Peak& peak) {
    std::string peak_string;
    std::stringstream peak_stream;
    getline(is, peak_string);
    peak_stream.str(peak_string);
    peak_stream >> peak.chromosome >> peak.start >> peak.end >> peak.name;

    return is;
}

bool peak_overlapping_reads_descending_comparator(const Peak& p1, const Peak& p2) {
    return p1.overlapping_reads > p2.overlapping_reads;
}


PeakMetrics::PeakMetrics(const Metrics& metrics) {
    this->metrics = metrics;
}

PeakMetrics::PeakMetrics(const Metrics& metrics, const std::string& peak_filename, const std::string& peak_overlap_filename, const bool verbose) {
    this->metrics = metrics;
    load_peaks(peak_filename, peak_overlap_filename, verbose);
}

///
/// Read the output files of MACS2 and bedtools intersect, to get peak
/// dimensions and intersections of read pairs with peaks
///
void PeakMetrics::load_peaks(const std::string& peak_filename, const std::string& peak_overlap_filename, const bool verbose) {
    std::map<std::string, Peak> peak_map;
    std::ifstream peak_file;
    std::ifstream peak_overlap_file;
    Peak peak;
    ull count = 0;
    ull total_peak_territory = 0;

    peak_file.open(peak_filename.c_str());
    if (peak_file.fail() || !peak_file.is_open()) {
        throw FileException("Could not open the specified file of called peaks (\"" + peak_filename + "\"): " + strerror(errno));
    }

    if (verbose) {
        std::cout << "Reading peaks from " << peak_filename << "...";
    }

    while (peak_file >> peak) {
        peak_map[peak.name] = peak;
        total_peak_territory += peak.size();
    }

    if (verbose) {
        std::cout << "Done." << std::endl << "Found " << peak_map.size() << " peaks." << std::endl;
    }

    peak_overlap_file.open(peak_overlap_filename.c_str());
    if (peak_overlap_file.fail() || !peak_overlap_file.is_open()) {
        throw FileException("Could not open the specified 'bedtools intersect' output file \"" + peak_overlap_filename + "\": " + strerror(errno));
    }

    if (verbose) {
        std::cout << "Reading peak/read pair intersections from " << peak_overlap_filename << "...";
    }

    while (peak_overlap_file >> peak) {
        count++;
        peak_map[peak.name].overlapping_reads += 2;  // count both reads in the pair
    }

    for (const auto peak_it : peak_map) {
        peak = peak_it.second;
        peak.fraction_of_peak_territory = fraction(peak.size(), total_peak_territory);
        peaks.push_back(peak);
    }

    // sort the peaks by the number of overlapping reads, most to least
    std::sort(peaks.begin(), peaks.end(), peak_overlapping_reads_descending_comparator);

    ull peak_count = 0;
    for (auto peak: peaks) {
        peak_count++;
        reads_in_peaks += peak.overlapping_reads;
        if (peak_count == 1) {
            top_peak_count = reads_in_peaks;
        }

        if (peak_count <= 10) {
            top_10_peak_count = reads_in_peaks;
        }

        if (peak_count <= 100) {
            top_100_peak_count = reads_in_peaks;
        }

        if (peak_count <= 1000) {
            top_1000_peak_count = reads_in_peaks;
        }

        if (peak_count <= 10000) {
            top_10000_peak_count = reads_in_peaks;
        }
    }

    if (verbose) {
        std::cout << "Done." << std::endl << "Found " << count << " peak/read pair intersections." << std::endl;
    }
}

void PeakMetrics::write_peak_metrics(std::ostream& os) {
    long double cumulative_fraction_of_perfect_reads = 0.0;
    long double cumulative_fraction_of_total_peak_territory = 0.0;

    os << "Sample"
       << "\t" << "Group"
       << "\t" << "PeakRank"
       << "\t" << "PeakName"
       << "\t" << "OverlappingReads"
       << "\t" << "FractionOfTotalPeakTerritory"
       << "\t" << "CumulativeFractionOfPerfectAutosomalReads"
       << "\t" << "CumulativeFractionOfTotalPeakTerritory"
       << std::endl;

    ull count = 1;
    for (auto peak: peaks) {
        cumulative_fraction_of_perfect_reads += fraction(peak.overlapping_reads, metrics.perfect_reads);
        cumulative_fraction_of_total_peak_territory += peak.fraction_of_peak_territory;
        os << metrics.sample << "\t"
           << metrics.group << "\t"
           << count << "\t"
           << peak.name << "\t"
           << peak.overlapping_reads << "\t"
           << std::fixed << peak.fraction_of_peak_territory << "\t"
           << std::fixed << cumulative_fraction_of_perfect_reads << "\t"
           << std::fixed << cumulative_fraction_of_total_peak_territory
           << std::endl;
        count++;
    }
}

std::ostream& operator<<(std::ostream& os, const PeakMetrics& m) {
    os << "PEAK METRICS" << std::endl
       << "------------" << std::endl
       << "Peak count: " << m.peaks.size() << std::endl
       << "Perfect reads that overlapped peaks: "  << percentage_string(m.reads_in_peaks, m.metrics.perfect_reads, " (", "% of all perfect reads)") << std::endl
       << "Number of perfect reads overlapping the top 10,000 peaks: " << std::endl
       << std::setw(22) << std::right << "Top peak: " << std::fixed << percentage_string(m.top_peak_count, m.metrics.perfect_reads, " (", "% of all perfect reads)") << std::endl
       << std::setw(22) << std::right << "Top 10 peaks: " << std::fixed << percentage_string(m.top_10_peak_count, m.metrics.perfect_reads, " (", "% of all perfect reads)") << std::endl
       << std::setw(22) << std::right << "Top 100 peaks: "<< std::fixed << percentage_string(m.top_100_peak_count, m.metrics.perfect_reads, " (", "% of all perfect reads)") << std::endl
       << std::setw(22) << std::right << "Top 1000 peaks: " << std::fixed << percentage_string(m.top_1000_peak_count, m.metrics.perfect_reads, " (", "% of all perfect reads)") << std::endl
       << std::setw(22) << std::right << "Top 10,000 peaks: " << std::fixed << percentage_string(m.top_10000_peak_count, m.metrics.perfect_reads, " (", "% of all perfect reads)") << std::endl;
    return os;
}

void PeakMetrics::write_table_column_headers(std::ostream& os) {
    os << "Peak Count" << "\t"
       << "Perfect Reads That Overlapped Peaks" << "\t"
       << "Perfect Reads Overlapping Top Peak" << "\t"
       << "Perfect Reads Overlapping Top 10 Peaks" << "\t"
       << "Perfect Reads Overlapping Top 100 Peaks" << "\t"
       << "Perfect Reads Overlapping Top 1000 Peaks" << "\t"
       << "Perfect Reads Overlapping Top 10000 Peaks";
}

void PeakMetrics::write_table_columns(std::ostream& os) {
    os << peaks.size() << "\t"
       << reads_in_peaks << "\t"
       << top_peak_count << "\t"
       << top_10_peak_count << "\t"
       << top_100_peak_count << "\t"
       << top_1000_peak_count << "\t"
       << top_10000_peak_count;
}
