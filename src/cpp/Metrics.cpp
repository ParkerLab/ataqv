//
// Copyright 2015 The Parker Lab at the University of Michigan
//
// Licensed under Version 3 of the GPL or any later version
//

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>

#include "Exceptions.hpp"
#include "Metrics.hpp"
#include "HTS.hpp"
#include "Types.hpp"
#include "Utils.hpp"


//
// Constructors
//
Metrics::Metrics() : Metrics("GRCh37") {};

Metrics::Metrics(const std::string& reference_genome) : Metrics(reference_genome, "", false) {};

Metrics::Metrics(const std::string& reference_genome, const std::string& reference_filename, const bool& verbose) {
    genome_autosomal_references = {
        {"GRCh37", {{"chr1", 1}, {"chr2", 1}, {"chr3", 1}, {"chr4", 1}, {"chr5", 1},
                    {"chr6", 1}, {"chr7", 1}, {"chr8", 1}, {"chr9", 1}, {"chr10", 1},
                    {"chr11", 1}, {"chr12", 1}, {"chr13", 1}, {"chr14", 1}, {"chr15", 1},
                    {"chr16", 1}, {"chr17", 1}, {"chr18", 1}, {"chr19", 1}, {"chr20", 1},
                    {"chr21", 1}, {"chr22", 1}}},
        {"GRCh38", {{"chr1", 1}, {"chr2", 1}, {"chr3", 1}, {"chr4", 1}, {"chr5", 1},
                    {"chr6", 1}, {"chr7", 1}, {"chr8", 1}, {"chr9", 1}, {"chr10", 1},
                    {"chr11", 1}, {"chr12", 1}, {"chr13", 1}, {"chr14", 1}, {"chr15", 1},
                    {"chr16", 1}, {"chr17", 1}, {"chr18", 1}, {"chr19", 1}, {"chr20", 1},
                    {"chr21", 1}, {"chr22", 1}}},
        {"MGSCv37", {{"chr1", 1}, {"chr2", 1}, {"chr3", 1}, {"chr4", 1}, {"chr5", 1},
                     {"chr6", 1}, {"chr7", 1}, {"chr8", 1}, {"chr9", 1}, {"chr10", 1},
                     {"chr11", 1}, {"chr12", 1}, {"chr13", 1}, {"chr14", 1}, {"chr15", 1},
                     {"chr16", 1}, {"chr17", 1}, {"chr18", 1}, {"chr19", 1}}},
        {"Rnor_5.0", {{"chr1", 1}, {"chr2", 1}, {"chr3", 1}, {"chr4", 1}, {"chr5", 1},
                      {"chr6", 1}, {"chr7", 1}, {"chr8", 1}, {"chr9", 1}, {"chr10", 1},
                      {"chr11", 1}, {"chr12", 1}, {"chr13", 1}, {"chr14", 1}, {"chr15", 1},
                      {"chr16", 1}, {"chr17", 1}, {"chr18", 1}, {"chr19", 1}, {"chr20", 1}}},
        {"S288c", {{"chrI", 1}, {"chrII", 1}, {"chrIII", 1}, {"chrIV", 1}, {"chrV", 1},
                   {"chrVI", 1}, {"chrVII", 1}, {"chrVIII", 1}, {"chrIX", 1}, {"chrX", 1},
                   {"chrXI", 1}, {"chrXII", 1}, {"chrXIII", 1}, {"chrXIV", 1}, {"chrXV", 1},
                   {"chrXVI", 1}}}
    };

    this->reference_genome = reference_genome;

    if (!reference_filename.empty()) {
        load_autosomal_reference(reference_genome, reference_filename, verbose);
    }
}

///
/// We only want to tally template sizes for reads from autosomal
/// chromosomes in the reference assembly, ignoring allosomes and
/// the sketchy stuff: contigs for which the exact position or
/// chromosome was unknown.
///
/// The genome_autosomal_references map contains for each species all
/// the chromosomes that will be considered when recording template
/// sizes, so if we find the read's reference name there, we'll
/// count it.
///
bool Metrics::is_autosomal(std::string& reference_name) {
    return bool(genome_autosomal_references[reference_genome] [reference_name]);
}


