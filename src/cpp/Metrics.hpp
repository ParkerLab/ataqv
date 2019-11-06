//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#ifndef METRICS_HPP
#define METRICS_HPP

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "json.hpp"

#include "Exceptions.hpp"
#include "Features.hpp"
#include "HTS.hpp"
#include "IO.hpp"
#include "Peaks.hpp"


class MetricsCollector;
class Metrics;


//
// The MetricsCollector examines a BAM file and optionally, a BED file
// containing peaks, to collect metrics for each read group found. If
// the BAM file has no read groups defined, one will be fabricated for
// it, using the filename.
//
class MetricsCollector {
private:
    void make_default_autosomal_references();
    void load_autosomal_references();
    void load_excluded_regions();

public:
    std::map<std::string, Metrics*, numeric_string_comparator> metrics;

    std::string name = "";
    std::string organism  = "";
    std::string description = "";
    std::string library_description = "";
    std::string url = "";

    std::string alignment_filename = "";

    std::string autosomal_reference_filename = "";
    std::string mitochondrial_reference_name = "chrM";

    // For each organism, the autosomal chromosomes that we'll
    // consider when recording fragment lengths or overlap with peaks.
    std::map<std::string, std::unordered_map<std::string, int>, numeric_string_comparator> autosomal_references;

    std::string peak_filename = "auto";

    std::string tss_filename = "";
    const int tss_extension = 1000;
    FeatureTree tss_tree;

    bool verbose = false;
    int thread_limit = 1;
    bool ignore_read_groups = false;
    bool log_problematic_reads = false;

    std::vector<std::string> excluded_region_filenames = {};
    std::vector<Feature> excluded_regions = {};

    MetricsCollector(const std::string& name = "",
                     const std::string& organism = "human",
                     const std::string& description = "",
                     const std::string& library_description = "",
                     const std::string& url = "",
                     const std::string& alignment_filename = "",
                     const std::string& autosomal_reference_filename = "",
                     const std::string& mitochondrial_reference_name = "chrM",
                     const std::string& peak_filename = "",
                     const std::string& tss_filename = "",
                     const int tss_extension = 1000,
                     bool verbose = false,
                     const int thread_limit = 1,
                     bool ignore_read_groups = false,
                     bool log_problematic_reads = false,
                     const std::vector<std::string>& excluded_region_filenames = {});

    std::string autosomal_reference_string(std::string separator = ", ") const;
    std::string configuration_string() const;
    bool is_autosomal(const std::string &reference_name);
    bool is_mitochondrial(const std::string& reference_name);
    bool is_hqaa(const bam_hdr_t* header, const bam1_t* record);
    void load_tss();
    void load_alignments();
    std::map<std::string,std::map<int, unsigned long long int>> get_tss_coverage_for_reference(const std::string &reference, const int extension);
    void calculate_tss_coverage();
    nlohmann::json to_json();
};


std::ostream& operator<<(std::ostream& os, const MetricsCollector& collector);


// Sequenced library metadata, with SAM spec tag in comments
class Library {
public:
    std::string library = "";  // LB
    std::string sample = "";  // SM
    std::string description = "";  // DS
    std::string center = "";  // CN
    std::string date = "";  // DT
    std::string platform = "";  // PL
    std::string platform_model = "";  // PM
    std::string platform_unit = "";  // PU
    std::string flow_order = "";  // FO
    std::string key_sequence = "";  // KS
    std::string predicted_median_insert_size = "";  // PI
    std::string programs = "";  // PG

    nlohmann::json to_json();
};

std::ostream& operator<<(std::ostream& os, const Library& library);


class Metrics {
private:
    MetricsCollector* collector;
    std::string problematic_read_filename = "";
    boost::shared_ptr<boost::iostreams::filtering_ostream> problematic_read_stream = nullptr;

    void log_problematic_read(const std::string& problem, const std::string& record = "");
    void open_problematic_read_stream();

public:
    std::string name = "";
    Library library = {};

    // read group attributes
    PeakTree peaks;

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
    unsigned long long int fr_reads = 0;

    unsigned long long int unmapped_reads = 0;
    unsigned long long int unmapped_mate_reads = 0;
    unsigned long long int qcfailed_reads = 0;
    unsigned long long int unpaired_reads = 0;
    unsigned long long int ff_reads = 0;
    unsigned long long int rf_reads = 0;
    unsigned long long int rr_reads = 0;
    unsigned long long int reads_with_mate_mapped_to_different_reference = 0;
    unsigned long long int reads_mapped_with_zero_quality = 0;
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

    std::map<std::string, unsigned long long int> chromosome_counts = {};

    unsigned long long int hqaa_short_count = 0;
    unsigned long long int hqaa_mononucleosomal_count = 0;

    std::map<int, unsigned long long int> mapq_counts = {};

    std::map<int, unsigned long long int> tss_coverage = {};
    std::map<int, double> tss_coverage_scaled = {};
    double tss_enrichment = 0.0;

    bool log_problematic_reads = false;
    bool peaks_requested = false;
    bool tss_requested = false;

    Metrics(MetricsCollector* collector, const std::string& name = nullptr);

    void add_alignment(const bam_hdr_t* header, const bam1_t* record);
    std::string configuration_string() const;
    void add_tss_coverage(const Feature& fragment);
    void calculate_tss_metrics();
    std::map<int, unsigned long long int> calculate_tss_metric_for_reference(const std::string &reference, const int extension, FeatureTree &fragment_tree);

    bool is_autosomal(const std::string &reference_name);
    bool is_mitochondrial(const std::string& reference_name);
    bool is_ff(const bam1_t* record);
    bool is_fr(const bam1_t* record);
    bool is_rf(const bam1_t* record);
    bool is_rr(const bam1_t* record);
    bool is_hqaa(const bam_hdr_t* header, const bam1_t* record);
    void load_peaks();
    void make_aggregate_diagnoses();
    std::string make_metrics_filename(const std::string& suffix);
    bool mapq_at_least(const int& mapq, const bam1_t* record);
    double mean_mapq() const;
    double median_mapq() const;
    nlohmann::json to_json();
};

std::ostream& operator<<(std::ostream& os, const Metrics& metrics);

#endif
