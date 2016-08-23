//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#ifndef METRICS_HPP
#define METRICS_HPP

#include <map>
#include <string>
#include <vector>

#include "json.hpp"

#include "Exceptions.hpp"
#include "Features.hpp"
#include "HTS.hpp"
#include "IO.hpp"
#include "Peaks.hpp"

using json = nlohmann::json;

class Metrics {
private:
    void log(boost::shared_ptr<boost::iostreams::filtering_ostream> problem_log, const std::string& message);
    void log_problematic_read(boost::shared_ptr<boost::iostreams::filtering_ostream> problem_log, const bam_hdr_t* header, const bam1_t *record, const std::string& problem);

public:
    std::string name = "";
    std::string description = "";
    std::string url = "";

    std::string organism  = "";
    std::string library  = "";
    unsigned long long int cell_count = 0;

    std::string reference_genome = "GRCh37";
    std::string mitochondrial_reference_name = "chrM";

    std::string alignment_filename = "";
    std::string peak_filename = "";
    std::string autosomal_reference_filename = "";
    std::string problematic_reads_filename = "";
    std::vector<std::string> excluded_region_filenames = {};
    bool verbose = false;

    /// For each species, list the autosomal chromosomes that we'll consider
    /// when recording fragment lengths.
    std::map<std::string, std::map<std::string, int>> genome_autosomal_references = {};

    std::vector<Feature> excluded_regions = {};

    unsigned long long int total_reads = 0;
    unsigned long long int forward_reads = 0;
    unsigned long long int reverse_reads = 0;
    unsigned long long int secondary_reads = 0;
    unsigned long long int supplementary_reads = 0;
    unsigned long long int duplicate_reads = 0;

    unsigned long long int paired_reads = 0;
    unsigned long long int paired_and_mapped_reads = 0;
    unsigned long long int properly_paired_and_mapped_reads = 0;
    unsigned long long int first_reads = 0;
    unsigned long long int second_reads = 0;
    unsigned long long int forward_mate_reads = 0;
    unsigned long long int reverse_mate_reads = 0;

    unsigned long long int unmapped_reads = 0;
    unsigned long long int unmapped_mate_reads = 0;
    unsigned long long int qcfailed_reads = 0;
    unsigned long long int unpaired_reads = 0;
    unsigned long long int ff_reads = 0;
    unsigned long long int fr_reads = 0;
    unsigned long long int rf_reads = 0;
    unsigned long long int rr_reads = 0;
    unsigned long long int reads_improperly_oriented = 0;
    unsigned long long int reads_with_mate_mapped_to_different_reference = 0;
    unsigned long long int reads_mapped_with_zero_quality = 0;
    unsigned long long int reads_with_mate_mapped_on_same_strand = 0;
    unsigned long long int reads_mapped_and_paired_but_improperly = 0;

    unsigned long long int unclassified_reads = 0;

    unsigned long long int maximum_proper_pair_fragment_size = 0;
    unsigned long long int reads_with_mate_too_distant = 0;

    std::map<std::string, std::vector<unsigned long long int>> unlikely_fragment_sizes = {};

    unsigned long long int total_autosomal_reads = 0;
    unsigned long long int total_mitochondrial_reads = 0;
    unsigned long long int duplicate_autosomal_reads = 0;
    unsigned long long int duplicate_mitochondrial_reads = 0;

    unsigned long long int hqaa = 0;  // primary, properly paired and mapped to autosomal references

    std::map<int, unsigned long long int> fragment_length_counts = {};
    std::map<int, unsigned long long int> hqaa_fragment_length_counts = {};

    unsigned long long int hqaa_tf_count = 0;
    unsigned long long int hqaa_mononucleosomal_count = 0;

    std::map<int, unsigned long long int> mapq_counts = {};

    std::vector<Peak> peaks = {};
    unsigned long long int total_peak_territory = 0;

    unsigned long long int reads_in_peaks = 0;
    unsigned long long int top_peak_read_count = 0;
    unsigned long long int top_10_peak_read_count = 0;
    unsigned long long int top_100_peak_read_count = 0;
    unsigned long long int top_1000_peak_read_count = 0;
    unsigned long long int top_10000_peak_read_count = 0;

    unsigned long long int hqaa_in_peaks = 0;
    unsigned long long int top_peak_hqaa_read_count = 0;
    unsigned long long int top_10_peak_hqaa_read_count = 0;
    unsigned long long int top_100_peak_hqaa_read_count = 0;
    unsigned long long int top_1000_peak_hqaa_read_count = 0;
    unsigned long long int top_10000_peak_hqaa_read_count = 0;

    Metrics();

    void add_alignment(const bam_hdr_t* header, const bam1_t* record, boost::shared_ptr<boost::iostreams::filtering_ostream> problem_log);
    std::vector<std::string> autosomal_references() const;
    std::string configuration_string() const;
    void increment_overlapping_read_count(Peak* peak);
    bool is_autosomal(std::string &reference_name);
    bool is_ff(const bam1_t* record);
    bool is_fr(const bam1_t* record);
    bool is_rf(const bam1_t* record);
    bool is_rr(const bam1_t* record);
    bool is_hqaa(const bam_hdr_t* header, const bam1_t* record);
    void load_alignments();
    void load_autosomal_reference();
    void load_excluded_regions();
    void load_peaks();
    bool mapq_at_least(const int& mapq, const bam1_t* record);
    double mean_mapq() const;
    double median_mapq() const;

    json to_json();
    void write_json(std::ostream& os);
};

std::ostream& operator<<(std::ostream& os, const Metrics& metrics);

#endif