///
/// Read autosomal references from a file, one per line, creating or
/// replacing an entry for them under the given reference genome in
/// genome_autosomal_references.
///
void Metrics::load_autosomal_reference(const std::string& reference_genome, const std::string& reference_filename, const bool& verbose) {
    std::ifstream reference_file;
    std::string reference_name;

    reference_file.open(reference_filename.c_str());
    if (reference_file.fail() || !reference_file.is_open()) {
        throw FileException("Could not open the supplied peak intersection file \"" + reference_filename + "\": " + strerror(errno));
    }

    if (verbose) {
        std::cout << "Reading " << reference_genome << " autosomal references from " << reference_filename << "..." << std::endl;
    }

    // clear any existing references for this genome
    genome_autosomal_references[reference_genome] = {};

    // load the file
    while (reference_file >> reference_name) {
        genome_autosomal_references[reference_genome] [reference_name] = 1;
    }

    if (verbose) {
        std::vector<std::string> autosomal_references;
        std::cout << "Autosomal references for genome " << reference_genome << ": " << std::endl;
        for (auto it : genome_autosomal_references[reference_genome]) {
            autosomal_references.push_back(it.first);
        }
        std::sort(autosomal_references.begin(), autosomal_references.end(), sort_strings_numerically);
        for (const auto it : autosomal_references) {
            std::cout << "\t" << it << std::endl;
        }
    }
}

///
/// Measure and record a single read
///
void Metrics::add(const bam_hdr_t* header, const bam1_t* record, const bool& verbose) {
    std::string reference_name;
    std::string chrM("chrM");
    ull template_length = 0;

    total_reads++;

    //
    // Record the read's quality
    //
    mapq_counts[record->core.qual]++;

    if (IS_DUP(record)) {
        duplicate_reads++;
    }

    if (IS_READ1(record)) {
        first_reads++;
    }

    if (IS_READ2(record)) {
        second_reads++;
    }

    if (!IS_PAIRED(record)) {
        unpaired_reads++;
    }

    if (IS_QCFAIL(record)) {
        qcfailed_reads++;
    }

    if (IS_REVERSE(record)) {
        reverse_reads++;
    }

    if (IS_MATE_REVERSE(record)) {
        reverse_mate_reads++;
    }

    if (IS_SECONDARY(record)) {
        secondary_reads++;
    }

    if (IS_SUPPLEMENTARY(record)) {
        supplementary_reads++;
    }

    if (IS_UNMAPPED(record)) {
        unmapped_reads++;
    } else if (IS_MATE_UNMAPPED(record)) {
        unmapped_mate_reads++;
    } else if (IS_PAIRED_AND_MAPPED(record)) {
        if (IS_PROPERLYPAIRED(record)) {
            properly_paired_and_mapped_reads++;
        } else {
            // OK, the read was paired, and mapped, but not in a
            // proper pair. Some possibilities: the reads mapped too
            // far from each other (maybe on different chromosomes),
            // or were both on the same strand.
            reads_mapped_and_paired_but_improperly++;
        }
    } else {
        // Most cases should have been caught by now, so let's
        // make a special note of any unexpected oddballs.
        unclassified_reads++;
        std::cerr << "Unclassified read: " << record_to_string(header, record) << std::endl;
    }

    // if the read mapped, then there should be a valid target ID
    // and we should be able to retrieve the target reference
    // sequence name, so we can distinguish autosomal reads
    if (!IS_UNMAPPED(record) && !IS_MATE_UNMAPPED(record) && (record->core.tid >= 0)) {
        reference_name = header->target_name[record->core.tid];
        if (is_autosomal(reference_name)) {
            total_autosomal_reads++;
            if (IS_DUP(record)) {
                duplicate_autosomal_reads++;
            } else {
                // nonduplicate, properly paired and uniquely mapped
                // autosomal reads will be the basis of our template
                // size and peak statistics
                if (IS_PAIRED_AND_MAPPED(record) && IS_PROPERLYPAIRED(record) && IS_ORIGINAL(record)) {
                    perfect_reads++;

                    // record the template sizes of perfect reads, so we
                    // can plot nucleosomal periodicity
                    template_length = abs(record->core.isize) - 8;  // on each end of the fragment, remove four bases to transposase integration point
                    if (template_length <= 1000) {
                        template_length_counts[template_length]++;
                    }
                }
            }
        } else {
            if (chrM.compare(reference_name) == 0) {
                total_mitochondrial_reads++;
                if (IS_DUP(record)) {
                    duplicate_mitochondrial_reads++;
                }
            }
        }
    }
}


