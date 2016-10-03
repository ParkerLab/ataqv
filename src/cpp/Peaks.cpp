//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#include <algorithm>
#include <iostream>
#include <sstream>

#include <boost/chrono.hpp>

#include "Peaks.hpp"


std::ostream& operator<<(std::ostream& os, const Peak& peak) {
    os << peak.reference << '\t' << peak.start << '\t' << peak.end << '\t' << peak.name;
    return os;
}


std::istream& operator>>(std::istream& is, Peak& peak) {
    std::string peak_string;
    std::stringstream peak_stream;
    std::getline(is, peak_string);
    peak_stream.str(peak_string);
    peak_stream >> peak.reference >> peak.start >> peak.end >> peak.name;
    peak.overlapping_hqaa = 0;
    return is;
}


bool peak_coordinate_comparator(const Peak& p1, const Peak& p2) {
    if (p1.start < p2.start) {
        return true;
    }

    if (p1.end < p2.end) {
        return true;
    }

    return false;
}


bool peak_overlapping_hqaa_descending_comparator(const Peak& p1, const Peak& p2) {
    return p1.overlapping_hqaa > p2.overlapping_hqaa;
}


void ReferencePeakCollection::add(Peak& peak) {
    peaks.push_back(peak);
    if (start > peak.start) {
        start = peak.start;
    }

    if (end < peak.end) {
        end = peak.end;
    }
}


bool ReferencePeakCollection::overlaps(const Feature& feature) {
    return !peaks.empty() && ((start <= feature.end) || (feature.start <= end));
}


void ReferencePeakCollection::sort_peaks() {
    std::sort(peaks.begin(), peaks.end(), peak_coordinate_comparator);
}


void PeakTree::add(Peak& peak) {
    tree[peak.reference].add(peak);
}


bool PeakTree::empty() {
    return tree.empty();
}


ReferencePeakCollection* PeakTree::get_reference_peaks(const std::string& reference_name){
    return &tree[reference_name];
}


void PeakTree::increment_overlapping_hqaa(const Feature& hqaa) {
    ReferencePeakCollection* rpc = get_reference_peaks(hqaa.reference);
    if (rpc->overlaps(hqaa)) {
        auto peak = std::upper_bound(rpc->peaks.begin(), rpc->peaks.end(), hqaa);
        auto end = rpc->peaks.end();

        for (; peak != end; ++peak) {
            if (peak->overlaps(hqaa)) {
                peak->overlapping_hqaa++;
            } else {
                break;
            }
        }
    }
}


std::vector<Peak> PeakTree::list_peaks() {
    std::vector<Peak> peaks;
    for (auto ref_peaks : tree) {
        for (auto peak: ref_peaks.second.peaks) {
            peaks.push_back(peak);
        }
    }
    return peaks;
}


std::vector<Peak> PeakTree::list_peaks_by_overlapping_hqaa_descending() {
    std::vector<Peak> peaks;
    for (auto ref_peaks : tree) {
        for (auto peak: ref_peaks.second.peaks) {
            peaks.push_back(peak);
        }
    }
    std::sort(peaks.begin(), peaks.end(), peak_overlapping_hqaa_descending_comparator);
    return peaks;
}


void PeakTree::print_reference_peak_counts() {
    for (auto refpeaks : tree) {
        std::cout << refpeaks.first << " peak count: " << refpeaks.second.peaks.size() << std::endl;
    }
}


size_t PeakTree::size() const {
    size_t size = 0;
    for (auto refpeaks : tree) {
        size += refpeaks.second.peaks.size();
    }
    return size;
}


void PeakTree::sort_peaks() {
    for (auto it : tree) {
        it.second.sort_peaks();
    }
}
