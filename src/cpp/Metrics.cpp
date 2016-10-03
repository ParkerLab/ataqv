//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#include <cmath>
#include <cstdarg>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <boost/chrono.hpp>

#include "Features.hpp"
#include "HTS.hpp"
#include "IO.hpp"
#include "Metrics.hpp"
#include "Utils.hpp"


MetricsCollector::MetricsCollector(const std::string& name,
                                   const std::string& organism,
                                   const std::string& description,
                                   const std::string& library_description,
                                   const std::string& url,
                                   const std::string& alignment_filename,
                                   const std::string& autosomal_reference_filename,
                                   const std::string& mitochondrial_reference_name,
                                   const std::string& peak_filename,
                                   const std::vector<std::string>& excluded_region_filenames,
                                   bool verbose,
                                   bool log_problematic_reads) :
    metrics({}),
    name(name),
    organism(organism),
    description(description),
    library_description(library_description),
    url(url),
    alignment_filename(alignment_filename),
    autosomal_reference_filename(autosomal_reference_filename),
    mitochondrial_reference_name(mitochondrial_reference_name),
    peak_filename(peak_filename),
    excluded_region_filenames(excluded_region_filenames),
    log_problematic_reads(log_problematic_reads),
    verbose(verbose)
{

    std::map<std::string, int> default_references = {
        {"human", 22},
        {"mouse", 19},
        {"rat", 20}
    };

    for (auto r : default_references) {
        for (int i = 1; i <= r.second; i++) {
            autosomal_references[r.first][std::to_string(i)] = 1;
            autosomal_references[r.first]["chr" + std::to_string(i)] = 1;
        }
    }

    if (!autosomal_reference_filename.empty()) {
        load_autosomal_references();
    }

    if (verbose) {
        std::cout << configuration_string();
    }

    if (!excluded_region_filenames.empty()) {
        load_excluded_regions();
    }
}


std::string MetricsCollector::configuration_string() const {
    std::stringstream cs;
    cs << "ataqc " << version_string() << std::endl << std::endl

       << "Experiment information" << std::endl
       << "======================" << std::endl
       << "Organism: " << organism << std::endl
       << "Description: " << description << std::endl
       << "URL: " << url << std::endl << std::endl

       << "Reference genome configuration" << std::endl
       << "==============================" << std::endl
       << "Mitochondrial reference: " << mitochondrial_reference_name << std::endl
       << "Autosomal references: " << std::endl << wrap(autosomal_reference_string(), 72, 2) << std::endl << std::endl;

    return cs.str();
}


std::string MetricsCollector::autosomal_reference_string() const {
    if (autosomal_references.count(organism) == 0) {
        return "";
    }
    std::vector<std::string> ars;
    for (const auto& it : autosomal_references.at(organism)) {
        ars.push_back(it.first);
    }

    std::sort(ars.begin(), ars.end(), sort_strings_numerically);

    auto last = --ars.end();

    std::stringstream ss;
    for (auto i = ars.begin(); i != ars.end(); i++) {
        ss << *i;
        if (i != last) {
            ss << ", ";
        }
    }
    return ss.str();
}


///
/// Read autosomal references from a file, one per line, creating or
/// replacing an entry for them under the given reference genome in
/// genome_autosomal_references.
///
void MetricsCollector::load_autosomal_references() {
    if (!autosomal_reference_filename.empty()) {
        std::string reference_name;
        boost::shared_ptr<boost::iostreams::filtering_istream> reference_file;

        try {
            reference_file = mistream(autosomal_reference_filename);
        } catch (FileException& e) {
            throw FileException("Could not open the supplied autosomal reference file \"" + autosomal_reference_filename + "\": " + e.what());
        }

        if (verbose) {
            std::cout << "Reading " << organism << " autosomal references from " << autosomal_reference_filename << "." << std::endl;
        }

        // clear any existing references for this genome
        autosomal_references[organism] = {};

        // load the file
        while (*reference_file >> reference_name) {
            autosomal_references[organism][reference_name] = 1;
        }

        if (verbose) {
            std::cout << "Autosomal references for " << organism << ": " << std::endl;
            for (const auto it : autosomal_references[organism]) {
                std::cout << "\t" << it.first << std::endl;
            }
        }
    }
}


bool MetricsCollector::is_autosomal(const std::string& reference_name) {
    static std::map<std::string, bool> refcache;

    if (refcache.count(reference_name) == 0) {
        refcache[reference_name] = autosomal_references[organism].find(reference_name) != autosomal_references[organism].end();
    }

    return refcache.at(reference_name);
}


