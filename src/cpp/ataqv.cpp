//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

#include <algorithm>
#include <cerrno>
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <ncurses.h>
#include <numeric>
#include <set>
#include <sstream>
#include <stdlib.h>

#include <boost/filesystem.hpp>

#include "HTS.hpp"
#include "IO.hpp"
#include "Metrics.hpp"
#include "Peaks.hpp"
#include "Utils.hpp"


enum {
    OPT_HELP,
    OPT_VERBOSE,
    OPT_VERSION,

    OPT_THREADS,

    OPT_PEAK_FILE,
    OPT_TSS_FILE,
    OPT_TSS_EXTENSION,
    OPT_EXCLUDED_REGION_FILE,

    OPT_METRICS_FILE,
    OPT_LOG_PROBLEMATIC_READS,
    OPT_TABULAR_OUTPUT,
    OPT_LESS_REDUNDANT,

    OPT_NAME,
    OPT_IGNORE_READ_GROUPS,
    OPT_NUCLEUS_BARCODE_TAG,
    OPT_DESCRIPTION,
    OPT_LIBRARY_DESCRIPTION,
    OPT_URL,

    OPT_ORGANISM,
    OPT_AUTOSOMAL_REFERENCE_FILE,
    OPT_MITOCHONDRIAL_REFERENCE_NAME
};


void print_version() {
    std::cout << version_string() << std::endl;
}


