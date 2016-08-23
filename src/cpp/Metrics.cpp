//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#include <boost/algorithm/string/join.hpp>
#include <boost/chrono.hpp>

#include "IO.hpp"
#include "Metrics.hpp"
#include "Utils.hpp"


//
// Constructors
//

Metrics::Metrics() {
    std::map<std::string, int> default_references = {
        {"GRCh38", 22},
        {"GRCh37", 22},
        {"MGSCv37", 19},
        {"Rnor_5.0", 20}
    };

    for (auto r : default_references) {
        for (int i = 1; i <= r.second; i++) {
            genome_autosomal_references[r.first]["chr" + std::to_string(i)] = 1;
        }
    }
}


bool Metrics::is_autosomal(std::string& reference_name) {
    try {
        genome_autosomal_references.at(reference_genome).at(reference_name);
        return true;
    } catch (std::out_of_range) {
        return false;
    }
}


std::vector<std::string> Metrics::autosomal_references() const {
    std::vector<std::string> autosomal_references;
    for (auto it : genome_autosomal_references.at(reference_genome)) {
        autosomal_references.push_back(it.first);
    }
    std::sort(autosomal_references.begin(), autosomal_references.end(), sort_strings_numerically);
    return autosomal_references;
}


std::string Metrics::configuration_string() const {
    std::stringstream cs;
    cs << "ataqc " << version_string() << std::endl
       << "Experiment description: " << description << std::endl
       << "URL: " << url << std::endl
       << "Organism: " << organism << std::endl
       << "Library description: " << library << std::endl
       << "Reference genome: " << reference_genome << std::endl
       << "Autosomal references: " << boost::algorithm::join(autosomal_references(), ", ") << std::endl
       << "Mitochondrial reference: " << mitochondrial_reference_name << std::endl << std::endl;
    return cs.str();
}


bool Metrics::mapq_at_least(const int& mapq, const bam1_t* record) {
    return (mapq <= record->core.qual);
}


bool Metrics::is_hqaa(const bam_hdr_t* header, const bam1_t* record) {
    bool hqaa = false;

    if (
        !IS_UNMAPPED(record) &&
        !IS_MATE_UNMAPPED(record) &&
        !IS_DUP(record) &&
        IS_PAIRED_AND_MAPPED(record) &&
        IS_PROPERLYPAIRED(record) &&
        IS_ORIGINAL(record) &&
        mapq_at_least(30, record) &&
        (record->core.tid >= 0)) {

        std::string reference_name(header->target_name[record->core.tid]);
        if (is_autosomal(reference_name)) {
            hqaa = true;
        }
    }

    return hqaa;
}


//
// https://sourceforge.net/p/samtools/mailman/message/27693741/
//
// What is "FR orientation"?
//
// "The end mapped to smaller coordinate is on the forward strand and
// the other end on the reverse strand." -- Heng Li
//
bool Metrics::is_fr(const bam1_t* record) {
    return
        !IS_UNMAPPED(record) &&
        !IS_MATE_UNMAPPED(record) &&
        record->core.tid == record->core.mtid &&
        record->core.pos != 0 &&
        record->core.mpos != 0 &&
        (
            (
                !IS_REVERSE(record) &&
                IS_MATE_REVERSE(record) &&
                record->core.isize > 0
            )
            ||
            (
                IS_REVERSE(record) &&
                !IS_MATE_REVERSE(record) &&
                record->core.isize < 0
            )
        );
}


bool Metrics::is_rf(const bam1_t* record) {
    return
        !IS_UNMAPPED(record) &&
        !IS_MATE_UNMAPPED(record) &&
        record->core.tid == record->core.mtid &&
        record->core.pos != 0 &&
        record->core.mpos != 0 &&
        record->core.isize != 0 &&
        (
            (
                IS_REVERSE(record) &&
                !IS_MATE_REVERSE(record) &&
                record->core.isize > 0
            )
            ||
            (
                !IS_REVERSE(record) &&
                IS_MATE_REVERSE(record) &&
                record->core.isize < 0
            )
        );
}

bool Metrics::is_ff(const bam1_t* record) {
    return (!IS_REVERSE(record) && !IS_MATE_REVERSE(record));
}


bool Metrics::is_rr(const bam1_t* record) {
    return (IS_REVERSE(record) && IS_MATE_REVERSE(record));
}


