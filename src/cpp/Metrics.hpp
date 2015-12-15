//
// Copyright 2015 The Parker Lab at the University of Michigan
//
// Licensed under Version 3 of the GPL or any later version
//

#ifndef METRICS_HPP
#define METRICS_HPP

#include <map>
#include <string>

#include "Exceptions.hpp"
#include "HTS.hpp"
#include "Types.hpp"

class Metrics {
public:
    std::string sample;
    std::string group;
    std::string reference_genome = "GRCh37";

    /// For each species, list the autosomal chromosomes that we'll consider
    /// when recording template sizes.
    std::map<std::string, std::map<std::string, int>> genome_autosomal_references;

    ull total_reads = 0;
    ull first_reads = 0;
    ull second_reads = 0;
    ull duplicate_reads = 0;
    ull properly_paired_and_mapped_reads = 0;
    ull unpaired_reads = 0;
    ull unmapped_reads = 0;
    ull unmapped_mate_reads = 0;
    ull reverse_reads = 0;
    ull reverse_mate_reads = 0;
    ull qcfailed_reads = 0;
    ull secondary_reads = 0;
    ull supplementary_reads = 0;
    ull reads_mapped_and_paired_but_improperly = 0;
    ull unclassified_reads = 0;

    ull total_mitochondrial_reads = 0;
    ull duplicate_mitochondrial_reads = 0;
    ull nonduplicate_mitochondrial_reads = 0;

    ull total_autosomal_reads = 0;
    ull perfect_reads = 0;  // primary, properly paired and mapped to autosomal references
    ull duplicate_autosomal_reads = 0;  // just not marked duplicate

    ull total_nonduplicate_reads = 0;

    ull template_length = 0;
    std::map<int, ull> template_length_counts;

    float tf_fraction = 0.0;
    float mononucleosomal_fraction = 0.0;

    std::map<int, ull> mapq_counts;

    // Constructors
    Metrics();
    Metrics(const std::string &reference_genome);
    Metrics(const std::string &reference_genome, const std::string &reference_filename, const bool& verbose = false);

    void add(const bam_hdr_t* header, const bam1_t* record, const bool& verbose = false);
    bool is_autosomal(std::string &reference_name);
    void load_bam_file(const std::string& filename, const bool& verbose = false);
    void load_autosomal_reference(const std::string &reference_genome, const std::string &reference_filename, const bool& verbose = false);
    void write_template_lengths(std::ostream& os);
    void write_table_column_headers(std::ostream& os);
    void write_table_columns(std::ostream& os);
};

std::ostream& operator<<(std::ostream& os, const Metrics& metrics);

#endif
