//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#include <chrono>
#include <cmath>
#include <cstdarg>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <unordered_map>

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
                                   const std::string& tss_filename,
                                   const int tss_extension,
                                   bool verbose,
                                   const int thread_limit,
                                   bool ignore_read_groups,
                                   bool log_problematic_reads,
                                   const std::vector<std::string>& excluded_region_filenames) :
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
    tss_filename(tss_filename),
    tss_extension(tss_extension),
    verbose(verbose),
    thread_limit(thread_limit),
    ignore_read_groups(ignore_read_groups),
    log_problematic_reads(log_problematic_reads),
    excluded_region_filenames(excluded_region_filenames)
{

    make_default_autosomal_references();

    if (!autosomal_reference_filename.empty()) {
        load_autosomal_references();
    }

    if (!excluded_region_filenames.empty()) {
        load_excluded_regions();
    }
}


std::string MetricsCollector::configuration_string() const {
    std::stringstream cs;
    cs << "ataqv " << version_string() << std::endl << std::endl

       << "Operating parameters" << std::endl
       << "====================" << std::endl;

    cs
        << "Thread limit: " << thread_limit << std::endl
        << "Ignoring read groups: " << (ignore_read_groups ? "yes" : "no") << std::endl;

    if (!tss_filename.empty()) {
        cs << "TSS extension: " << tss_extension << std::endl;
    }

    cs << std::endl
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


std::string MetricsCollector::autosomal_reference_string(std::string separator) const {
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
            ss << separator;
        }
    }
    return ss.str();
}


//
// Set up default autosomal references for common organisms
//
void MetricsCollector::make_default_autosomal_references() {
    // Normal, predictable, sensible chromosome names :^)
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

    // Fly
    std::vector<std::string> fly_chromosomes = {"2L", "2R", "3L", "3R", "4"};
    for (auto c : fly_chromosomes) {
        autosomal_references["fly"][c] = 1;
        autosomal_references["fly"]["chr" + c] = 1;
    }

    // Worm
    for (int i = 1; i < 6; i++) {
        autosomal_references["worm"][integer_to_roman(i)] = 1;
        autosomal_references["worm"]["chr" + integer_to_roman(i)] = 1;
    }

    // Yeast
    for (int i = 1; i < 17; i++) {
        autosomal_references["yeast"][integer_to_roman(i)] = 1;
        autosomal_references["yeast"]["chr" + integer_to_roman(i)] = 1;
    }
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
            std::cout << "Autosomal references for " << organism << ":\n\t" << autosomal_reference_string("\n\t") << std::endl;
        }
    }
}


bool MetricsCollector::is_autosomal(const std::string& reference_name) {
    static std::unordered_map<std::string, bool> refcache;

    if (refcache.count(reference_name) == 0) {
        refcache[reference_name] = autosomal_references[organism].find(reference_name) != autosomal_references[organism].end();
    }

    return refcache.at(reference_name);
}


bool MetricsCollector::is_mitochondrial(const std::string& reference_name) {
    return mitochondrial_reference_name.compare(reference_name) == 0;
}

bool MetricsCollector::is_hqaa(const bam_hdr_t* header, const bam1_t* record) {
    bool hqaa = false;

    if (
        !IS_UNMAPPED(record) &&
        !IS_MATE_UNMAPPED(record) &&
        !IS_DUP(record) &&
        IS_PAIRED_AND_MAPPED(record) &&
        IS_PROPERLYPAIRED(record) &&
        IS_PRIMARY(record) &&
        (record->core.qual >= 30) &&
        (record->core.tid >= 0)) {

        std::string reference_name(header->target_name[record->core.tid]);
        if (is_autosomal(reference_name)) {
            hqaa = true;
        }
    }

    return hqaa;
}


//
// Load transcription start sites for the organism
//
void MetricsCollector::load_tss() {
    if (verbose) {
        std::cout << "Loading TSS file '" << tss_filename << "'." << std::endl;
    }

    boost::shared_ptr<boost::iostreams::filtering_istream> tss_istream;
    try {
        tss_istream = mistream(tss_filename);
    } catch (FileException& e) {
        throw FileException("Could not open the supplied TSS file \"" + tss_filename + "\": " + e.what());
    }

    boost::chrono::high_resolution_clock::time_point start = boost::chrono::high_resolution_clock::now();
    boost::chrono::duration<double> duration;
    Feature tss;

    while (*tss_istream >> tss) {
        bool excluded = false;
        for (auto er : excluded_regions) {
            if (tss.overlaps(er)) {
                if (verbose) {
                    std::cout << "Excluding TSS [" << tss << "] which overlaps excluded region [" << er << "]" << std::endl;
                }
                excluded = true;
                break;
            }
        }
        if (!excluded && is_autosomal(tss.reference)) {
            tss_tree.add(tss);
        }
    }

    if (verbose) {
        duration = boost::chrono::high_resolution_clock::now() - start;
        tss_tree.print_reference_feature_counts();
        std::cout << "Loaded " << tss_tree.size() << " TSS in " << duration << "." << " (" << (tss_tree.size() / duration.count()) << " TSS/second)." << std::endl << std::endl;
    }
}