///
/// Measure all the reads in a BAM file
///
void Metrics::load_bam_file(const std::string& filename, const bool& verbose) {
    int r;
    samFile *in = 0;
    bam_hdr_t *header = NULL;
    bam1_t *record = bam_init1();

    if (verbose) {
        std::cout << "Collecting statistics from " << filename << "..." << std::endl;
    }

    if ((in = sam_open(filename.c_str(), "r")) == 0) {
        throw FileException("Could not open \"" + filename + "\".");
    }

    if ((header = sam_hdr_read(in)) == 0) {
        throw FileException("Could not read a valid header from \"" + filename +  "\".");
    }

    while ((r = sam_read1(in, header, record)) >= 0) {
        add(header, record);
    }

    if (verbose) {
        std::cout << "Read count: " << total_reads << std::endl;
        std::cout << "Done." << std::endl << std::endl;
    }

    for (int i = 30; i <= 80; i++) {
        tf_fraction += fraction(template_length_counts[i], perfect_reads);
    }

    for (int i = 150; i <= 200; i++) {
        mononucleosomal_fraction += fraction(template_length_counts[i], perfect_reads);
    }
}



///
/// Present the Metrics in plain text
///
std::ostream& operator<<(std::ostream& os, const Metrics& m) {
    os << "ataqc " << version_string() << std::endl << std::endl
       << "Sample: " << m.sample  << std::endl
       << "Group: " << m.group  << std::endl
       << "Reference genome: " << m.reference_genome << std::endl << std::endl

       << "READ METRICS" << std::endl
       << "------------" << std::endl
       << "       Total reads: " << m.total_reads << std::endl
       << "       First reads: " << percentage_string(m.first_reads, m.total_reads) << std::endl
       << "      Second reads: " << percentage_string(m.second_reads, m.total_reads) << std::endl
       << "     Reverse reads: " << percentage_string(m.reverse_reads, m.total_reads) << std::endl
       << "Reverse mate reads: " << percentage_string(m.reverse_mate_reads, m.total_reads) << std::endl << std::endl
       << "Properly paired and mapped reads: " << percentage_string(m.properly_paired_and_mapped_reads, m.total_reads) << std::endl
       << "Reads that paired and mapped but not properly: " << percentage_string(m.reads_mapped_and_paired_but_improperly, m.total_reads) << std::endl
       << "Unpaired reads: " << percentage_string(m.unpaired_reads, m.total_reads) << std::endl
       << "Unmapped reads: " << percentage_string(m.unmapped_reads, m.total_reads) << std::endl
       << "Unmapped mate reads: " << percentage_string(m.unmapped_mate_reads, m.total_reads) << std::endl
       << "Reads not passing quality controls: " << percentage_string(m.qcfailed_reads, m.total_reads) << std::endl
       << "Secondary reads: " << percentage_string(m.secondary_reads, m.total_reads) << std::endl
       << "Supplementary reads: " << percentage_string(m.supplementary_reads, m.total_reads) << std::endl
       << "Unclassified reads: " << percentage_string(m.unclassified_reads, m.total_reads) << std::endl
       << "Duplicate reads: " << percentage_string(m.duplicate_reads, m.total_reads, 3, " (", "% of all reads)") << std::endl << std::endl

       << "AUTOSOMAL/MITOCHONDRIAL METRICS" << std::endl
       << "-------------------------------" << std::endl
       << "Total autosomal reads: " << percentage_string(m.total_autosomal_reads, m.total_reads, 3,  " (", "% of all reads)") << std::endl
       << "Total mitochondrial reads: " << percentage_string(m.total_mitochondrial_reads, m.total_reads, 3, " (", "% of all reads)") << std::endl
       << "Duplicate autosomal reads: " << percentage_string(m.duplicate_autosomal_reads, m.total_autosomal_reads, 3, " (", "% of all autosomal reads)") << std::endl
       << "Duplicate mitochondrial reads: " << percentage_string(m.duplicate_mitochondrial_reads, m.total_mitochondrial_reads, 3, " (", "% of all mitochondrial reads)") << std::endl
       << "Perfect -- nonduplicate, properly paired and uniquely mapped autosomal -- reads: " << percentage_string(m.perfect_reads, m.total_autosomal_reads, 3, " (", "% of all autosomal reads)") << std::endl << std::endl;

    os << "SHORT/MONONUCLEOSOMAL FRAGMENT RATIO" << std::endl
       << "------------------------------------" << std::endl
       << "Fraction of perfect reads in templates 30-80bp long: " << std::fixed << m.tf_fraction << std::endl
       << "Fraction of perfect reads in templates 150-200bp long: " << std::fixed << m.mononucleosomal_fraction << std::endl
       << "Ratio of those fractions (30-80/150-200): " << std::fixed << fraction(m.tf_fraction, m.mononucleosomal_fraction) << std::endl << std::endl;

    double average_mapq = 0;
    std::set<int> mapq_values;

    for (auto it : m.mapq_counts) {
        mapq_values.insert(it.first);
        average_mapq += it.first * it.second;
    }
    average_mapq /= m.total_reads;

    os << "MAPPING QUALITY" << std::endl
       << "---------------" << std::endl
       << "Average MAPQ: " << std::fixed << average_mapq << std::endl
       << "MAPQ values seen: ";

    int last_value = -1;
    int last_printed = -1;
    for (auto it : mapq_values) {
        if (last_value < 0) {
            os << it;
            last_printed = it;
        } else if (it - last_value > 1) {
            if (last_printed != last_value) {
                os << "-" << last_value;
            }
            os << ", " << it;
            last_printed = it;
        }
        last_value = it;
    }
    os << std::endl;

    os << "Reads with MAPQ >=..." << std::endl;
    for (int threshold = 5; threshold <= 30; threshold += 5) {
        ull count = 0;
        for (auto it : m.mapq_counts) {
            if (it.first >= threshold) {
                count += it.second;
            }
        }
        os << std::setfill(' ') << std::setw(20) << std::right << threshold << ": " << percentage_string(count, m.total_reads) << std::endl;
    }

    os << "MAPQ distribution:" << std::endl;
    for (auto it : mapq_values) {
        ull count;
        try {
            count = m.mapq_counts.at(it);
        } catch (std::out_of_range) {
            count = 0;
        }
        float count_length = 20 * fraction(count, m.total_reads);
        std::string bar(count_length, '*');

        os << std::setfill(' ')
           << std::right << std::setw(20) << it << ": "
           << std::left << std::setw(24) << percentage_string(count, m.total_reads, 3)
           << std::left << "|" << std::setw(20) << bar << "|"
           << std::endl;
    }

    return os;
}

