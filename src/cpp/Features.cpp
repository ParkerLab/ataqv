//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#include <iostream>
#include <sstream>

#include "Features.hpp"


Feature::Feature() {}

Feature::Feature(const std::string& reference, unsigned long long int start, unsigned long long int end, const std::string& name): reference(reference), start(start), end(end), name(name) {}


Feature::Feature(const bam_hdr_t *header, const bam1_t *record) :
    reference(std::string(header->target_name[record->core.tid])),
    start(record->core.pos),
    end(bam_endpos(record)),
    name(get_qname(record)) {}


bool operator< (const Feature& f1, const Feature& f2) {
    return (
        sort_strings_numerically(f1.reference, f2.reference) ||
        (f1.reference == f2.reference &&
         (f1.start < f2.start ||
            (f1.start == f2.start &&
             (f1.end < f2.end ||
              (f1.end == f2.end && sort_strings_numerically(f1.name, f2.name))
             )
            )
         )
        )
    );
}


bool feature_overlap_comparator(const Feature& f1, const Feature& f2) {
    return sort_strings_numerically(f1.reference, f2.reference) || f1.start < f2.start || f1.end < f2.end;
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
    std::getline(is, feature_string);
    feature_stream.str(feature_string);
    feature_stream >> feature.reference >> feature.start >> feature.end >> feature.name;
    return is;
}


bool Feature::overlaps(const Feature& other) const {
    return
        reference == other.reference && (
            (
                (start <= other.start && other.start <= end) ||
                (start <= other.end && other.end <= end)
            ) ||
            (
                (other.start <= start && start <= other.end) ||
                (other.start <= end && end <= other.end)
            )
        );
}
