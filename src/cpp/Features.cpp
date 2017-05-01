//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#include <algorithm>
#include <iostream>
#include <sstream>

#include "Features.hpp"


Feature::Feature() {}

Feature::Feature(const std::string& reference, unsigned long long int start, unsigned long long int end, const std::string& name, const double score, const std::string& strand) :
    reference(reference),
    start(start),
    end(end),
    name(name),
    score(score),
    strand(strand) {}


Feature::Feature(const bam_hdr_t *header, const bam1_t *record) :
    reference(std::string(header->target_name[record->core.tid])),
    start(record->core.pos),
    end(bam_endpos(record)),
    name(get_qname(record)),
    strand(IS_UNMAPPED(record) ? "." : (IS_REVERSE(record) ? "-": "+")) {}


bool operator== (const Feature& f1, const Feature& f2) {
    return (
        f1.reference == f2.reference &&
        f1.start == f2.start &&
        f1.end == f2.end &&
        f1.name == f2.name
    );
}


bool operator< (const Feature& f1, const Feature& f2) {
    return (sort_strings_numerically(f1.reference, f2.reference) ||
            (f1.reference == f2.reference &&
             (f1.start < f2.start ||
              (f1.start == f2.start &&
               (f1.end < f2.end ||
                (f1.end == f2.end &&
                 sort_strings_numerically(f1.name, f2.name)))))));
}


bool feature_overlap_comparator(const Feature& f1, const Feature& f2) {
    return sort_strings_numerically(f1.reference, f2.reference) || f1.end < f2.start;
}


bool feature_size_descending_comparator(const Feature& f1, const Feature& f2) {
    return f1.size() > f2.size();
}


bool Feature::is_reverse() const {
    return strand == "-";
}


unsigned long long int Feature::size() const {
    return end - start;
}


std::ostream& operator<<(std::ostream& os, const Feature& feature) {
    os << feature.reference << '\t' << feature.start << '\t' << feature.end << '\t' << feature.name << '\t' << feature.score << '\t' << feature.strand;
    return os;
}


std::istream& operator>>(std::istream& is, Feature& feature) {
    std::string feature_string;
    std::stringstream feature_stream;
    std::getline(is, feature_string);
    feature_stream.str(feature_string);
    feature_stream >> feature.reference >> feature.start >> feature.end >> feature.name >> feature.score >> feature.strand;
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


void ReferenceFeatureCollection::add(const Feature& feature) {
    features.push_back(feature);

    if (reference != feature.reference) {
        if (reference.empty()) {
            reference = feature.reference;
        } else {
            throw std::out_of_range("Feature reference does not match collection.");
        }
    }

    if (start == 0 || start > feature.start) {
        start = feature.start;
    }

    if (end == 0 || end < feature.end) {
        end = feature.end;
    }
}


bool ReferenceFeatureCollection::overlaps(const Feature& feature) const {
    return !features.empty() &&
        reference == feature.reference && (
            (
                (start <= feature.start && feature.start <= end) ||
                (start <= feature.end && feature.end <= end)
            ) ||
            (
                (feature.start <= start && start <= feature.end) ||
                (feature.start <= end && end <= feature.end)
            )
        );
}


void FeatureTree::add(Feature& feature) {
    tree[feature.reference].add(feature);
}


bool FeatureTree::empty() {
    return tree.empty();
}


ReferenceFeatureCollection* FeatureTree::get_reference_feature_collection(const std::string& reference_name) {
    return &tree[reference_name];
}


void ReferenceFeatureCollection::sort() {
    std::sort(features.begin(), features.end());
}


std::vector<Feature> FeatureTree::list_features() {
    std::vector<Feature> features;
    for (auto ref_features : tree) {
        for (auto feature: ref_features.second.features) {
            features.push_back(feature);
        }
    }
    std::sort(features.begin(), features.end());
    return features;
}


std::vector<Feature> FeatureTree::list_features_by_size_descending() {
    std::vector<Feature> features;
    for (auto ref_features : tree) {
        for (auto feature: ref_features.second.features) {
            features.push_back(feature);
        }
    }
    std::sort(features.begin(), features.end(), feature_size_descending_comparator);
    return features;
}


void FeatureTree::print_reference_feature_counts(std::ostream* os) {
    std::ostream out(os ? os->rdbuf() : std::cout.rdbuf());
    for (auto reffeatures : tree) {
        out << reffeatures.first << " feature count: " << reffeatures.second.features.size() << std::endl;
    }
}


std::vector<std::string> FeatureTree::get_references() {
    std::vector<std::string> references;
    for (auto it : tree) {
        references.push_back(it.first);
    }
    return references;
}


std::vector<std::string> FeatureTree::get_references_by_feature_count() {
    std::vector<std::pair<unsigned long long int, std::string>> references_by_feature_count = {};
    for (auto it : tree) {
        references_by_feature_count.push_back(std::pair<unsigned long long int, std::string>(it.second.features.size(), it.first));
    }

    std::vector<std::string> references;

    for (auto reference = references_by_feature_count.crbegin(); reference != references_by_feature_count.crend(); reference++ ) {
        references.push_back(reference->second);
    }
    return references;
}


size_t FeatureTree::size() const {
    size_t size = 0;
    for (auto reffeatures : tree) {
        size += reffeatures.second.features.size();
    }
    return size;
}