///
/// Measure all the reads in a BAM file
///
void MetricsCollector::load_alignments() {

    samFile *alignment_file = nullptr;
    bam_hdr_t *alignment_file_header = nullptr;
    hts_idx_t *alignment_file_index = nullptr;
    bam1_t *record = bam_init1();

    if (alignment_filename.empty()) {
        throw FileException("Alignment file has not been specified.");
    }

    if ((alignment_file = sam_open(alignment_filename.c_str(), "r")) == nullptr) {
        throw FileException("Could not open alignment file \"" + alignment_filename + "\".");
    }

    if (!tss_filename.empty()) {
        if ((alignment_file_index = sam_index_load(alignment_file, alignment_filename.c_str())) == nullptr) {
            throw FileException("Before TSS enrichment can be calculated, you must create an index file\nfor alignment file \"" + alignment_filename + "\" with \"samtools index " + alignment_filename + "\".");
        }

        load_tss();
    }

    if (verbose) {
        std::cout << "Collecting metrics from " << alignment_filename << "." << std::endl << std::endl;
    }

    alignment_file_header = sam_hdr_read(alignment_file);
    if (alignment_file_header == NULL) {
        throw FileException("Could not read a valid header from alignment file \"" + alignment_filename +  "\".");
    }

    std::string default_metrics_id = name.empty() ? basename(alignment_filename) : name;

    try {
        sam_header header = parse_sam_header(alignment_file_header->text);
        if (!ignore_read_groups && header.count("RG") > 0) {
            for (auto read_group : header["RG"]) {
                std::string read_group_id = read_group["ID"];
                metrics[read_group_id] = new Metrics(this, read_group_id);

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

                metrics[read_group_id]->library = library;
            }
        } else {
            metrics[default_metrics_id] = new Metrics(this, default_metrics_id);

            Library library;
            library.library = default_metrics_id;
            library.sample = default_metrics_id;
            library.description = library_description;
            metrics[default_metrics_id]->library = library;
        }

        boost::chrono::high_resolution_clock::time_point start = boost::chrono::high_resolution_clock::now();
        boost::chrono::duration<double> duration;
        double rate = 0.0;

        unsigned long long int total_reads = 0;

        while (sam_read1(alignment_file, alignment_file_header, record) >= 0) {
            Metrics* m;

            uint8_t* rgaux = bam_aux_get(record, "RG");
            if (!ignore_read_groups && rgaux) {
                std::string read_group_id = bam_aux2Z(rgaux);

                // It can happen that records have RG tags that don't
                // exist in the file header. If we're not ignoring
                // read groups altogether, create new Metrics
                // instances for these rapscallions.
                try {
                    m = metrics.at(read_group_id);
                } catch (std::out_of_range&) {
                    std::cout << "Adding metrics for read group missing from file header: " << read_group_id << std::endl;
                    metrics[read_group_id] = new Metrics(this, read_group_id);
                    m = metrics[read_group_id];
                }
            } else {
                m = metrics[default_metrics_id];
            }

            m->add_alignment(alignment_file_header, record);

            total_reads++;

            if (verbose && total_reads % 100000 == 0) {
                duration = boost::chrono::high_resolution_clock::now() - start;
                rate = (total_reads / duration.count());
                std::cout << "Analyzed " << total_reads << " reads in " << duration << " (" << rate << " reads/second)." << std::endl;
            }
        }

        calculate_tss_coverage();

        for (auto& it : metrics) {
            Metrics* m = it.second;
            if (m->total_reads == 0) {
                std::cout << "Dropping metrics " << m->name << " which has no reads." << std::endl;
                metrics.erase(it.first);
            } else {
                m->make_aggregate_diagnoses();
                m->peaks.determine_top_peaks();
                m->calculate_tss_metrics();
            }
        }

        bam_destroy1(record);
        bam_hdr_destroy(alignment_file_header);
        if (alignment_file) {
            hts_close(alignment_file);
        }

        if (verbose) {
            duration = boost::chrono::high_resolution_clock::now() - start;
            rate = (total_reads / duration.count());
            std::cout << "Analyzed " << total_reads << " reads in " << duration << " (" << rate << " reads/second)." << std::endl << std::endl;
        }
    } catch (FileException& e) {
        bam_destroy1(record);
        bam_hdr_destroy(alignment_file_header);
        hts_idx_destroy(alignment_file_index);
        if (alignment_file) {
            hts_close(alignment_file);
        }
        throw;
    }
}