void print_usage() {
    MetricsCollector collector = MetricsCollector();
    std::cout << "ataqv " << version_string() << ": QC metrics for ATAC-seq data" << std::endl << std::endl

              << "Usage:" << std::endl << std::endl << "ataqv [options] organism alignment-file" << std::endl << std::endl
              << "where:" << std::endl
              << "    organism is the subject of the experiment, which determines the list of autosomes"  << std::endl
              << "    (see \"Reference Genome Configuration\" below)."  << std::endl  << std::endl
              << "    alignment-file is a BAM file with duplicate reads marked." << std::endl

              << std::endl

              << "Basic options" << std::endl
              << "-------------" << std::endl << std::endl

              << "--help: show this usage message." << std::endl
              << "--verbose: show more details and progress updates." << std::endl
              << "--version: print the version of the program." << std::endl
              << "--threads <n>: the maximum number of threads to use (right now, only for calculating TSS enrichment)." << std::endl << std::endl

              << "Optional Input" << std::endl
              << "--------------" << std::endl << std::endl

              << "--peak-file \"file name\"" << std::endl
              << "    A BED file of peaks called for alignments in the BAM file. Specify \"auto\" to use the" << std::endl
              << "    BAM file name with \".peaks\" appended, or if the BAM file contains read groups, to" << std::endl
              << "    assume each read group has a peak file whose name is the read group ID with \".peaks\"" << std::endl
              << "    appended. If you specify a single filename instead of \"auto\" with read groups, the " << std::endl
              << "    same peaks will be used for all reads -- be sure this is what you want." << std::endl << std::endl

              << "--tss-file \"file name\"" << std::endl
              << "    A BED file of transcription start sites for the experiment organism. If supplied," << std::endl
              << "    a TSS enrichment score will be calculated according to the ENCODE data standards." << std::endl
              << "    This calculation requires that the BAM file of alignments be indexed." << std::endl << std::endl

              << "--tss-extension \"size\"" << std::endl
              << "    If a TSS enrichment score is requested, it will be calculated for a region of " << std::endl
              << "    \"size\" bases to either side of transcription start sites. The default is 1000bp." << std::endl << std::endl

              << "--excluded-region-file \"file name\"" << std::endl
              << "    A BED file containing excluded regions. Peaks or TSS overlapping these will be ignored." << std::endl
              << "    May be given multiple times." << std::endl

              << std::endl

              << "Output" << std::endl
              << "------" << std::endl << std::endl

              << "--metrics-file \"file name\"" << std::endl
              << "    The JSON file to which metrics will be written. The default filename will be based on" << std::endl
              << "    the BAM file, with the suffix \".ataqv.json\"." << std::endl << std::endl

              << "--log-problematic-reads" << std::endl
              << "    If given, problematic reads will be logged to a file per read group, with names" << std::endl
              << "    derived from the read group IDs, with \".problems\" appended. If no read groups" << std::endl
              << "    are found, the reads will be written to one file named after the BAM file." << std::endl << std::endl

              << "--tabular-output" << std::endl
              << "    If given, the metrics file output will be a tabular (TSV) text file, not JSON. This " << std::endl
              << "    output CANNOT be used to generate the HTML report, and excludes several metrics that" << std::endl
              << "    would otherwise be included in the JSON output (e.g., the full fragment length" << std::endl
              << "    distribution, the full TSS coverage curve, and the full mapping quality distribution)." << std::endl
              << "    This option is not recommended when analyzing bulk ATAC-seq data, but may be useful" << std::endl
              << "    when analyzing single nucleus ATAC-seq data with large numbers of distinct cell" << std::endl
              << "    barcodes (say, >100k); in such a case this option should substantially reduce memory" << std::endl
              << "    usage, reduce runtime, and avoid the need to parse a large JSON file in downstream" << std::endl
              << "    analysis, while still outputting the metrics commonly used to QC single nucleus" << std::endl
              << "    ATAC-seq data (TSS enrichment, read counts, and mitochondrial read counts, amongst others)." << std::endl << std::endl

              << "--less-redundant" << std::endl
              << "    If given, output a subset of metrics that should be less redundant. If this flag is used," << std::endl
              << "    the same flag should be passed to mkarv when making the viewer." << std::endl

              << std::endl

              << "Metadata" << std::endl
              << "--------" << std::endl << std::endl

              << "The following options provide metadata to be included in the metrics JSON file." << std::endl
              << "They make it easier to compare results in the ataqv web interface." << std::endl << std::endl

              << "--name \"name\"" << std::endl
              << "    A label to be used for the metrics when there are no read groups. If there are read" << std::endl
              << "    groups, each will have its metrics named using its ID field. With no read groups and" << std::endl
              << "    no --name given, your metrics will be named after the alignment file."  << std::endl << std::endl

              << "--ignore-read-groups" << std::endl
              << "    Even if read groups are present in the BAM file, ignore them and combine metrics" << std::endl
              << "    for all reads under a single sample and library named with the --name option. This" << std::endl
              << "    also implies that a single peak file will be used for all reads; see the --peak option." << std::endl << std::endl

              << "--nucleus-barcode-tag \"nucleus_barcode_tag\"" << std::endl
              << "    Data is single-nucleus, with the barcode stored in this BAM tag." << std::endl
              << "    In this case, metrics will be collected per barcode." << std::endl << std::endl

              << "--description \"description\"" << std::endl
              << "    A short description of the experiment." << std::endl << std::endl

              << "--url \"URL\"" << std::endl
              << "    A URL for more detail on the experiment (perhaps using a DOI)." << std::endl << std::endl

              << "--library-description \"description\"" << std::endl
              << "    Use this description for all libraries in the BAM file, instead of using the DS" << std::endl
              << "    field from each read group." << std::endl << std::endl

              << std::endl

              << "Reference Genome Configuration" << std::endl
              << "------------------------------" << std::endl << std::endl

              << "ataqv includes lists of autosomes for several organisms:" << std::endl << std::endl

              << std::setw(12) << std::setfill(' ') << "Organism"
              << "  " << "Autosomal References"
              << std::endl
              << std::setw(12) << std::setfill(' ') << "-------"
              << "  " << "------------------"
              << std::endl;

    for (const auto &it : collector.autosomal_references) {
        bool (*sort_function_ptr)(const std::string&, const std::string&) = sort_strings_with_roman_numerals;
        std::set<std::string, bool(*)(const std::string&, const std::string&)> autosomal_references(sort_function_ptr);
        for (const auto& rit : it.second) {
            autosomal_references.insert(rit.first.substr(rit.first.find_first_not_of("chr")));
        }

        std::cout << std::right << std::setw(12) << std::setfill(' ') << it.first << "  ";
        for (const auto& rit : autosomal_references) {
            std::cout << rit << ' ';
        }
        std::cout << std::endl;
    }

    std::cout << std::endl
              << "    The default autosomal reference lists contain names with \"chr\" prefixes" << std::endl
              << "    (\"chr1\") and without (\"1\"). If you need a different set of autosomes, you can"  << std::endl
              << "    supply a list with --autosomal-reference-file." << std::endl << std::endl

              << "--autosomal-reference-file \"file name\"" << std::endl
              << "    A file containing autosomal reference names, one per line. The names must match" << std::endl
              << "    the reference names in the alignment file exactly, or the metrics based on counts" <<std::endl
              << "    of autosomal alignments will be wrong." << std::endl  << std::endl

              << "--mitochondrial-reference-name \"name\"" << std::endl
              << "    If the reference name for mitochondrial DNA in your alignment file is not \"chrM\",." << std::endl
              << "    use this option to supply the correct name. Again, if this name is wrong, all the"<< std::endl
              << "    measurements involving mitochondrial alignments will be wrong." << std::endl << std::endl;
}