void Metrics::write_template_metrics(std::ostream& os) {
    // write a file containing for each observed template length, the
    // count of reads with that length, and the fraction of perfect
    // autosomal reads they represent
    os << "Sample\tGroup\tTemplateLength\tCount\tFractionOfPerfectAutosomalReads\tFractionOfAllReads" << std::endl;

    for (int i = 0; i <= 1000; i++) {
        os << sample
           << "\t" << group
           << "\t" << i
           << "\t" << template_length_counts[i]
           << "\t" << std::fixed << fraction(template_length_counts[i], perfect_reads)
           << "\t" << std::fixed << fraction(template_length_counts[i], total_reads)
           << std::endl;
    }
}

void Metrics::write_table_column_headers(std::ostream& os) {
    os << "Sample" << "\t"
       << "Group" << "\t"
       << "Total reads" << "\t"
       << "First reads " << "\t"
       << "First reads as percentage of all reads" << "\t"
       << "Second reads " << "\t"
       << "Second reads as percentage of all reads" << "\t"
       << "Reverse reads" << "\t"
       << "Reverse reads as percentage of all reads" << "\t"
       << "Reverse mate reads" << "\t"
       << "Reverse mate reads as percentage of all reads" << "\t"
       << "Properly paired and mapped reads" << "\t"
       << "Properly paired and mapped reads as percentage of all reads" << "\t"
       << "Reads that paired and mapped but not properly" << "\t"
       << "Reads that paired and mapped but not properly as percentage of all reads" << "\t"
       << "Unpaired reads" << "\t"
       << "Unpaired reads as percentage of all reads" << "\t"
       << "Unmapped reads" << "\t"
       << "Unmapped reads as percentage of all reads" << "\t"
       << "Unmapped mate reads" << "\t"
       << "Unmapped mate reads as percentage of all reads" << "\t"
       << "Reads not passing quality controls" << "\t"
       << "Reads not passing quality controls as percentage of all reads" << "\t"
       << "Secondary reads" << "\t"
       << "Secondary reads as percentage of all reads" << "\t"
       << "Supplementary reads" << "\t"
       << "Supplementary reads as percentage of all reads" << "\t"
       << "Unclassified reads" << "\t"
       << "Unclassified reads as percentage of all reads" << "\t"
       << "Total autosomal reads" << "\t"
       << "Total autosomal reads as percentage of all reads" << "\t"
       << "Total mitochondrial reads" << "\t"
       << "Total mitochondrial reads as percentage of all reads" << "\t"
       << "Total duplicate reads" << "\t"
       << "Total duplicate reads as percentage of all reads" << "\t"
       << "Duplicate autosomal reads" << "\t"
       << "Duplicate autosomal reads as percentage of all autosomal reads" << "\t"
       << "Duplicate mitochondrial reads" << "\t"
       << "Duplicate mitochondrial reads as percentage of all mitochondrial reads" << "\t"
       << "Perfect (nonduplicate, properly paired and uniquely mapped, autosomal) reads" << "\t"
       << "Perfect (nonduplicate, properly paired and uniquely mapped, autosomal) reads as percentage of all autosomal reads" << "\t"
       << "Fraction of perfect reads in templates 30-80bp long" << "\t"
       << "Fraction of perfect reads in templates 150-200bp long" << "\t"
       << "Ratio of perfect read template fractions (30-80/150-200)";
}