Metrics::Metrics(MetricsCollector* collector, const std::string& name): collector(collector), name(name), peaks(), log_problematic_reads(collector->log_problematic_reads) {

    if (log_problematic_reads) {
        try {
            problematic_read_filename = make_metrics_filename(".problems");

            if (collector->verbose) {
                std::cout << "Logging problematic reads to " << problematic_read_filename << "." << std::endl << std::endl;
            }

            problematic_read_stream = mostream(problematic_read_filename);
        } catch (FileException& e) {
            throw FileException("Could not open problematic read file " + problematic_read_filename + ": " + e.what());
        }
    }

    if (!collector->peak_filename.empty()) {
        peaks_requested = true;
        load_peaks();
    }

    if (!collector->tss_filename.empty()) {
        tss_requested = true;
        for (int i = 1; i <= 1 + 2 * collector->tss_extension; i++) {
            tss_coverage[i] = 0;
        }
    }
}


std::string Metrics::make_metrics_filename(const std::string& suffix) {
    return name + suffix;
}


void Metrics::log_problematic_read(const std::string& problem, const std::string& record) {
    if (!log_problematic_reads) {
        return;
    }

    *problematic_read_stream << problem;

    if (!record.empty()) {
        *problematic_read_stream << '\t' << record;
    }

    *problematic_read_stream << '\n';
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
    cs << "Read Group\n==========\nID: " << name << std::endl << library << std::endl;
    return cs.str();
}