bool MetricsCollector::is_mitochondrial(const std::string& reference_name) {
    return mitochondrial_reference_name.compare(reference_name) == 0;
}


///
/// Measure all the reads in a BAM file
///
void MetricsCollector::load_alignments() {

    samFile *alignment_file = nullptr;
    bam_hdr_t *alignment_file_header = nullptr;
    bam1_t *record = bam_init1();
    boost::chrono::high_resolution_clock::time_point start;
    boost::chrono::duration<double> duration;
    double rate;
    unsigned long long int total_reads = 0;

    if (alignment_filename.empty()) {
        throw FileException("Alignment file has not been specified.");
    }

    if ((alignment_file = sam_open(alignment_filename.c_str(), "r")) == 0) {
        throw FileException("Could not open alignment file \"" + alignment_filename + "\".");
    }

    if (verbose) {
        std::cout << "Collecting metrics from " << alignment_filename << "." << std::endl << std::endl;
    }

    start = boost::chrono::high_resolution_clock::now();

    if ((alignment_file_header = sam_hdr_read(alignment_file)) == 0) {
        throw FileException("Could not read a valid header from alignment file \"" + alignment_filename +  "\".");
    }

    std::string metrics_id;

    std::shared_ptr<MetricsCollector> collector(this);
    sam_header header = parse_sam_header(alignment_file_header->text);
    if (header.count("RG") > 0) {
        for (auto read_group : header["RG"]) {
            metrics_id = read_group["ID"];
            metrics[metrics_id] = new Metrics(collector, metrics_id);

            Library library;
            library.library = read_group["LB"];
            library.sample = read_group["SM"];
            library.description = library_description.empty() ? read_group["DS"] : library_description;
            library.center = read_group["CN"];
            library.date = read_group["DT"];
            library.platform = read_group["PL"];
            library.platform_model = read_group["PM"];
            library.platform_unit = read_group["PU"];
            library.flow_order = read_group["FO"];
            library.key_sequence = read_group["KS"];
            library.programs = read_group["PG"];
            library.predicted_median_insert_size = read_group["PI"];

            metrics[metrics_id]->library = library;
        }
    } else {
        std::cerr << "No read groups found in alignment file." << std::endl;
        if (peak_filename == "auto") {
            peak_filename = "";
        }

        metrics_id = name.empty() ? alignment_filename : name;
        metrics[metrics_id] = new Metrics(collector, metrics_id);

        Library library;
        library.description = library_description;
        metrics[metrics_id]->library = library;
    }

    while (sam_read1(alignment_file, alignment_file_header, record) >= 0) {
        uint8_t* aux = bam_aux_get(record, "RG");
        if (aux) {
            metrics_id = std::string(bam_aux2Z(aux));
            metrics[metrics_id]->add_alignment(alignment_file_header, record);
        } else {
            metrics[metrics_id]->add_alignment(alignment_file_header, record);
        }
        total_reads++;

        if (verbose && total_reads % 100000 == 0) {
            duration = boost::chrono::high_resolution_clock::now() - start;
            rate = (total_reads / duration.count());
            std::cout << "Analyzed " << total_reads << " reads in " << duration << " (" << rate << " reads/second)." << std::endl;
        }
    }
    bam_destroy1(record);
    bam_hdr_destroy(alignment_file_header);
    hts_close(alignment_file);

    for (auto& m : metrics) {
        m.second->make_aggregate_diagnoses();
        m.second->determine_top_peaks();
    }

    if (verbose) {
        duration = boost::chrono::high_resolution_clock::now() - start;
        std::cout << "Analyzed " << total_reads << " reads in " << duration << " (" << (total_reads / duration.count()) << " reads/second)." << std::endl << std::endl;
    }
}


Metrics::Metrics(std::shared_ptr<MetricsCollector> collector, const std::string& name): collector(collector), name(name), peaks(), log_problematic_reads (collector->log_problematic_reads) {
    if (!collector->peak_filename.empty()) {
        peaks_requested = true;
        load_peaks();
    }
}


std::string Metrics::make_metrics_filename(const std::string& suffix) {
    return name + suffix;
}


void Metrics::open_problematic_read_stream() {
    problematic_read_filename = make_metrics_filename(".problems.gz");

    if (collector->verbose) {
        std::cout << "Logging problematic reads to " << problematic_read_filename << "." << std::endl << std::endl;
    }

    problematic_read_stream = mostream(problematic_read_filename);
}


