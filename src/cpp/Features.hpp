//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#ifndef FEATURES_HPP
#define FEATURES_HPP

#include <string>

#include "HTS.hpp"


class Feature {
public:
    std::string reference = "";
    unsigned long long int start = 0;
    unsigned long long int end = 0;
    std::string name = "";
    double score = 0.0;
    std::string strand;

    Feature();
    Feature(const std::string& reference, unsigned long long int start, unsigned long long int end, const std::string& name, const double score = 0.0, const std::string& strand = ".");
    Feature(const bam_hdr_t *header, const bam1_t *record);

    bool is_reverse() const;
    bool overlaps(const Feature& other) const;
    unsigned long long int size() const;
};

bool operator== (const Feature &f1, const Feature &f2);
bool operator< (const Feature &f1, const Feature &f2);

std::ostream& operator<<(std::ostream& os, const Feature& feature);
std::istream& operator>>(std::istream& is, Feature& feature);

bool feature_overlap_comparator(const Feature& f1, const Feature& f2);

class ReferenceFeatureCollection {
public:
    std::string reference = "";
    std::vector<Feature> features = {};

    unsigned long long int start = 0;
    unsigned long long int end = 0;

    void add(const Feature& feature);
};


class FeatureTree {
private:
    std::map<std::string, ReferenceFeatureCollection, numeric_string_comparator> tree = {};

public:
    void add(Feature& feature);
    ReferenceFeatureCollection* get_reference_feature_collection(const std::string& reference_name);
    std::vector<std::string> get_references_by_feature_count();
    void print_reference_feature_counts(std::ostream* os = nullptr);
    size_t size() const;
};


#endif // FEATURES_HPP