template <typename T>
void print_error(T t)
{
    initscr();
    std::string start_error = has_colors() ? "\033[1;31m" : "";
    std::string end_error = has_colors() ? "\033[1;0m" : "";
    endwin();

    std::cerr << std::endl << start_error << t << end_error << std::endl << std::endl;
}


template<typename T, typename... Args>
void print_error(T t, Args... args)
{
    print_error(t);
    print_error(args...) ;
}


int main(int argc, char **argv) {

    int c, option_index = 0;
    bool verbose = false;
    int thread_limit = 1;
    bool log_problematic_reads = false;
    bool tabular_output = false;
    bool less_redundant = false;

    std::string name;
    bool ignore_read_groups = false;
    std::string description;
    bool is_single_nucleus = false;
    std::string nucleus_barcode_tag;
    std::string library_description;
    std::string url;
    std::string organism;
    std::string library;

    std::string alignment_filename;
    std::string autosomal_reference_filename;
    std::string mitochondrial_reference_name = "chrM";
    std::string peak_filename;
    std::string tss_filename;
    int tss_extension = 1000;
    std::vector<std::string> excluded_region_filenames;

    std::string metrics_filename;
    boost::shared_ptr<boost::iostreams::filtering_ostream> metrics_file;

    static struct option long_options[] = {
        {"help", no_argument, nullptr, OPT_HELP},
        {"verbose", no_argument, nullptr, OPT_VERBOSE},
        {"version", no_argument, nullptr, OPT_VERSION},
        {"threads", required_argument, nullptr, OPT_THREADS},
        {"log-problematic-reads", no_argument, nullptr, OPT_LOG_PROBLEMATIC_READS},
        {"tabular-output", no_argument, nullptr, OPT_TABULAR_OUTPUT},
        {"less-redundant", no_argument, nullptr, OPT_LESS_REDUNDANT},
        {"name", required_argument, nullptr, OPT_NAME},
        {"ignore-read-groups", no_argument, nullptr, OPT_IGNORE_READ_GROUPS},
        {"nucleus-barcode-tag", required_argument, nullptr, OPT_NUCLEUS_BARCODE_TAG},
        {"description", required_argument, nullptr, OPT_DESCRIPTION},
        {"library-description", required_argument, nullptr, OPT_LIBRARY_DESCRIPTION},
        {"url", required_argument, nullptr, OPT_URL},
        {"metrics-file", required_argument, nullptr, OPT_METRICS_FILE},
        {"excluded-region-file", required_argument, nullptr, OPT_EXCLUDED_REGION_FILE},
        {"peak-file", required_argument, nullptr, OPT_PEAK_FILE},
        {"tss-file", required_argument, nullptr, OPT_TSS_FILE},
        {"tss-extension", required_argument, nullptr, OPT_TSS_EXTENSION},
        {"autosomal-reference-file", required_argument, nullptr, OPT_AUTOSOMAL_REFERENCE_FILE},
        {"mitochondrial-reference-name", required_argument, nullptr, OPT_MITOCHONDRIAL_REFERENCE_NAME},
        {0, 0, 0, 0}
    };

    // parse the command line arguments
    while ((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        switch (c) {
        case OPT_HELP:
            print_usage();
            exit(1);
        case OPT_VERBOSE:
            verbose = true;
            break;
        case OPT_VERSION:
            print_version();
            exit(1);
        case OPT_THREADS:
            thread_limit = std::stoi(optarg);
            break;
        case OPT_LOG_PROBLEMATIC_READS:
            log_problematic_reads = true;
            break;
        case OPT_TABULAR_OUTPUT:
            tabular_output = true;
            break;
        case OPT_LESS_REDUNDANT:
            less_redundant = true;
            break;
        case OPT_NAME:
            name = optarg;
            break;
        case OPT_IGNORE_READ_GROUPS:
            ignore_read_groups = true;
            break;
        case OPT_NUCLEUS_BARCODE_TAG:
            nucleus_barcode_tag = optarg;
            is_single_nucleus = true;
            break;
        case OPT_DESCRIPTION:
            description = optarg;
            break;
        case OPT_LIBRARY_DESCRIPTION:
            library_description = optarg;
            break;
        case OPT_URL:
            url = optarg;
            break;
        case OPT_METRICS_FILE:
            metrics_filename = optarg;
            break;
        case OPT_EXCLUDED_REGION_FILE:
            excluded_region_filenames.push_back(optarg);
            break;
        case OPT_PEAK_FILE:
            peak_filename = optarg;
            break;
        case OPT_TSS_FILE:
            tss_filename = optarg;
            break;
        case OPT_TSS_EXTENSION:
            tss_extension = std::stoi(optarg);
            break;
        case OPT_AUTOSOMAL_REFERENCE_FILE:
            autosomal_reference_filename = optarg;
            break;
        case OPT_MITOCHONDRIAL_REFERENCE_NAME:
            mitochondrial_reference_name = optarg;
            break;
        default:
            print_usage();
            exit(1);
        }
    }

    // Make sure the BAM file was specified
    if (optind + 2 > argc) {
        print_error("ERROR: Please specify the organism and alignment file.");
        print_usage();
        exit(1);
    }

    organism = argv[optind];
    alignment_filename = argv[optind + 1];

    if (organism.empty()) {
        print_error("ERROR: Please specify the organism for the libraries in this alignment file.");
        exit(1);
    }

    if (alignment_filename.empty()) {
        print_error("ERROR: Please specify the alignment file.");
        exit(1);
    }

    if (!boost::filesystem::exists(alignment_filename)) {
        print_error("ERROR: The specified alignment file does not exist.");
        exit(1);
    }

    try {
        MetricsCollector collector(
            name,
            organism,
            nucleus_barcode_tag,
            description,
            library_description,
            url,
            alignment_filename,
            autosomal_reference_filename,
            mitochondrial_reference_name,
            peak_filename,
            tss_filename,
            tss_extension,
            verbose,
            thread_limit,
            ignore_read_groups,
            is_single_nucleus,
            log_problematic_reads,
            !tabular_output,
            less_redundant,
            excluded_region_filenames);

        // if the filename for the metrics output wasn't specified,
        // construct it from the source BAM filename
        if (metrics_filename.empty()) {
            metrics_filename = basename(alignment_filename);
            metrics_filename += ".ataqv.json";
        }

        try {
            metrics_file = mostream(metrics_filename);
        } catch (FileException& e) {
            print_error("ERROR: Could not open metrics file \"" + metrics_filename + "\" for writing: " + e.what());
            exit(1);
        }

        // Make sure the reference genome is valid
        if (!(collector.autosomal_references.count(organism))) {
            print_error(
                "ERROR: Sorry, we don't have a list of autosomal references for \"" + organism + "\".\n"
                "You can name its autosomes with the --autosomal-reference-file option."
            );
            exit(1);
        }

        collector.load_alignments();

        std::cout << collector << std::endl;  // Print the metrics

        if (!tabular_output) {
            std::cout << "Writing JSON metrics to " << metrics_filename << std::endl << std::flush;
            *metrics_file << std::setw(2) << collector.to_json();
        } else {
            std::cout << "Writing tabular metrics to " << metrics_filename << std::endl << std::flush;
            collector.to_table(metrics_file);
        }
        std::cout << "Metrics written to \"" << metrics_filename << "\"" << std::endl;

    } catch (FileException& e) {
        print_error("ERROR: " + std::string(e.what()));
        exit(1);
    }
    std::cout << "Finished." << std::endl << std::flush;
}