double Metrics::mean_mapq() const {
    unsigned long long int total_mapq = 0;
    for (auto it : mapq_counts) {
        total_mapq += it.first * it.second;
    }
    return (double) total_mapq / total_reads;
}


double Metrics::median_mapq() const {
    double median;
    std::vector<unsigned long long int> mapq;
    unsigned long long int mapq_count;
    for (auto it : mapq_counts) {
        for (unsigned long long int i = 0; i < it.second; i++) {
            mapq.push_back(it.first);
        }
    }
    std::sort(mapq.begin(), mapq.end());
    mapq_count = mapq.size();
    if (mapq_count % 2 == 0) {
        median = (mapq[((unsigned long long int)mapq_count / 2) - 1] + mapq[((unsigned long long int)mapq_count / 2)]) / 2;
    } else {
        median = mapq[mapq_count / 2];
    }
    return median;
}


///
/// Read autosomal references from a file, one per line, creating or
/// replacing an entry for them under the given reference genome in
/// genome_autosomal_references.
///
void Metrics::load_autosomal_reference() {
    if (autosomal_reference_filename.empty()) {
        std::cerr << "No autosomal reference filename has been specified." << std::endl;
    } else {
        std::string reference_name;
        boost::shared_ptr<boost::iostreams::filtering_istream> reference_file;

        try {
            reference_file = mistream(autosomal_reference_filename);
        } catch (FileException& e) {
            throw FileException("Could not open the supplied autosomal reference file \"" + autosomal_reference_filename + "\": " + e.what());
        }

        if (verbose) {
            std::cout << "Reading " << reference_genome << " autosomal references from " << autosomal_reference_filename << "..." << std::endl;
        }

        // clear any existing references for this genome
        genome_autosomal_references[reference_genome] = {};

        // load the file
        while (*reference_file >> reference_name) {
            genome_autosomal_references[reference_genome][reference_name] = 1;
        }

        if (verbose) {
            std::cout << "Autosomal references for genome " << reference_genome << ": " << std::endl;
            for (const auto it : autosomal_references()) {
                std::cout << "\t" << it << std::endl;
            }
        }
    }
}


void Metrics::load_excluded_regions() {
    boost::shared_ptr<boost::iostreams::filtering_istream> region_file;
    Feature region;
    unsigned long long int count = 0;

    if (excluded_region_filenames.empty()) {
        std::cerr << "No excluded region files have been specified." << std::endl;
    }

    for (auto filename : excluded_region_filenames) {
        try {
            region_file = mistream(filename);
        } catch (FileException& e) {
            throw FileException("Could not open the supplied excluded region file \"" + filename + "\": " + e.what());
        }

        while (*region_file >> region) {
            excluded_regions.push_back(region);
            count++;
        }

        if (verbose) {
            std::cout << "Read " << count << " excluded regions from " << filename << "." << std::endl;
        }
    }
}


void Metrics::log(boost::shared_ptr<boost::iostreams::filtering_ostream> log_stream, const std::string& message) {
    if (log_stream && log_stream->is_complete()) {
        *log_stream << message << std::endl << std::flush;
    }
}


void Metrics::log_problematic_read(boost::shared_ptr<boost::iostreams::filtering_ostream> problem_log, const bam_hdr_t* header, const bam1_t *record, const std::string& problem) {
    log(problem_log, problem + "\t" + record_to_string(header, record));
}