void Metrics::log_problematic_read(const std::string& problem, const std::string& record) {
    if (!log_problematic_reads) {
        return;
    }

    if (!(problematic_read_stream && problematic_read_stream->is_complete())) {
        try {
            open_problematic_read_stream();
        } catch (FileException& e) {
            std::cerr << "Could not open problematic read file: " <<  e.what() << std::endl << std::flush;
            exit(1);
        }
    }

    *problematic_read_stream << problem;

    if (!record.empty()) {
        *problematic_read_stream << "\t" << record;
    }

    *problematic_read_stream << std::endl << std::flush;
}


void Metrics::make_aggregate_diagnoses() {
    // last-minute classification of undiagnosed reads
    reads_with_mate_too_distant = 0;
    reads_mapped_and_paired_but_improperly = 0;

    for (auto&& suspect_iterator : unlikely_fragment_sizes) {
        for (auto&& unlikely_fragment_size : suspect_iterator.second) {
            if (maximum_proper_pair_fragment_size < unlikely_fragment_size) {
                reads_with_mate_too_distant++;
                if (log_problematic_reads) {
                    log_problematic_read("Mate too distant", suspect_iterator.first);
                }
            } else {
                reads_mapped_and_paired_but_improperly++;
                if (log_problematic_reads) {
                    log_problematic_read("Undiagnosed", suspect_iterator.first);
                }
            }
        }
        suspect_iterator.second.clear();
    }
}


bool Metrics::is_autosomal(const std::string& reference_name) {
    return collector->is_autosomal(reference_name);
}


bool Metrics::is_mitochondrial(const std::string& reference_name) {
    return collector->is_mitochondrial(reference_name);
}


std::string Metrics::configuration_string() const {
    std::stringstream cs;
    cs << "Read Group: " << name << std::endl << library << std::endl;
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
    double median = 0.0;

    if (total_reads == 0) {
        return 0;
    }

    unsigned long long int median1;
    unsigned long long int median2;

    // if the number of reads is even, we need to take the mean of the
    // two values around the ideal median, but with an odd number of
    // reads, we just need the existent median value

    if (total_reads % 2 == 0) {
        median1 = ((unsigned long long int)total_reads / 2) - 1;
        median2 = (unsigned long long int)total_reads / 2;
    } else {
        median1 = median2 = (unsigned long long int)total_reads / 2;
    }

    unsigned long long int mapq_index = 0;
    unsigned long long int next_mapq_index = 0;
    for (auto it : mapq_counts) {
        next_mapq_index = mapq_index + it.second;
        bool median1_here = (mapq_index <= median1 && median1 <= next_mapq_index);
        bool median2_here = (mapq_index <= median2 && median2 <= next_mapq_index);

        if (median1_here) {
            median += it.first;
        }

        if (median2_here) {
            median += it.first;
            median /= 2;
        }
        mapq_index += it.second;
    }
    return median;
}