void Metrics::write_table_columns(std::ostream& os) {
    os << sample << "\t"
       << group << "\t"
       << total_reads << "\t"
       << first_reads << "\t"
       << std::fixed << percentage(first_reads, total_reads) << "\t"
       << second_reads << "\t"
       << std::fixed << percentage(second_reads, total_reads) << "\t"
       << reverse_reads << "\t"
       << std::fixed << percentage(reverse_reads, total_reads) << "\t"
       << reverse_mate_reads << "\t"
       << std::fixed << percentage(reverse_mate_reads, total_reads) << "\t"
       << properly_paired_and_mapped_reads << "\t"
       << std::fixed << percentage(properly_paired_and_mapped_reads, total_reads) << "\t"
       << reads_mapped_and_paired_but_improperly << "\t"
       << std::fixed << percentage(reads_mapped_and_paired_but_improperly, total_reads) << "\t"
       << unpaired_reads << "\t"
       << std::fixed << percentage(unpaired_reads, total_reads) << "\t"
       << unmapped_reads << "\t"
       << std::fixed << percentage(unmapped_reads, total_reads) << "\t"
       << unmapped_mate_reads << "\t"
       << std::fixed << percentage(unmapped_mate_reads, total_reads) << "\t"
       << qcfailed_reads << "\t"
       << std::fixed << percentage(qcfailed_reads, total_reads) << "\t"
       << secondary_reads << "\t"
       << std::fixed << percentage(secondary_reads, total_reads) << "\t"
       << supplementary_reads << "\t"
       << std::fixed << percentage(supplementary_reads, total_reads) << "\t"
       << unclassified_reads << "\t"
       << std::fixed << percentage(unclassified_reads, total_reads) << "\t"
       << total_autosomal_reads << "\t"
       << std::fixed << percentage(total_autosomal_reads, total_reads) << "\t"
       << total_mitochondrial_reads << "\t"
       << std::fixed << percentage(total_mitochondrial_reads, total_reads) << "\t"
       << duplicate_reads << "\t"
       << std::fixed << percentage(duplicate_reads, total_reads) << "\t"
       << duplicate_autosomal_reads << "\t"
       << std::fixed << percentage(duplicate_autosomal_reads, total_autosomal_reads) << "\t"
       << duplicate_mitochondrial_reads << "\t"
       << std::fixed << percentage(duplicate_mitochondrial_reads, total_mitochondrial_reads) << "\t"
       << perfect_reads << "\t"
       << std::fixed << percentage(perfect_reads, total_autosomal_reads) << "\t"
       << std::fixed << tf_fraction << "\t"
       << std::fixed << mononucleosomal_fraction << "\t"
       << std::fixed << fraction(tf_fraction, mononucleosomal_fraction);
}