///
/// Measure and record a single read
///
void Metrics::add_alignment(const bam_hdr_t* header, const bam1_t* record, boost::shared_ptr<boost::iostreams::filtering_ostream> problem_log) {
    unsigned long long int fragment_length = abs(record->core.isize);

    total_reads++;

    // record the read's quality
    mapq_counts[record->core.qual]++;

    if (IS_REVERSE(record)) {
        reverse_reads++;
    } else {
        forward_reads++;
    }

    if (IS_SECONDARY(record)) {
        secondary_reads++;
    }

    if (IS_SUPPLEMENTARY(record)) {
        supplementary_reads++;
    }

    if (IS_DUP(record)) {
        duplicate_reads++;
    }

    if (IS_READ1(record)) {
        first_reads++;
    }

    if (IS_READ2(record)) {
        second_reads++;
    }

    if (IS_MATE_REVERSE(record)) {
        reverse_mate_reads++;
    } else {
        forward_mate_reads++;
    }

    if (IS_PAIRED(record)) {
        paired_reads++;
    }

    if (IS_QCFAIL(record)) {
        qcfailed_reads++;
        log_problematic_read(problem_log, header, record, "QC failed");
    } else if (!IS_PAIRED(record)) {
        unpaired_reads++;
        log_problematic_read(problem_log, header, record, "Unpaired");
    } else if (IS_UNMAPPED(record)) {
        unmapped_reads++;
        log_problematic_read(problem_log, header, record, "Unmapped");
    } else if (IS_MATE_UNMAPPED(record)) {
        unmapped_mate_reads++;
        log_problematic_read(problem_log, header, record, "Unmapped mate");
    } else if (is_rf(record)) {
        rf_reads++;
        log_problematic_read(problem_log, header, record, "RF");
    } else if (is_ff(record)) {
        ff_reads++;
        log_problematic_read(problem_log, header, record, "FF");
    } else if (is_rr(record)) {
        rr_reads++;
        log_problematic_read(problem_log, header, record, "RR");
    } else if (record->core.qual == 0) {
        reads_mapped_with_zero_quality++;
        log_problematic_read(problem_log, header, record, "Mapped with zero quality");
    } else if (IS_PAIRED_AND_MAPPED(record)) {
        paired_and_mapped_reads++;

        if (IS_PROPERLYPAIRED(record)) {
            properly_paired_and_mapped_reads++;

            if (is_fr(record)) {
                fr_reads++;
            }

            if (IS_ORIGINAL(record)) {
                // record proper pairs' fragment lengths
                fragment_length_counts[fragment_length]++;

                // Keep track of the longest fragment seen in a proper
                // pair (ignoring secondary and supplementary
                // alignments). BWA has an idea of the maximum reasonable
                // fragment size a proper pair can have, but rather than
                // choose one aligner-specific heuristic, we'll just go
                // with the observed result, and hopefully work with other
                // aligners too.
                //
                // When we've added all the reads, we'll use this to
                // identify those that mapped too far from their
                // mates.
                //
                if (maximum_proper_pair_fragment_size < fragment_length) {
                    maximum_proper_pair_fragment_size = fragment_length;
                }
            }
        } else if (record->core.tid != record->core.mtid) {
            // Compare the record's reference ID to its mate's
            // reference ID. If they're different, the internet is
            // full of interesting explanations. This might be
            // because of adapter errors, where pairs of fragments
            // that each have one adapter attached look like one
            // proper fragment with both adapters. Or maybe you have
            // something interesting: translocations, fusions, or in
            // the case of allosomal references, perhaps a chimera, a
            // pregnant mother with offspring of a different gender,
            // or simply alignment to regions homologous between the X
            // and Y chromosomes.
            reads_with_mate_mapped_to_different_reference++;
            log_problematic_read(problem_log, header, record, "Mate mapped to different reference");
        } else {
            // OK, the read was paired, and mapped, but not in a
            // proper pair, for a reason we don't yet know. Its
            // mate may have mapped too far away, but we can't
            // check until we've seen all the reads.
            std::string record_name = get_qname(record);
            unlikely_fragment_sizes[record_name].push_back(fragment_length);
            log_problematic_read(problem_log, header, record, "Improper");
        }
    } else {
        // Most cases should have been caught by now, so let's
        // make a special note of any unexpected oddballs.
        unclassified_reads++;
        log_problematic_read(problem_log, header, record, "Unclassified");
    }

    // if the read mapped, then there should be a valid target ID
    // and we should be able to retrieve the target reference
    // sequence name, so we can distinguish autosomal reads
    if (!IS_UNMAPPED(record) && !IS_MATE_UNMAPPED(record) && (record->core.tid >= 0)) {
        std::string reference_name(header->target_name[record->core.tid]);
        if (is_autosomal(reference_name)) {
            total_autosomal_reads++;
            if (IS_DUP(record)) {
                duplicate_autosomal_reads++;
            } else {
                // nonduplicate, properly paired and uniquely mapped
                // autosomal reads will be the basis of our fragment
                // size and peak statistics
                //if (IS_PAIRED_AND_MAPPED(record) && IS_PROPERLYPAIRED(record) && IS_ORIGINAL(record)) {
                if (is_hqaa(header, record)) {
                    hqaa++;

                    hqaa_fragment_length_counts[fragment_length]++;

                    if (50 <= fragment_length && fragment_length <= 100) {
                        hqaa_tf_count++;
                    }

                    if (150 <= fragment_length && fragment_length <= 200) {
                        hqaa_mononucleosomal_count++;
                    }

                }
            }
        } else {
            if (mitochondrial_reference_name.compare(reference_name) == 0) {
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
void Metrics::load_alignments() {

    samFile *alignment_file = nullptr;
    bam_hdr_t *alignment_file_header = nullptr;
    bam1_t *record = bam_init1();
    boost::chrono::system_clock::time_point start;
    boost::chrono::duration<double> duration;

    if (alignment_filename.empty()) {
        throw FileException("Alignment file has not been specified.");
    }

    if ((alignment_file = sam_open(alignment_filename.c_str(), "r")) == 0) {
        throw FileException("Could not open alignment file \"" + alignment_filename + "\".");
    }

    if (verbose) {
        std::cout << "Collecting statistics from " << alignment_filename << "..." << std::endl;
    }

    start = boost::chrono::system_clock::now();

    if ((alignment_file_header = sam_hdr_read(alignment_file)) == 0) {
        throw FileException("Could not read a valid header from alignment file \"" + alignment_filename +  "\".");
    }

    boost::shared_ptr<boost::iostreams::filtering_ostream> problem_log;

    if (!problematic_reads_filename.empty()) {
        std::cerr << "Writing problematic reads to " << problematic_reads_filename << std::endl;
        try {
            problem_log = mostream(problematic_reads_filename);
        } catch (FileException& e) {
            std::cerr << "Could not open problematics read log \"" << problematic_reads_filename << "\": " << e.what() << std::endl;
            exit(1);
        }
    }

    while (sam_read1(alignment_file, alignment_file_header, record) >= 0) {
        add_alignment(alignment_file_header, record, problem_log);
    }

    if (verbose) {
        duration = boost::chrono::system_clock::now() - start;
        std::cout << "Added " << total_reads << " reads in " << duration << "." << std::endl << std::endl;
    }

    // last-minute classification of undiagnosed reads
    for (auto&& suspect_iterator : unlikely_fragment_sizes) {
        for (auto&& unlikely_fragment_size : suspect_iterator.second) {
            if (maximum_proper_pair_fragment_size < unlikely_fragment_size) {
                reads_with_mate_too_distant++;
                log(problem_log, "Mate too distant\t" + suspect_iterator.first);
            } else {
                reads_mapped_and_paired_but_improperly++;
                log(problem_log, "Undiagnosed\t" + suspect_iterator.first);
            }
        }
        suspect_iterator.second.clear();
    }
}


void Metrics::load_peaks() {
    htsFile* alignment_file = nullptr;
    hts_idx_t* alignment_file_index = nullptr;
    bam_hdr_t* alignment_file_header = nullptr;
    hts_itr_t* alignment_iterator = nullptr;
    std::stringstream query_region;
    bam1_t *record = bam_init1();
    boost::chrono::system_clock::time_point start;
    boost::chrono::duration<double> duration;

    boost::shared_ptr<boost::iostreams::filtering_istream> peak_istream = mistream(peak_filename);
    unsigned long long int count = 0;
    Peak peak;

    if (alignment_filename.empty()) {
        throw FileException("Alignment file has not been specified.");
    }

    if (peak_filename.empty()) {
        throw FileException("Peak file has not been specified.");
    }

    if ((alignment_file = sam_open(alignment_filename.c_str(), "r")) == 0) {
        throw FileException("Could not open alignment file \"" + alignment_filename + "\".");
    }

    if ((alignment_file_header = sam_hdr_read(alignment_file)) == 0) {
        throw FileException("Could not read a valid header from alignment file \"" + alignment_filename +  "\".");
    }

    alignment_file_index = sam_index_load(alignment_file, alignment_filename.c_str());
    if (!alignment_file_index) {
        throw FileException("Could not load index for alignment file \"" + alignment_filename + "\".");
    }

    if (verbose) {
        std::cout << "Reading peaks from " << peak_filename << "." << std::endl << std::flush;
    }

    start = boost::chrono::system_clock::now();

    while (*peak_istream >> peak) {
        bool excluded = false;
        for (auto er : excluded_regions) {
            if (peak.overlaps(er)) {
                std::cerr << "Excluding peak " << peak << " which overlaps excluded region " << er << std::endl;
                excluded = true;
                break;
            }
        }
        if (!excluded) {
            // count the reads overlapping this peak. Alignment (BAM)
            // and peak (BED) files should use the same zero-based
            // coordinates.
            query_region.str("");
            query_region << peak.reference << ":" << peak.start << "-" << peak.end;
            alignment_iterator = sam_itr_querys(alignment_file_index, alignment_file_header, query_region.str().c_str());
            if (alignment_iterator) {
                while (sam_itr_next(alignment_file, alignment_iterator, record) >= 0) {
                    peak.overlapping_reads++;
                    if (is_hqaa(alignment_file_header, record)) {
                        peak.overlapping_hqaa++;
                    }
                }
            }
            peaks.push_back(peak);
            total_peak_territory += peak.size();
        }
    }

    // sort the peaks by the number of overlapping reads, most to least
    std::sort(peaks.begin(), peaks.end(), peak_overlapping_reads_descending_comparator);
    for (auto peak: peaks) {
        count++;
        reads_in_peaks += peak.overlapping_reads;
        if (count == 1) {
            top_peak_read_count = reads_in_peaks;
        }

        if (count <= 10) {
            top_10_peak_read_count = reads_in_peaks;
        }

        if (count <= 100) {
            top_100_peak_read_count = reads_in_peaks;
        }

        if (count <= 1000) {
            top_1000_peak_read_count = reads_in_peaks;
        }

        if (count <= 10000) {
            top_10000_peak_read_count = reads_in_peaks;
        }
    }

    // sort the peaks by the number of overlapping high quality autosomal aligments, most to least
    std::sort(peaks.begin(), peaks.end(), peak_overlapping_hqaa_descending_comparator);
    count = 0;
    for (auto peak: peaks) {
        count++;
        hqaa_in_peaks += peak.overlapping_hqaa;
        if (count == 1) {
            top_peak_hqaa_read_count = hqaa_in_peaks;
        }

        if (count <= 10) {
            top_10_peak_hqaa_read_count = hqaa_in_peaks;
        }

        if (count <= 100) {
            top_100_peak_hqaa_read_count = hqaa_in_peaks;
        }

        if (count <= 1000) {
            top_1000_peak_hqaa_read_count = hqaa_in_peaks;
        }

        if (count <= 10000) {
            top_10000_peak_hqaa_read_count = hqaa_in_peaks;
        }
    }



    if (peaks.empty()) {
        std::cerr << "No peaks were found in " << peak_filename << std::endl;
    } else if (verbose) {
        duration = boost::chrono::system_clock::now() - start;
        std::cout << "Loaded " << count << " peaks in " << duration << "." << std::endl << std::endl;
    }
}


///
/// Present the Metrics in plain text
///
std::ostream& operator<<(std::ostream& os, const Metrics& m) {
    unsigned long long int total_problems = (m.unmapped_reads +
                                             m.unmapped_mate_reads +
                                             m.qcfailed_reads +
                                             m.unpaired_reads +
                                             m.reads_with_mate_mapped_to_different_reference +
                                             m.reads_mapped_with_zero_quality +
                                             m.reads_with_mate_too_distant +
                                             m.rf_reads +
                                             m.ff_reads +
                                             m.rr_reads +
                                             m.reads_mapped_and_paired_but_improperly);

    os << m.configuration_string()

       << "READ METRICS" << std::endl
       << "============" << std::endl
       << "Total reads: " << m.total_reads << std::endl
       << "Forward reads: " << percentage_string(m.forward_reads, m.total_reads) << std::endl
       << "Reverse reads: " << percentage_string(m.reverse_reads, m.total_reads) << std::endl
       << "Secondary reads: " << percentage_string(m.secondary_reads, m.total_reads) << std::endl
       << "Supplementary reads: " << percentage_string(m.supplementary_reads, m.total_reads) << std::endl
       << "Duplicate reads: " << percentage_string(m.duplicate_reads, m.total_reads, 3, " (", "% of all reads)") << std::endl << std::endl

       << "Paired read metrics" << std::endl
       << "-------------------" << std::endl
       << "Paired reads: " << percentage_string(m.paired_reads, m.total_reads) << std::endl
       << "Paired and mapped reads: " << percentage_string(m.paired_and_mapped_reads, m.total_reads) << std::endl
       << "Properly paired and mapped reads: " << percentage_string(m.properly_paired_and_mapped_reads, m.total_reads) << std::endl
       << "FR reads: " << percentage_string(m.fr_reads, m.total_reads, 6) << std::endl
       << "First of pair: " << percentage_string(m.first_reads, m.total_reads) << std::endl
       << "Second of pair: " << percentage_string(m.second_reads, m.total_reads) << std::endl
       << "Forward mate reads: " << percentage_string(m.forward_mate_reads, m.total_reads) << std::endl
       << "Reverse mate reads: " << percentage_string(m.reverse_mate_reads, m.total_reads) << std::endl

       << std::endl

       << "Problematic reads" << std::endl
       << "-----------------" << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "Unmapped reads: " <<percentage_string(m.unmapped_reads, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "Unmapped mate reads: " <<percentage_string(m.unmapped_mate_reads, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "Reads not passing quality controls: " << percentage_string(m.qcfailed_reads, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "Unpaired reads: " << percentage_string(m.unpaired_reads, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "RF reads: " << percentage_string(m.rf_reads, m.total_reads, 6) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "FF reads: " << percentage_string(m.ff_reads, m.total_reads, 6) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "RR reads: " << percentage_string(m.rr_reads, m.total_reads, 6) << std::endl << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "Reads that paired and mapped but..." << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  on different references: " << percentage_string(m.reads_with_mate_mapped_to_different_reference, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  with zero quality: " << percentage_string(m.reads_mapped_with_zero_quality, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  probably too far from their mates: " << percentage_string(m.reads_with_mate_too_distant, m.total_reads) << " (longest proper fragment seems to be " << m.maximum_proper_pair_fragment_size << ")" << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  just not properly: " << percentage_string(m.reads_mapped_and_paired_but_improperly, m.total_reads) << std::endl << std::endl

       << std::setfill(' ') << std::left << std::setw(40) << "Total problems: " << percentage_string(total_problems, m.total_reads) << std::endl << std::endl;

    if (m.unclassified_reads == 0 && (total_problems + m.properly_paired_and_mapped_reads == m.total_reads)) {
        os << "All reads classified. Total problems + properly paired and mapped reads == total reads." << std::endl;
    } else {
        os << "Some reads were not classified: " << percentage_string(m.total_reads - m.unclassified_reads - m.properly_paired_and_mapped_reads - total_problems, m.total_reads) << std::endl
           << "We'd like to know what we're missing. If it would be possible for you to share the unclassified reads, please file an issue at: " << std::endl << std::endl
           << "    https://github.com/ParkerLab/ataqc/issues" << std::endl;
    }

    os << std::endl
       << "AUTOSOMAL/MITOCHONDRIAL METRICS" << std::endl
       << "===============================" << std::endl
       << "Total autosomal reads: " << percentage_string(m.total_autosomal_reads, m.total_reads, 3,  " (", "% of all reads)") << std::endl
       << "Total mitochondrial reads: " << percentage_string(m.total_mitochondrial_reads, m.total_reads, 3, " (", "% of all reads)") << std::endl
       << "Duplicate autosomal reads: " << percentage_string(m.duplicate_autosomal_reads, m.total_autosomal_reads, 3, " (", "% of all autosomal reads)") << std::endl
       << "Duplicate mitochondrial reads: " << percentage_string(m.duplicate_mitochondrial_reads, m.total_mitochondrial_reads, 3, " (", "% of all mitochondrial reads)") << std::endl << std::endl

       << "High quality -- nonduplicate, properly paired and uniquely mapped -- autosomal alignments: " << m.hqaa << std::endl
       << "  as a percentage of autosomal reads: " << std::setprecision(3) << std::fixed << percentage(m.hqaa, m.total_autosomal_reads) << "%" << std::endl
       << "  as a percentage of all reads: " << std::setprecision(3) << std::fixed << percentage(m.hqaa, m.total_reads) << "%" << std::endl << std::endl;

    os << "SHORT/MONONUCLEOSOMAL FRAGMENT RATIO" << std::endl
       << "====================================" << std::endl
       << "High quality autosomal alignments in fragments 50-100bp long: " << percentage_string(m.hqaa_tf_count, m.hqaa, 3, " (", "% of all high quality autosomal aligments)") << std::endl
       << "High quality autosomal alignments in fragments 150-200bp long: " << percentage_string(m.hqaa_mononucleosomal_count, m.hqaa, 3, " (", "% of all high quality autosomal aligments)") << std::endl
       << "Short to mononucleosomal ratio: " << std::setprecision(3) << std::fixed << fraction(m.hqaa_tf_count, m.hqaa_mononucleosomal_count) << std::endl << std::endl;

    os << "MAPPING QUALITY" << std::endl
       << "===============" << std::endl
       << "Mean MAPQ: " << std::fixed << m.mean_mapq() << std::endl
       << "Median MAPQ: " << std::fixed << m.median_mapq() << std::endl;

    os << "Reads with MAPQ >=..." << std::endl;
    for (int threshold = 5; threshold <= 30; threshold += 5) {
        unsigned long long int count = 0;
        for (auto it : m.mapq_counts) {
            if (it.first >= threshold) {
                count += it.second;
            }
        }
        os << std::setfill(' ') << std::setw(20) << std::right << threshold << ": " << percentage_string(count, m.total_reads) << std::endl;
    }

    os << "PEAK METRICS" << std::endl
       << "------------" << std::endl
       << "Peak count: " << m.peaks.size() << std::endl <<std::endl

       << "Reads that overlapped peaks: "  << percentage_string(m.reads_in_peaks, m.total_reads, 3, " (", "% of all reads)") << std::endl
       << "Number of reads overlapping the top 10,000 peaks: " << std::endl
       << std::setfill(' ') << std::setw(22) << std::right << "Top peak: " << std::fixed << percentage_string(m.top_peak_read_count, m.total_reads, 3, " (", "% of all reads)") << std::endl
       << std::setfill(' ') << std::setw(22) << std::right << "Top 10 peaks: " << std::fixed << percentage_string(m.top_10_peak_read_count, m.total_reads, 3, " (", "% of all reads)") << std::endl
       << std::setfill(' ') << std::setw(22) << std::right << "Top 100 peaks: "<< std::fixed << percentage_string(m.top_100_peak_read_count, m.total_reads, 3, " (", "% of all reads)") << std::endl
       << std::setfill(' ') << std::setw(22) << std::right << "Top 1000 peaks: " << std::fixed << percentage_string(m.top_1000_peak_read_count, m.total_reads, 3, " (", "% of all reads)") << std::endl
       << std::setfill(' ') << std::setw(22) << std::right << "Top 10,000 peaks: " << std::fixed << percentage_string(m.top_10000_peak_read_count, m.total_reads, 3, " (", "% of all reads)") << std::endl

       << std::endl

       << "High quality autosomal aligments that overlapped peaks: "  << percentage_string(m.hqaa_in_peaks, m.hqaa, 3, " (", "% of all high quality autosomal alignments)") << std::endl
       << "Number of high quality autosomal aligments overlapping the top 10,000 peaks: " << std::endl
       << std::setfill(' ') << std::setw(22) << std::right << "Top peak: " << std::fixed << percentage_string(m.top_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal aligments)") << std::endl
       << std::setfill(' ') << std::setw(22) << std::right << "Top 10 peaks: " << std::fixed << percentage_string(m.top_10_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal aligments)") << std::endl
       << std::setfill(' ') << std::setw(22) << std::right << "Top 100 peaks: "<< std::fixed << percentage_string(m.top_100_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal aligments)") << std::endl
       << std::setfill(' ') << std::setw(22) << std::right << "Top 1000 peaks: " << std::fixed << percentage_string(m.top_1000_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal aligments)") << std::endl
       << std::setfill(' ') << std::setw(22) << std::right << "Top 10,000 peaks: " << std::fixed << percentage_string(m.top_10000_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal aligments)") << std::endl;
    return os;
}


json Metrics::to_json() {
    std::vector<std::string> fragment_length_counts_fields = {"fragment_length", "read_count", "fraction_of_all_reads"};
    json fragment_length_counts_json;
    int max_fragment_length = std::max(1000, fragment_length_counts.empty() ? 0 : fragment_length_counts.rbegin()->first);

    for (int fragment_length = 0; fragment_length < max_fragment_length; fragment_length++) {
        int count = fragment_length_counts[fragment_length];
        json flc;
        flc.push_back(fragment_length);
        flc.push_back(count);
        flc.push_back(fraction(count, total_reads));

        fragment_length_counts_json.push_back(flc);
    }

    std::vector<std::string> hqaa_fragment_length_counts_fields = {"fragment_length", "read_count", "fraction_of_hqaa"};
    json hqaa_fragment_length_counts_json;
    max_fragment_length = std::max(1000, hqaa_fragment_length_counts.empty() ? 0 : hqaa_fragment_length_counts.rbegin()->first);
    for (int fragment_length = 0; fragment_length < max_fragment_length; fragment_length++) {
        int count = hqaa_fragment_length_counts[fragment_length];
        json flc;
        flc.push_back(fragment_length);
        flc.push_back(count);
        flc.push_back(fraction(count, hqaa));

        hqaa_fragment_length_counts_json.push_back(flc);
    }

    std::vector<std::string> mapq_counts_fields = {"mapq", "read_count"};

    json mapq_counts_json;
    for (auto it : mapq_counts) {
        json mc;
        mc.push_back(it.first);
        mc.push_back(it.second);
        mapq_counts_json.push_back(mc);
    }

    std::vector<std::string> peaks_fields = {
        "name",
        "overlapping_hqaa",
        "territory"
    };

    std::vector<json> peak_list;
    unsigned long long int peak_count = 0;
    unsigned long long int hqaa_overlapping_peaks = 0;

    for (auto peak: peaks) {
        peak_count++;
        hqaa_overlapping_peaks += peak.overlapping_hqaa;

        json jp;
        jp.push_back(peak.name);
        jp.push_back(peak.overlapping_hqaa);
        jp.push_back(peak.size());

        peak_list.push_back(jp);
    }

    json result = {
        {"ataqc_version", version_string()},
        {"timestamp", iso8601_timestamp()},
        {"metrics",
         {
             {"name", name},
             {"description", description},
             {"url", url},
             {"organism", organism},
             {"library", library},
             {"cell_count", cell_count},
             {"reference_genome", reference_genome},
             {"total_reads", total_reads},
             {"hqaa", hqaa},
             {"forward_reads", forward_reads},
             {"reverse_reads", reverse_reads},
             {"secondary_reads", secondary_reads},
             {"supplementary_reads", supplementary_reads},
             {"duplicate_reads", duplicate_reads},
             {"paired_reads", paired_reads},
             {"properly_paired_and_mapped_reads", properly_paired_and_mapped_reads},
             {"fr_reads", fr_reads},
             {"ff_reads", ff_reads},
             {"rf_reads", rf_reads},
             {"rr_reads", rr_reads},
             {"first_reads", first_reads},
             {"second_reads", second_reads},
             {"forward_mate_reads", forward_mate_reads},
             {"reverse_mate_reads", reverse_mate_reads},
             {"unmapped_reads", unmapped_reads},
             {"unmapped_mate_reads", unmapped_mate_reads},
             {"qcfailed_reads", qcfailed_reads},
             {"unpaired_reads", unpaired_reads},
             {"reads_with_mate_mapped_to_different_reference", reads_with_mate_mapped_to_different_reference},
             {"reads_mapped_with_zero_quality", reads_mapped_with_zero_quality},
             {"reads_mapped_and_paired_but_improperly", reads_mapped_and_paired_but_improperly},
             {"unclassified_reads", unclassified_reads},
             {"maximum_proper_pair_fragment_size", maximum_proper_pair_fragment_size},
             {"reads_with_mate_too_distant", reads_with_mate_too_distant},
             {"total_autosomal_reads", total_autosomal_reads},
             {"total_mitochondrial_reads", total_mitochondrial_reads},
             {"duplicate_autosomal_reads", duplicate_autosomal_reads},
             {"duplicate_mitochondrial_reads", duplicate_mitochondrial_reads},
             {"hqaa_tf_count", hqaa_tf_count},
             {"hqaa_mononucleosomal_count", hqaa_mononucleosomal_count},
             {"short_mononucleosomal_ratio", fraction(hqaa_tf_count, hqaa_mononucleosomal_count)},
             {"fragment_length_counts_fields", fragment_length_counts_fields},
             {"fragment_length_counts", fragment_length_counts_json},
             {"hqaa_fragment_length_counts_fields", hqaa_fragment_length_counts_fields},
             {"hqaa_fragment_length_counts", hqaa_fragment_length_counts_json},
             {"mapq_counts_fields", mapq_counts_fields},
             {"mapq_counts", mapq_counts_json},
             {"mean_mapq", mean_mapq()},
             {"median_mapq", median_mapq()},
             {"peaks_fields", peaks_fields},
             {"peaks", peak_list},
             {"total_peaks", peak_count},
             {"total_peak_territory", total_peak_territory},
             {"hqaa_overlapping_peaks_percent", percentage(hqaa_overlapping_peaks, hqaa)}
         }
        }
    };
    return result;
}