bool Metrics::is_hqaa(const bam_hdr_t* header, const bam1_t* record) {
    bool hqaa = false;

    if (
        !IS_UNMAPPED(record) &&
        !IS_MATE_UNMAPPED(record) &&
        !IS_DUP(record) &&
        IS_PAIRED_AND_MAPPED(record) &&
        IS_PROPERLYPAIRED(record) &&
        IS_PRIMARY(record) &&
        (record->core.qual >= 30) &&
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
    for (auto it : mapq_counts) {
        unsigned long long int next_mapq_index = mapq_index + it.second;
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
    unsigned long long int fragment_length = llabs(record->core.isize);

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

                if (is_mitochondrial(reference_name)) {
                    total_mitochondrial_reads++;
                    if (IS_DUP(record)) {
                        duplicate_mitochondrial_reads++;
                    }
                } else {
                    if (is_autosomal(reference_name)) {
                        total_autosomal_reads++;

                        if (!peaks.empty()) {
                            peaks.record_alignment(Feature(header, record), is_hqaa(header, record), IS_DUP(record));
                        }

                        if (IS_DUP(record)) {
                            duplicate_autosomal_reads++;
                        } else {
                            // nonduplicate, properly paired and uniquely mapped
                            // autosomal reads will be the basis of our fragment
                            // size and peak statistics
                            if (is_hqaa(header, record)) {
                                hqaa++;
                                chromosome_counts[reference_name]++;

                                // record proper pairs' fragment lengths
                                fragment_length_counts[fragment_length]++;

                                if (50 <= fragment_length && fragment_length <= 100) {
                                    hqaa_short_count++;
                                }

                                if (150 <= fragment_length && fragment_length <= 200) {
                                    hqaa_mononucleosomal_count++;
                                }
                            }
                        }
                    }
                }
            }

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
            if (IS_PRIMARY(record) &&  maximum_proper_pair_fragment_size < fragment_length) {
                maximum_proper_pair_fragment_size = fragment_length;
                if (collector->verbose) {
                    std::cerr << "New maximum proper pair fragment length: " << maximum_proper_pair_fragment_size << " from [" << record_to_string(header, record) << "]" << std::endl;
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

    if (peak_filename == "auto") {
        peak_filename = make_metrics_filename(".peaks");
    }

    if (collector->verbose) {
        std::cout << "Loading peaks for read group " << name << " from " << peak_filename << "." << std::endl;
    }

    boost::shared_ptr<boost::iostreams::filtering_istream> peak_istream;
    try {
        peak_istream = mistream(peak_filename);
    } catch (FileException& e) {
        throw FileException("Could not open the supplied peak file \"" + peak_filename + "\": " + e.what());
    }

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
        }
    }

    if (collector->verbose) {
        duration = boost::chrono::high_resolution_clock::now() - start;
        peaks.print_reference_peak_counts();
        std::cout << "Loaded " << peaks.size() << " peaks in " << duration << "." << " (" << (peaks.size() / duration.count()) << " peaks/second)." << std::endl << std::endl;
    }
}


std::map<std::string,std::map<int, unsigned long long int>> MetricsCollector::get_tss_coverage_for_reference(const std::string &reference, const int extension) {
    std::map<std::string,std::map<int, unsigned long long int>> ref_tss_cov = {};

    ReferenceFeatureCollection *tss_collection = tss_tree.get_reference_feature_collection(reference);

    if (!tss_collection->features.empty()) {
        samFile *alignment_file = nullptr;
        bam_hdr_t *alignment_file_header = nullptr;
        hts_idx_t *alignment_file_index = nullptr;
        bam1_t *record = bam_init1();

        try {
            if (alignment_filename.empty()) {
                throw FileException("Alignment file has not been specified.");
            }

            if ((alignment_file = sam_open(alignment_filename.c_str(), "r")) == nullptr) {
                throw FileException("Could not open alignment file \"" + alignment_filename + "\".");
            }

            if ((alignment_file_index = sam_index_load(alignment_file, alignment_filename.c_str())) == nullptr) {
                throw FileException("Could not open index for alignment file \"" + alignment_filename + "\".");
            }

            alignment_file_header = sam_hdr_read(alignment_file);
            if (alignment_file_header == NULL) {
                throw FileException("Could not read a valid header from alignment file \"" + alignment_filename +  "\".");
            }

            for (auto tss : tss_collection->features) {
                std::map<std::string, bool> fragments_seen = {};

                Feature tss_region(tss);
                tss_region.start = std::max((unsigned long long int)0, tss_region.start - extension);
                tss_region.end += extension;

                std::stringstream query;

                // The HTSlib iterator starts at the first record starting after the beginning of the region, so
                // we ask for records further before and after the TSS region, then filter them ourselves
                query << tss_region.reference << ":" << std::max(tss_region.start - extension * 2, 0ULL) << "-" << (tss_region.end + extension * 2);

                hts_itr_t *alignment_iterator;
                if ((alignment_iterator = sam_itr_querys(alignment_file_index, alignment_file_header, query.str().c_str())) == nullptr) {
                    std::cerr <<  "Could not find TSS region " << query.str() << " in your BAM file. Check that your TSS file's chromosome naming scheme matches your reference." << std::endl;
                    continue;
                }

                while (sam_itr_next(alignment_file, alignment_iterator, record) >= 0) {
                    if (is_hqaa(alignment_file_header, record)) {
                        std::string qname = get_qname(record);
                        if (fragments_seen.count(qname) == 0) {
                            Feature fragment;
                            fragment.reference = reference;
                            fragment.start = std::min(record->core.pos, record->core.mpos);
                            fragment.end = fragment.start + abs(record->core.isize);
                            fragments_seen[qname] = true;

                            if (fragment.overlaps(tss_region)) {
                                std::string metrics_id = name.empty() ? basename(alignment_filename) : name;
                                if (!ignore_read_groups) {
                                    uint8_t* rgaux = bam_aux_get(record, "RG");
                                    metrics_id = bam_aux2Z(rgaux);
                                }
                                for (unsigned long long int pos = tss_region.start; pos <= tss_region.end; pos++) {
                                    if (pos >= fragment.start && pos <= fragment.end) {
                                        int base = tss.is_reverse() ? (tss_region.end - pos) : (pos - tss_region.start);
                                        ref_tss_cov[metrics_id][base]++;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            bam_destroy1(record);
            bam_hdr_destroy(alignment_file_header);
            hts_idx_destroy(alignment_file_index);
            if (alignment_file) {
                hts_close(alignment_file);
            }
        } catch (FileException& e) {
            bam_destroy1(record);
            bam_hdr_destroy(alignment_file_header);
            hts_idx_destroy(alignment_file_index);
            if (alignment_file) {
                hts_close(alignment_file);
            }
            throw;
        }
    }

    return ref_tss_cov;
}

void MetricsCollector::calculate_tss_coverage() {

    if (tss_filename == "") {
        return;
    }

    if (verbose) {
        std::cout << "Calculating TSS coverage..." << std::endl;
    }

    boost::chrono::high_resolution_clock::time_point start = boost::chrono::high_resolution_clock::now();
    boost::chrono::duration<double> duration;

    double tss_count = (double) tss_tree.size();
    std::map<std::string,std::map<int, unsigned long long int>> tss_coverage = {};

    if (tss_count > 0) {
        for (auto it: metrics) {
            std::string metrics_id = it.first;
            for (int i = 1; i <= 1 + 2 * tss_extension; i++) {
                tss_coverage[metrics_id][i] = 0;
            }
        }

        std::vector<std::future<std::map<std::string,std::map<int, unsigned long long int>>>> results = {};
        int thread_count = 0;
        for (auto reference : tss_tree.get_references_by_feature_count()) {
            results.push_back(std::async(std::launch::async, &MetricsCollector::get_tss_coverage_for_reference, this, reference, tss_extension));
            thread_count++;
            if (verbose) {
                std::cout << "Added TSS coverage for " << reference << "; thread count=" << thread_count << std::endl;
            }
            while (thread_count == thread_limit) {
                for (auto result = results.begin(); result != results.end(); result++) {
                    if (!(*result).valid()) {
                        results.erase(result);
                        thread_count--;
                        break;
                    } else {
                        std::future_status status = (*result).wait_for(std::chrono::milliseconds(100));
                        if (status == std::future_status::ready) {
                            auto ref_cov = (*result).get();
                            for (auto it: metrics) {
                                std::string metrics_id = it.first;
                                for (int i = 1; i <= 1 + 2 * tss_extension; i++) {
                                    tss_coverage[metrics_id][i] += ref_cov[metrics_id][i];
                                }
                            }
                        }
                    }
                }
            }
        }

        if (verbose) {
            std::cout << "All TSS jobs started. Waiting for last ones to complete." << std::endl;
        }

        while (!results.empty()) {
            for (auto result = results.begin(); result != results.end(); result++) {
                if (!(*result).valid()) {
                    results.erase(result);
                    break;
                }

                std::future_status status = (*result).wait_for(std::chrono::milliseconds(100));
                if (status == std::future_status::ready) {
                    auto ref_cov = (*result).get();
                    for (auto it: metrics) {
                        std::string metrics_id = it.first;
                        for (int i = 1; i <= 1 + 2 * tss_extension; i++) {
                            tss_coverage[metrics_id][i] += ref_cov[metrics_id][i];
                        }
                    }
                }
            }
        }

        for (auto it: metrics) {
            Metrics* m = it.second;
            m->tss_coverage = tss_coverage[it.first];
        }
    }

    duration = boost::chrono::high_resolution_clock::now() - start;
    if (verbose) {
        std::cout << "Calculated TSS coverage in " << duration << "." << std::endl;
    }
}

void Metrics::calculate_tss_metrics() {

    if (!tss_requested) {
        return;
    }

    if (collector->verbose) {
        std::cout << "Calculating TSS metrics..." << std::endl;
    }

    double tss_count = (double) collector->tss_tree.size();

    boost::chrono::high_resolution_clock::time_point start = boost::chrono::high_resolution_clock::now();
    boost::chrono::duration<double> duration;

    // calculate the average read depth in each 100bp flank
    double upstream_flank = 0.0;
    int index = 0;
    for (auto position = tss_coverage.begin(); index < 100; index++, position++) {
        upstream_flank += (position->second / tss_count);
    }
    upstream_flank /= 100.0;

    double downstream_flank = 0.0;
    index = 0;
    for (auto position = tss_coverage.rbegin(); index < 100; index++, position++) {
        downstream_flank += (position->second / tss_count);
    }
    downstream_flank /= 100.0;

    // average the two flanks
    double mean_flank = (upstream_flank + downstream_flank) / 2;

    // scale them to 1
    double scale = 1 / mean_flank;

    double mean_flank_scaled = (mean_flank * scale);

    // calculate mean enrichment around TSS
    for (auto pc : tss_coverage) {
        tss_coverage_scaled[pc.first] = ((pc.second / tss_count) * scale) / mean_flank_scaled;
    }

    // and finally, take the value at TSS as our canonical enrichment score
    tss_enrichment = tss_coverage_scaled[collector->tss_extension + 1];

    duration = boost::chrono::high_resolution_clock::now() - start;
    if (collector->verbose) {
        std::cout << "Calculated TSS metrics in " << duration << "." << std::endl;
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
       << "Metrics\n"
       << "-------\n\n"
       << "  Read Mapping Metrics" << std::endl
       << "  --------------------" << std::endl
       << "  Total reads: " << m.total_reads << std::endl
       << "  Total problems: " << total_problems << percentage_string(total_problems, m.total_reads) << std::endl
       << "  Properly paired and mapped reads: " << m.properly_paired_and_mapped_reads << percentage_string(m.properly_paired_and_mapped_reads, m.total_reads) << std::endl
       << "  Secondary reads: " << m.secondary_reads << percentage_string(m.secondary_reads, m.total_reads) << std::endl
       << "  Supplementary reads: " << m.supplementary_reads << percentage_string(m.supplementary_reads, m.total_reads) << std::endl
       << "  Duplicate reads: " << m.duplicate_reads << percentage_string(m.duplicate_reads, m.total_reads, 3, " (", "% of all reads)") << std::endl

       << std::endl

       << "  Quality Indicators" << std::endl
       << "  ------------------" << std::endl
       << "  Short to mononucleosomal ratio: " << std::setprecision(3) << std::fixed << fraction_string(m.hqaa_short_count, m.hqaa_mononucleosomal_count) << std::endl
       << "  High quality, nonduplicate, properly paired, uniquely mapped autosomal alignments: " << m.hqaa << std::endl
       << "    as a percentage of autosomal reads: " << std::setprecision(3) << std::fixed << percentage_string(m.hqaa, m.total_autosomal_reads, 3, "", "%") << std::endl
       << "    as a percentage of all reads: " << std::setprecision(3) << std::fixed << percentage_string(m.hqaa, m.total_reads, 3, "", "%") << std::endl;

    if (m.tss_requested) {
        os << "  TSS enrichment: " << m.tss_enrichment << std::endl;
    }

    os << std::endl
       << "  Paired Read Metrics" << std::endl
       << "  -------------------" << std::endl
       << "  Paired reads: " << m.paired_reads << percentage_string(m.paired_reads, m.total_reads) << std::endl
       << "  Paired and mapped reads: " << m.paired_and_mapped_reads << percentage_string(m.paired_and_mapped_reads, m.total_reads) << std::endl
       << "  FR reads: " << m.fr_reads << percentage_string(m.fr_reads, m.total_reads, 6) << std::endl
       << "  First of pair: " << m.first_reads << percentage_string(m.first_reads, m.total_reads) << std::endl
       << "  Second of pair: " << m.second_reads << percentage_string(m.second_reads, m.total_reads) << std::endl
       << "  Forward reads: " << m.forward_reads << percentage_string(m.forward_reads, m.total_reads) << std::endl
       << "  Reverse reads: " << m.reverse_reads << percentage_string(m.reverse_reads, m.total_reads) << std::endl
       << "  Forward mate reads: " << m.forward_mate_reads << percentage_string(m.forward_mate_reads, m.total_reads) << std::endl
       << "  Reverse mate reads: " << m.reverse_mate_reads << percentage_string(m.reverse_mate_reads, m.total_reads) << std::endl

       << std::endl

       << "  Unmapped Read Metrics" << std::endl
       << "  ---------------------" << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  Unmapped reads: " << m.unmapped_reads << percentage_string(m.unmapped_reads, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  Unmapped mate reads: " << m.unmapped_mate_reads << percentage_string(m.unmapped_mate_reads, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  Reads not passing quality controls: " << m.qcfailed_reads << percentage_string(m.qcfailed_reads, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  Unpaired reads: " << m.unpaired_reads << percentage_string(m.unpaired_reads, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  Reads with zero mapping quality: " << m.reads_mapped_with_zero_quality << percentage_string(m.reads_mapped_with_zero_quality, m.total_reads) << std::endl

       << std::endl

       << "  Aberrant Mapping Metrics" << std::endl
       << "  ------------------------" << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  RF reads: " << m.rf_reads << percentage_string(m.rf_reads, m.total_reads, 6) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  FF reads: " << m.ff_reads << percentage_string(m.ff_reads, m.total_reads, 6) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  RR reads: " << m.rr_reads << percentage_string(m.rr_reads, m.total_reads, 6) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "  Reads that paired and mapped but..." << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "    on different chromosomes: " << m.reads_with_mate_mapped_to_different_reference << percentage_string(m.reads_with_mate_mapped_to_different_reference, m.total_reads) << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "    probably too far from their mates: " << m.reads_with_mate_too_distant << percentage_string(m.reads_with_mate_too_distant, m.total_reads) << " (longest proper fragment seems to be " << m.maximum_proper_pair_fragment_size << ")" << std::endl
       << std::setfill(' ') << std::left << std::setw(40) << "    just not properly: " << m.reads_mapped_and_paired_but_improperly << percentage_string(m.reads_mapped_and_paired_but_improperly, m.total_reads) << std::endl

       << std::endl

       << "  Autosomal/Mitochondrial Metrics" << std::endl
       << "  -------------------------------" << std::endl
       << "  Total autosomal reads: " << m.total_autosomal_reads << percentage_string(m.total_autosomal_reads, m.total_reads, 3,  " (", "% of all reads)") << std::endl
       << "  Total mitochondrial reads: " << m.total_mitochondrial_reads << percentage_string(m.total_mitochondrial_reads, m.total_reads, 3, " (", "% of all reads)") << std::endl
       << "  Duplicate autosomal reads: " << m.duplicate_autosomal_reads << percentage_string(m.duplicate_autosomal_reads, m.total_autosomal_reads, 3, " (", "% of all autosomal reads)") << std::endl
       << "  Duplicate mitochondrial reads: " << m.duplicate_mitochondrial_reads << percentage_string(m.duplicate_mitochondrial_reads, m.total_mitochondrial_reads, 3, " (", "% of all mitochondrial reads)") << std::endl << std::endl

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
        os << std::setfill(' ') << std::setw(20) << std::right << threshold << ": " << count << percentage_string(count, m.total_reads) << std::endl;
    }

    if (m.peaks_requested) {
        os << std::endl << "  Peak Metrics" << std::endl
           << "  ------------" << std::endl
           << "  Peak count: " << m.peaks.size() << std::endl <<std::endl

           << "  High quality autosomal alignments that overlapped peaks: "  << m.peaks.hqaa_in_peaks << percentage_string(m.peaks.hqaa_in_peaks, m.hqaa, 3, " (", "% of all high quality autosomal alignments)") << std::endl
           << "  Number of high quality autosomal alignments overlapping the top 10,000 peaks: " << std::endl
           << std::setfill(' ') << std::setw(20) << std::right << "Top peak: " << std::fixed << m.peaks.top_peak_hqaa_read_count << percentage_string(m.peaks.top_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal alignments)") << std::endl
           << std::setfill(' ') << std::setw(20) << std::right << "Top 10 peaks: " << std::fixed << m.peaks.top_10_peak_hqaa_read_count << percentage_string(m.peaks.top_10_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal alignments)") << std::endl
           << std::setfill(' ') << std::setw(20) << std::right << "Top 100 peaks: "<< std::fixed << m.peaks.top_100_peak_hqaa_read_count << percentage_string(m.peaks.top_100_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal alignments)") << std::endl
           << std::setfill(' ') << std::setw(20) << std::right << "Top 1000 peaks: " << std::fixed << m.peaks.top_1000_peak_hqaa_read_count << percentage_string(m.peaks.top_1000_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal alignments)") << std::endl
           << std::setfill(' ') << std::setw(20) << std::right << "Top 10,000 peaks: " << std::fixed << m.peaks.top_10000_peak_hqaa_read_count << percentage_string(m.peaks.top_10000_peak_hqaa_read_count, m.hqaa, 3, " (", "% of all high quality autosomal alignments)") << std::endl;
    }

    unsigned long long int mysteries = m.total_reads - m.unclassified_reads - m.properly_paired_and_mapped_reads - total_problems;
    if (!(m.unclassified_reads == 0 && (total_problems + m.properly_paired_and_mapped_reads == m.total_reads))) {
        os << "  Some reads slipped through our taxonomy: " << mysteries << percentage_string(mysteries, m.total_reads) << std::endl
           << "  We'd like to know what we're missing. If it would be possible for you\nto share your data with us, please file an issue at: " << std::endl << std::endl
           << "      https://github.com/ParkerLab/ataqv/issues" << std::endl;
    }

    os << std::endl;
    return os;
}


nlohmann::json Library::to_json() {
    nlohmann::json result = {
        {"library", library},
        {"sample", sample},
        {"description", description},
        {"sequencingcenter", center},
        {"sequencingdate", date},
        {"sequencingplatform", platform},
        {"platformmodel", platform_model},
        {"platformunit", platform_unit},
        {"floworder", flow_order},
        {"keysequence", key_sequence},
        {"predicted_median_insert_size", predicted_median_insert_size},
        {"programs", programs}
    };
    return result;
}


nlohmann::json Metrics::to_json() {
    std::vector<std::string> fragment_length_counts_fields = {"fragment_length", "read_count", "fraction_of_all_reads"};
    nlohmann::json fragment_length_counts_json;
    int max_fragment_length = std::min(1000, std::max(1000, fragment_length_counts.empty() ? 0 : fragment_length_counts.rbegin()->first));

    for (int fragment_length = 0; fragment_length <= max_fragment_length; fragment_length++) {
        int count = fragment_length_counts[fragment_length];
        nlohmann::json flc;
        flc.push_back(fragment_length);
        flc.push_back(count);
        long double fraction_of_total_reads = total_reads == 0 ? std::nan("") : count / (long double) total_reads;
        flc.push_back(fraction_of_total_reads);

        fragment_length_counts_json.push_back(flc);
    }

    unsigned long long int max_autosome_counts = 0;
    unsigned long long int total_autosome_counts = 0;
    nlohmann::json chromosome_counts_json;

    for (auto it : chromosome_counts) {
        std::string chromosome = it.first;
        unsigned long long int reads_from_chromosome = it.second;
        nlohmann::json cc;
        cc.push_back(chromosome);
        cc.push_back(reads_from_chromosome);
        chromosome_counts_json.push_back(cc);

        if (is_autosomal(chromosome)) {
                total_autosome_counts += reads_from_chromosome;
                if (reads_from_chromosome > max_autosome_counts) {
                        max_autosome_counts = reads_from_chromosome;
                }
        }
    }

    long double max_fraction_reads_from_single_autosome = total_autosome_counts == 0 ? std::nan("") : max_autosome_counts / (long double) total_autosome_counts;

    std::vector<std::string> mapq_counts_fields = {"mapq", "read_count"};

    nlohmann::json mapq_counts_json;
    for (auto it : mapq_counts) {
        nlohmann::json mc;
        mc.push_back(it.first);
        mc.push_back(it.second);
        mapq_counts_json.push_back(mc);
    }

    std::vector<std::string> peaks_fields = {
        "name",
        "overlapping_hqaa",
        "territory"
    };

    std::vector<nlohmann::json> peak_list;

    std::set<unsigned long long int> percentile_indices;
    std::map<std::string, std::vector<long double>> peak_percentiles = {
        {"cumulative_fraction_of_hqaa", {}},
        {"cumulative_fraction_of_territory", {}}
    };

    auto default_peak_list = peaks.list_peaks();
    unsigned long long int peak_count = default_peak_list.size();
    unsigned long long int hqaa_overlapping_peaks = 0;


    for (int percentile = 1; percentile < 101; percentile++) {
        percentile_indices.insert(peak_count * (percentile / 100.0));
    }

    for (auto peak: default_peak_list) {
        hqaa_overlapping_peaks += peak.overlapping_hqaa;

        nlohmann::json jp;
        jp.push_back(peak.name);
        jp.push_back(peak.overlapping_hqaa);
        jp.push_back(peak.size());

        peak_list.push_back(jp);
    }

    unsigned long long int count = 0;
    long double cumulative_fraction_of_hqaa = 0.0;
    for (auto peak: peaks.list_peaks_by_overlapping_hqaa_descending()) {
        count++;
        cumulative_fraction_of_hqaa += hqaa == 0 ? std::nan("") : (peak.overlapping_hqaa / (long double)hqaa);

        if (percentile_indices.count(count) == 1) {
            peak_percentiles["cumulative_fraction_of_hqaa"].push_back(cumulative_fraction_of_hqaa);
        }
    }

    count = 0;
    long double cumulative_fraction_of_territory = 0.0;
    for (auto peak: peaks.list_peaks_by_size_descending()) {
        count++;
        cumulative_fraction_of_territory += (peak.size() / (long double)peaks.total_peak_territory);

        if (percentile_indices.count(count) == 1) {
            peak_percentiles["cumulative_fraction_of_territory"].push_back(cumulative_fraction_of_territory);
        }
    }

    long double short_mononucleosomal_ratio = fraction(hqaa_short_count, hqaa_mononucleosomal_count);

    nlohmann::json tss_coverage_vec;
    for (auto pc : tss_coverage_scaled) {
        nlohmann::json pair;
        pair.push_back(pc.first);
        pair.push_back(pc.second);
        tss_coverage_vec.push_back(pair);
    }

    nlohmann::json result = {
        {"ataqv_version", version_string()},
        {"timestamp", iso8601_timestamp()},
        {"metrics",
         {
             {"name", name},
             {"organism", collector->organism},
             {"description", collector->description},
             {"url", collector->url},
             {"library", library.to_json()},
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
             {"hqaa_tf_count", hqaa_short_count},
             {"hqaa_mononucleosomal_count", hqaa_mononucleosomal_count},
             {"short_mononucleosomal_ratio", short_mononucleosomal_ratio},
             {"hqaa_in_peaks", peaks.hqaa_in_peaks},
             {"duplicates_in_peaks", peaks.duplicates_in_peaks},
             {"duplicates_not_in_peaks", peaks.duplicates_not_in_peaks},
             {"ppm_in_peaks", peaks.ppm_in_peaks},
             {"ppm_not_in_peaks", peaks.ppm_not_in_peaks},
             {"duplicate_fraction_in_peaks", fraction(peaks.duplicates_in_peaks, peaks.ppm_in_peaks)},
             {"duplicate_fraction_not_in_peaks", fraction(peaks.duplicates_not_in_peaks, peaks.ppm_not_in_peaks)},
             {"peak_duplicate_ratio", fraction(fraction(peaks.duplicates_not_in_peaks, peaks.ppm_not_in_peaks), fraction(peaks.duplicates_in_peaks, peaks.ppm_in_peaks))},
             {"fragment_length_counts_fields", fragment_length_counts_fields},
             {"fragment_length_counts", fragment_length_counts_json},
             {"fragment_length_distance", nullptr},
             {"mapq_counts_fields", mapq_counts_fields},
             {"mapq_counts", mapq_counts_json},
             {"mean_mapq", mean_mapq()},
             {"median_mapq", median_mapq()},
             {"peaks_fields", peaks_fields},
             {"peaks", peak_list},
             {"peak_percentiles", peak_percentiles},
             {"total_peaks", peak_count},
             {"total_peak_territory", peaks.total_peak_territory},
             {"hqaa_overlapping_peaks_percent", percentage(hqaa_overlapping_peaks, hqaa)},
             {"tss_coverage", tss_coverage_vec},
             {"tss_enrichment", tss_enrichment},
             {"chromosome_counts", chromosome_counts_json},
             {"max_fraction_reads_from_single_autosome", max_fraction_reads_from_single_autosome}
         }
        }
    };
    return result;
}

//
// Produce text version of all of a collector's Metrics
//
std::ostream& operator<<(std::ostream& os, const MetricsCollector& collector) {
    std::cout << collector.configuration_string();

    for (auto it : collector.metrics) {
        os << *(it.second);
    }

    return os;
}

nlohmann::json MetricsCollector::to_json() {
    nlohmann::json result;
    for (auto m : metrics) {
        result.push_back(m.second->to_json());
    }
    return result;
}