void MetricsCollector::load_excluded_regions() {
    if (excluded_region_filenames.empty()) {
        std::cerr << "No excluded region files have been specified." << std::endl;
    }

    for (auto filename : excluded_region_filenames) {
        boost::shared_ptr<boost::iostreams::filtering_istream> region_file;
        Feature region;
        unsigned long long int count = 0;

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


///
/// Measure and record a single read
///
void Metrics::add_alignment(const bam_hdr_t* header, const bam1_t* record) {
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
        if (log_problematic_reads) {
            log_problematic_read("QC failed", record_to_string(header, record));
        }
    } else if (!IS_PAIRED(record)) {
        unpaired_reads++;
        if (log_problematic_reads) {
            log_problematic_read("Unpaired", record_to_string(header, record));
        }
    } else if (IS_UNMAPPED(record)) {
        unmapped_reads++;
        if (log_problematic_reads) {
            log_problematic_read("Unmapped", record_to_string(header, record));
        }
    } else if (IS_MATE_UNMAPPED(record)) {
        unmapped_mate_reads++;
        if (log_problematic_reads) {
            log_problematic_read("Unmapped mate", record_to_string(header, record));
        }
    } else if (is_rf(record)) {
        rf_reads++;
        if (log_problematic_reads) {
            log_problematic_read("RF", record_to_string(header, record));
        }
    } else if (is_ff(record)) {
        ff_reads++;
        if (log_problematic_reads) {
            log_problematic_read("FF", record_to_string(header, record));
        }
    } else if (is_rr(record)) {
        rr_reads++;
        if (log_problematic_reads) {
            log_problematic_read("RR", record_to_string(header, record));
        }
    } else if (record->core.qual == 0) {
        reads_mapped_with_zero_quality++;
        if (log_problematic_reads) {
            log_problematic_read("Mapped with zero quality", record_to_string(header, record));
        }
    } else if (IS_PAIRED_AND_MAPPED(record)) {
        paired_and_mapped_reads++;

        if (IS_PROPERLYPAIRED(record)) {
            properly_paired_and_mapped_reads++;

            if (is_fr(record)) {
                fr_reads++;
            }

            // we'll only assert that a read is autosomal or
            // mitochondrial if it's properly paired and mapped and
            // (of course) has a valid reference name
            if (record->core.tid >= 0) {
                std::string reference_name(header->target_name[record->core.tid]);
                if (is_autosomal(reference_name)) {
                    total_autosomal_reads++;
                    if (IS_DUP(record)) {
                        duplicate_autosomal_reads++;
                    }
                } else if (is_mitochondrial(reference_name)) {
                    total_mitochondrial_reads++;
                    if (IS_DUP(record)) {
                        duplicate_mitochondrial_reads++;
                    }
                }
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

                // nonduplicate, properly paired and uniquely mapped
                // autosomal reads will be the basis of our fragment
                // size and peak statistics
                if (is_hqaa(header, record)) {
                    hqaa++;

                    hqaa_fragment_length_counts[fragment_length]++;

                    if (50 <= fragment_length && fragment_length <= 100) {
                        hqaa_short_count++;
                    }

                    if (150 <= fragment_length && fragment_length <= 200) {
                        hqaa_mononucleosomal_count++;
                    }

                    if (!peaks.empty()) {
                        peaks.increment_overlapping_hqaa(Feature(header, record));
                    }
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
            if (log_problematic_reads) {
                log_problematic_read("Mate mapped to different reference", record_to_string(header, record));
            }
        } else {
            // OK, the read was paired, and mapped, but not in a
            // proper pair, for a reason we don't yet know. Its
            // mate may have mapped too far away, but we can't
            // check until we've seen all the reads.
            std::string record_name = get_qname(record);
            unlikely_fragment_sizes[record_name].push_back(fragment_length);
            if (log_problematic_reads) {
                log_problematic_read("Improper", record_to_string(header, record));
            }
        }
    } else {
        // Most cases should have been caught by now, so let's
        // make a special note of any unexpected oddballs.
        unclassified_reads++;
        if (log_problematic_reads) {
            log_problematic_read("Unclassified", record_to_string(header, record));
        }
    }
}


void Metrics::load_peaks() {
    std::string peak_filename = collector->peak_filename;

    if (peak_filename.empty()) {
        throw FileException("Peak file has not been specified.");
    } else if (peak_filename == "auto") {
        peak_filename = make_metrics_filename(".peaks");
    }

    if (collector->verbose) {
        std::cout << "Loading peaks for read group " << name << " from " << peak_filename << "." << std::endl << std::flush;
    }

    boost::shared_ptr<boost::iostreams::filtering_istream> peak_istream = mistream(peak_filename);
    boost::chrono::high_resolution_clock::time_point start = boost::chrono::high_resolution_clock::now();
    boost::chrono::duration<double> duration;
    Peak peak;

    while (*peak_istream >> peak) {
        if (!is_autosomal(peak.reference)) {
            continue;
        }
        bool excluded = false;
        for (auto er : collector->excluded_regions) {
            if (peak.overlaps(er)) {
                if (collector->verbose) {
                    std::cout << "Excluding peak [" << peak << "] which overlaps excluded region [" << er << "]" << std::endl;
                }
                excluded = true;
                break;
            }
        }
        if (!excluded) {
            peaks.add(peak);
            total_peak_territory += peak.size();
        }
    }

    peaks.sort_peaks();

    if (peaks.empty()) {
        std::cout << "No peaks were found in " << peak_filename << std::endl;
    } else if (collector->verbose) {
        duration = boost::chrono::high_resolution_clock::now() - start;
        peaks.print_reference_peak_counts();
        std::cout << "Loaded " << peaks.size() << " peaks in " << duration << "." << " (" << (peaks.size() / duration.count()) << " peaks/second)." << std::endl << std::endl;
    }
}


void Metrics::determine_top_peaks() {
    unsigned long long int count = 0;
    for (auto peak: peaks.list_peaks_by_overlapping_hqaa_descending()) {
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
}

std::ostream& operator<<(std::ostream& os, const Library& library) {
    os
       << "Library: " << library.library << std::endl
       << "Sample: " << library.sample << std::endl
       << "Description: " << library.description << std::endl << std::endl
       << "Sequencing center: " << library.center << std::endl
       << "Sequencing date: " << library.date << std::endl
       << "Sequencing platform: " << library.platform << std::endl
       << "Platform model: " << library.platform_model << std::endl
       << "Platform unit: " << library.platform_unit << std::endl
       << "Flow order: " << library.flow_order << std::endl
       << "Key sequence: " << library.key_sequence << std::endl
       << "Predicted median insert size: " << library.predicted_median_insert_size << std::endl
       << "Programs: " << library.programs << std::endl;
    return os;
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

       << "  Read Mapping Metrics" << std::endl
       << "  --------------------" << std::endl
       << "  Total reads: " << m.total_reads << std::endl
       << "  Total problems: " << percentage_string(total_problems, m.total_reads) << std::endl
       << "  Properly paired and mapped reads: " << percentage_string(m.properly_paired_and_mapped_reads, m.total_reads) << std::endl
       << "  Secondary reads: " << percentage_string(m.secondary_reads, m.total_reads) << std::endl
       << "  Supplementary reads: " << percentage_string(m.supplementary_reads, m.total_reads) << std::endl
       << "  Duplicate reads: " << percentage_string(m.duplicate_reads, m.total_reads, 3, " (", "% of all reads)") << std::endl

       << std::endl

       << "  Quality Indicators" << std::endl
       << "  ------------------" << std::endl
       << "  Short to mononucleosomal ratio: " << std::setprecision(3) << std::fixed << fraction(m.hqaa_short_count, m.hqaa_mononucleosomal_count) << std::endl
       << "  High quality, nonduplicate, properly paired, uniquely mapped autosomal alignments: " << m.hqaa << std::endl
       << "    as a percentage of autosomal reads: " << std::setprecision(3) << std::fixed << percentage(m.hqaa, m.total_autosomal_reads) << "%" << std::endl
       << "    as a percentage of all reads: " << std::setprecision(3) << std::fixed << percentage(m.hqaa, m.total_reads) << "%" << std::endl

       << std::endl

       << "  Paired Read Metrics" << std::endl
       << "  -------------------" << std::endl
       << "  Paired reads: " << percentage_string(m.paired_reads, m.total_reads) << std::endl
       << "  Paired and mapped reads: " << percentage_string(m.paired_and_mapped_reads, m.total_reads) << std::endl
       << "  FR reads: " << percentage_string(m.fr_reads, m.total_reads, 6) << std::endl
       << "  First of pair: " << percentage_string(m.first_reads, m.total_reads) << std::endl
       << "  Second of pair: " << percentage_string(m.second_reads, m.total_reads) << std::endl
       << "  Forward reads: " << percentage_string(m.forward_reads, m.total_reads) << std::endl
       << "  Reverse reads: " << percentage_string(m.reverse_reads, m.total_reads) << std::endl
       << "  Forward mate reads: " << percentage_string(m.forward_mate_reads, m.total_reads) << std::endl
       << "  Reverse mate reads: " << percentage_string(m.reverse_mate_reads, m.total_reads) << std::endl

       << std::endl

       << "  Unmapped Read Metrics" << std::endl
       << "  ---------------------" << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  Unmapped reads: " <<percentage_string(m.unmapped_reads, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  Unmapped mate reads: " <<percentage_string(m.unmapped_mate_reads, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  Reads not passing quality controls: " << percentage_string(m.qcfailed_reads, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  Unpaired reads: " << percentage_string(m.unpaired_reads, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  Reads with zero mapping quality: " << percentage_string(m.reads_mapped_with_zero_quality, m.total_reads) << std::endl

       << std::endl

       << "  Aberrant Mapping Metrics" << std::endl
       << "  ------------------------" << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  RF reads: " << percentage_string(m.rf_reads, m.total_reads, 6) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  FF reads: " << percentage_string(m.ff_reads, m.total_reads, 6) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  RR reads: " << percentage_string(m.rr_reads, m.total_reads, 6) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  Reads that paired and mapped but..." << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "    on different chromosomes: " << percentage_string(m.reads_with_mate_mapped_to_different_reference, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "    probably too far from their mates: " << percentage_string(m.reads_with_mate_too_distant, m.total_reads) << " (longest proper fragment seems to be " << m.maximum_proper_pair_fragment_size << ")" << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "    just not properly: " << percentage_string(m.reads_mapped_and_paired_but_improperly, m.total_reads) << std::endl

       << std::endl

       << "  Autosomal/Mitochondrial Metrics" << std::endl
       << "  -------------------------------" << std::endl
       << "  Total autosomal reads: " << percentage_string(m.total_autosomal_reads, m.total_reads, 3,  " (", "% of all reads)") << std::endl
       << "  Total mitochondrial reads: " << percentage_string(m.total_mitochondrial_reads, m.total_reads, 3, " (", "% of all reads)") << std::endl
       << "  Duplicate autosomal reads: " << percentage_string(m.duplicate_autosomal_reads, m.total_autosomal_reads, 3, " (", "% of all autosomal reads)") << std::endl
       << "  Duplicate mitochondrial reads: " << percentage_string(m.duplicate_mitochondrial_reads, m.total_mitochondrial_reads, 3, " (", "% of all mitochondrial reads)") << std::endl << std::endl

       << std::endl

       << "  Mapping Quality" << std::endl
       << "  ---------------" << std::endl
       << "  Mean MAPQ: " << std::fixed << m.mean_mapq() << std::endl
       << "  Median MAPQ: " << std::fixed << m.median_mapq() << std::endl
       << "  Reads with MAPQ >=..." << std::endl;

    for (int threshold = 5; threshold <= 30; threshold += 5) {
        unsigned long long int count = 0;
        for (auto it : m.mapq_counts) {
            if (it.first >= threshold) {
                count += it.second;
            }
        }
        os << std::setfill(' ') << std::setw(20) << std::right << threshold << ": " << percentage_string(count, m.total_reads) << std::endl;
    }

    if (m.peaks_requested) {
        os << std::endl << "  Peak Metrics" << std::endl
           << "  ------------" << std::endl
           << "  Peak count: " << m.peaks.size() << std::endl <<std::endl

           << "  High quality autosomal aligments that overlapped peaks: "  << percentage_string(m.hqaa_in_peaks, m.hqaa, 3, " (", "% of all high quality autosomal alignments)") << std::endl
           << "  Number of high quality autosomal aligments overlapping the top 10,000 peaks: " << std::endl
           << std::setfill(' ') << std::setw(20) << std::right << "Top peak: " << std::fixed << percentage_string(m.top_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal aligments)") << std::endl
           << std::setfill(' ') << std::setw(20) << std::right << "Top 10 peaks: " << std::fixed << percentage_string(m.top_10_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal aligments)") << std::endl
           << std::setfill(' ') << std::setw(20) << std::right << "Top 100 peaks: "<< std::fixed << percentage_string(m.top_100_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal aligments)") << std::endl
           << std::setfill(' ') << std::setw(20) << std::right << "Top 1000 peaks: " << std::fixed << percentage_string(m.top_1000_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal aligments)") << std::endl
           << std::setfill(' ') << std::setw(20) << std::right << "Top 10,000 peaks: " << std::fixed << percentage_string(m.top_10000_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal aligments)") << std::endl;
    }

    if (!(m.unclassified_reads == 0 && (total_problems + m.properly_paired_and_mapped_reads == m.total_reads))) {
        os << "  Some reads were not classified: " << percentage_string(m.total_reads - m.unclassified_reads - m.properly_paired_and_mapped_reads - total_problems, m.total_reads) << std::endl
           << "  We'd like to know what we're missing. If it would be possible for you to share the unclassified reads, please file an issue at: " << std::endl << std::endl
           << "      https://github.com/ParkerLab/ataqc/issues" << std::endl;
    }

    os << std::endl << std::endl;
    return os;
}


void Library::to_json(std::ostream& os, int indent, bool standalone) {
    std::string in(indent, ' ');
    os << "{" << std::endl
       << in << "  \"library\": \"" << qq(library) << "\"," << std::endl
       << in << "  \"sample\": \"" << qq(sample) << "\"," << std::endl
       << in << "  \"description\": \"" << qq(description) << "\"," << std::endl
       << in << "  \"sequencing_center\": \"" << qq(center) << "\"," << std::endl
       << in << "  \"sequencing_date\": \"" << qq(date) << "\"," << std::endl
       << in << "  \"sequencing_platform\": \"" << qq(platform) << "\"," << std::endl
       << in << "  \"platform_model\": \"" << qq(platform_model) << "\"," << std::endl
       << in << "  \"platform_unit\": \"" << qq(platform_unit) << "\"," << std::endl
       << in << "  \"flow_order\": \"" << qq(flow_order) << "\"," << std::endl
       << in << "  \"key_sequence\": \"" << qq(key_sequence) << "\"," << std::endl
       << in << "  \"predicted_median_insert_size\": \"" << qq(predicted_median_insert_size) << "\"," << std::endl
       << in << "  \"programs\": \"" << qq(programs) << "\"" << std::endl
       << in << "}" << (standalone ? "" : ",") << std::endl;
}


void Metrics::to_json(std::ostream& os, int indent, bool standalone) {
    std::string in(indent, ' ');
    os
        << in << "{" << std::endl
        << in << "  \"ataqc_version\": \"" << version_string() << "\"," << std::endl
        << in << "  \"timestamp\": \"" << iso8601_timestamp() << "\"," << std::endl
        << in << "  \"metrics\": {" << std::endl
        << in << "    \"name\": \"" << qq(name) << "\"," << std::endl
        << in << "    \"organism\": \"" << qq(collector->organism) << "\"," << std::endl
        << in << "    \"description\": \"" << qq(collector->description) << "\"," << std::endl
        << in << "    \"url\": \"" << qq(collector->url) << "\"," << std::endl

        << in << "    \"library\": ";

    library.to_json(os, indent + 4);

    os
        << in << "    \"total_reads\": " << total_reads << "," << std::endl
        << in << "    \"forward_reads\": " << forward_reads << "," << std::endl
        << in << "    \"reverse_reads\": " << reverse_reads << "," << std::endl
        << in << "    \"secondary_reads\": " << secondary_reads << "," << std::endl
        << in << "    \"supplementary_reads\": " << supplementary_reads << "," << std::endl
        << in << "    \"duplicate_reads\": " << duplicate_reads << "," << std::endl

        << in << "    \"paired_reads\": " << paired_reads << "," << std::endl
        << in << "    \"paired_and_mapped_reads\": " << paired_and_mapped_reads << "," << std::endl
        << in << "    \"properly_paired_and_mapped_reads\": " << properly_paired_and_mapped_reads << "," << std::endl
        << in << "    \"first_reads\": " << first_reads << "," << std::endl
        << in << "    \"second_reads\": " << second_reads << "," << std::endl
        << in << "    \"forward_mate_reads\": " << forward_mate_reads << "," << std::endl
        << in << "    \"reverse_mate_reads\": " << reverse_mate_reads << "," << std::endl
        << in << "    \"fr_reads\": " << fr_reads << "," << std::endl

        << in << "    \"unmapped_reads\": " << unmapped_reads << "," << std::endl
        << in << "    \"unmapped_mate_reads\": " << unmapped_mate_reads << "," << std::endl
        << in << "    \"qcfailed_reads\": " << qcfailed_reads << "," << std::endl
        << in << "    \"unpaired_reads\": " << unpaired_reads << "," << std::endl
        << in << "    \"ff_reads\": " << ff_reads << "," << std::endl
        << in << "    \"rf_reads\": " << rf_reads << "," << std::endl
        << in << "    \"rr_reads\": " << rr_reads << "," << std::endl
        << in << "    \"reads_with_mate_mapped_to_different_reference\": " << reads_with_mate_mapped_to_different_reference << "," << std::endl
        << in << "    \"reads_mapped_with_zero_quality\": " << reads_mapped_with_zero_quality << "," << std::endl
        << in << "    \"reads_mapped_and_paired_but_improperly\": " << reads_mapped_and_paired_but_improperly << "," << std::endl
        << in << "    \"unclassified_reads\": " << unclassified_reads << "," << std::endl
        << in << "    \"maximum_proper_pair_fragment_size\": " << maximum_proper_pair_fragment_size << "," << std::endl
        << in << "    \"reads_with_mate_too_distant\": " << reads_with_mate_too_distant << "," << std::endl
        << in << "    \"total_autosomal_reads\": " << total_autosomal_reads << "," << std::endl
        << in << "    \"total_mitochondrial_reads\": " << total_mitochondrial_reads << "," << std::endl
        << in << "    \"duplicate_autosomal_reads\": " << duplicate_autosomal_reads << "," << std::endl
        << in << "    \"duplicate_mitochondrial_reads\": " << duplicate_mitochondrial_reads << "," << std::endl
        << in << "    \"hqaa\": " << hqaa << "," << std::endl
        << in << "    \"hqaa_short_count\": " << hqaa_short_count << "," << std::endl
        << in << "    \"hqaa_mononucleosomal_count\": " << hqaa_mononucleosomal_count << "," << std::endl
        << in << "    \"short_mononucleosomal_ratio\": " << fraction(hqaa_short_count, hqaa_mononucleosomal_count) << "," << std::endl
        << in << "    \"fragment_length_counts_fields\": [\"fragment_length\", \"read_count\", \"fraction_of_all_reads\"]," << std::endl
        << in << "    \"fragment_length_counts\": [" << std::endl;

    int max_fragment_length = std::max(1000, fragment_length_counts.empty() ? 0 : fragment_length_counts.rbegin()->first);

    for (int fragment_length = 0; fragment_length < max_fragment_length; fragment_length++) {
        int count = fragment_length_counts[fragment_length];
        os << in << "      [" << fragment_length << ", " << count << ", " << fraction(count, total_reads) << "]";
        if (fragment_length < max_fragment_length - 1) {
            os << ",";
        }
        os << std::endl;
    }

    os
        << in << "    ]," << std::endl
        << in << "    \"hqaa_fragment_length_counts_fields\": [\"fragment_length\", \"read_count\", \"fraction_of_hqaa\"]," << std::endl
        << in << "    \"hqaa_fragment_length_counts\": [" << std::endl;


    max_fragment_length = std::max(1000, hqaa_fragment_length_counts.empty() ? 0 : hqaa_fragment_length_counts.rbegin()->first);

    for (int fragment_length = 0; fragment_length < max_fragment_length; fragment_length++) {
        int count = hqaa_fragment_length_counts[fragment_length];
        os << in << "      [" << fragment_length << ", " << count << ", " << fraction(count, hqaa) << "]";
        if (fragment_length < max_fragment_length - 1) {
            os << ",";
        }
        os << std::endl;
    }

    os
        << in << "    ]," << std::endl
        << in << "    \"mapq_counts_fields\": [\"mapq\", \"read_count\"]," << std::endl
        << in << "    \"mapq_counts\": [" << std::endl;

    auto last_mapq = --mapq_counts.end();
    for (auto it = mapq_counts.begin(); it != mapq_counts.end(); ++it) {
        os << in << "      [" << it->first << ", " << it->second << "]";
        if (it != last_mapq) {
            os << ",";
        }
        os << std::endl;
    }

    os
        << in << "    ]," << std::endl
        << in << "    \"mean_mapq\": " << mean_mapq() << "," << std::endl
        << in << "    \"median_mapq\": " << median_mapq() << "," << std::endl
        << in << "    \"peaks_fields\": [\"name\", \"overlapping_hqaa\", \"territory\"]," << std::endl
        << in << "    \"peaks\": [" << std::endl;

    std::vector<Peak> peak_list = peaks.list_peaks();;
    unsigned long long int hqaa_overlapping_peaks = 0;

    auto last_peak = --peak_list.end();
    for (auto peak = peak_list.begin(); peak != peak_list.end(); ++peak) {
        hqaa_overlapping_peaks += peak->overlapping_hqaa;
        os << in << "      [\"" << peak->name << "\", " << peak->overlapping_hqaa << ", " << peak->size() << "]";
        if (peak != last_peak) {
            os << ",";
        }
        os << std::endl;
    }

    os
        << in << "    ]," << std::endl
        << in << "    \"total_peaks\": " << peak_list.size() << "," << std::endl
        << in << "    \"total_peak_territory\": " << total_peak_territory << "," << std::endl
        << in << "    \"hqaa_overlapping_peaks_percent\": " << percentage(hqaa_overlapping_peaks, hqaa) << std::endl
        << in << "  }" << std::endl
        << in << "}" << (standalone ? "" : ",") << std::endl;
}

//
// Produce text version of all of a collector's Metrics
//
std::ostream& operator<<(std::ostream& os, const MetricsCollector& collector) {
    for (auto it : collector.metrics) {
        os << *(it.second);
    }

    return os;
}

void MetricsCollector::to_json(std::ostream& os) {
    os << "[" << std::endl;
    auto last = --metrics.end();
    for (auto it = metrics.cbegin(); it != metrics.cend(); ++it) {
        if (it->second->total_reads > 0) {
            it->second->to_json(os, 2, (it == last));
        }
    }
    os << "]" << std::endl;
}
