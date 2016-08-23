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
#include <numeric>
#include <sstream>
#include <stdlib.h>

#include "json.hpp"

#include "HTS.hpp"
#include "IO.hpp"
#include "Metrics.hpp"
#include "Peaks.hpp"
#include "Utils.hpp"


using json = nlohmann::json;

enum {
    OPT_HELP = 100,
    OPT_VERBOSE = 101,
    OPT_VERSION = 102,
    OPT_PROBLEMATIC_READS_FILE = 103,

    OPT_NAME = 1000,
    OPT_DESCRIPTION = 1001,
    OPT_URL = 1002,
    OPT_ORGANISM = 1003,
    OPT_LIBRARY = 1004,
    OPT_METRICS_FILE = 1005,
    OPT_EXCLUDED_REGION_FILE = 1006,
    OPT_PEAK_FILE = 1007,
    OPT_REFERENCE_GENOME = 1008,
    OPT_AUTOSOMAL_REFERENCE_FILE = 1009,
    OPT_MITOCHONDRIAL_REFERENCE_NAME = 1010
};

void print_version() {
    std::cout << version_string() << std::endl;
}

void print_usage() {
    Metrics metrics = Metrics();
    std::cerr << "ataqc " << version_string() << ": QC metrics for ATAC-seq data" << std::endl << std::endl

              << "Usage:" << std::endl << std::endl << "ataqc [options] \"BAM file name\"" << std::endl << std::endl
              << "where:" << std::endl
              << "    \"BAM file name\" is a BAM file with flags set to" << std::endl
              << "                    indicate duplicate reads" << std::endl

              << std::endl

              << "Basic options" << std::endl
              << "-------------" << std::endl << std::endl

              << "--help: show this usage message." << std::endl
              << "--verbose: show more details and progress updates." << std::endl
              << "--version: print the version of the program." << std::endl

              << std::endl

              << "Optional Input" << std::endl
              << "--------------" << std::endl << std::endl

              << "--peak-file \"file name\"" << std::endl
              << "    A BED file containing peaks called for the aligned reads in the BAM file -- " << std::endl
              << "    ideally only the high quality autosomal, properly paired, uniquely mapped reads." << std::endl << std::endl

              << "--excluded-region-file \"file name\"" << std::endl
              << "    A BED file containing excluded regions. Peaks overlapping these will be ignored." << std::endl
              << "    May be given multiple times." << std::endl

              << std::endl

              << "Output" << std::endl
              << "------" << std::endl << std::endl

              << "--metrics-file \"file name\"" << std::endl
              << "    The JSON file to which metrics will be written. The default" << std::endl
              << "    filename will be based on the BAM file, with a \".ataqc.json\" suffix." << std::endl << std::endl

              << "--problematic-reads-file \"file name\"" << std::endl
              << "    If specified, problematic reads will be logged to a file." << std::endl

              << std::endl

              << "Metadata" << std::endl
              << "--------" << std::endl << std::endl

              << "The following options provide metadata to be included in the metrics JSON file." << std::endl
              << "They make it easier to compare results in the ataqc web interface." << std::endl << std::endl

              << "--name \"name\"" << std::endl
              << "    A name for the experiment." << std::endl << std::endl

              << "--description \"description\"" << std::endl
              << "    A short description of the experiment." << std::endl << std::endl

              << "--url \"URL\"" << std::endl
              << "    A URL for more detail on the experiment (perhaps using a DOI)." << std::endl << std::endl

              << "--organism \"organism\"" << std::endl
              << "    The organism studied in the experiment." << std::endl << std::endl

              << "--library \"description\"" << std::endl
              << "    A description of the assay's library (tissue, cell type, cell line, cell count)." << std::endl << std::endl

              << std::endl

              << "Reference Genome Configuration" << std::endl
              << "------------------------------" << std::endl << std::endl

              << "--reference-genome \"genome name\"" << std::endl
              << "    The reference genome for the dataset, which determines which reads" << std::endl
              << "    will be considered autosomal. Built-in references are:" << std::endl;

    std::cerr << std::endl
              << std::setw(12) << std::setfill(' ') << "Genome"
              << "  " << "Autosomal References"
              << std::endl
              << std::setw(12) << std::setfill(' ') << "-------"
              << "  " << "------------------"
              << std::endl;

    for (const auto &it : metrics.genome_autosomal_references) {
        std::vector<std::string> autosomal_references;
        for (const auto& rit : it.second) {
            autosomal_references.push_back(rit.first.substr(rit.first.find_first_not_of("chr")));
        }
        std::sort(autosomal_references.begin(), autosomal_references.end(), sort_strings_numerically);
        std::cerr << std::right << std::setw(12) << std::setfill(' ') << it.first << "  ";
        for (const auto& rit : autosomal_references) {
            std::cerr << rit << ' ';
        }
        std::cerr << std::endl;

    }

    std::cerr << std::endl << "    The default is GRCh37. If you're studying an organism for which" << std::endl
              << "    we don't provide a reference, or want to change the list of" << std::endl
              << "    autosomes, use this option with --autosomal-reference-file." << std::endl << std::endl

              << "--autosomal-reference-file \"file name\"" << std::endl
              << "    A file containing a list of chromosome names, one per line, that will" << std::endl
              << "    be considered autosomal for the organism specified with --reference-genome." << std::endl
              << "    They must of course match the reference names exactly." << std::endl  << std::endl

              << "--mitochondrial-reference-name \"name\"" << std::endl
              << "    The reference name for mitochondria in your reference genome." << std::endl << std::endl;

}

int main(int argc, char **argv) {
    int c, option_index = 0;
    bool verbose = 0;

    std::string name;
    std::string description;
    std::string url;
    std::string organism;
    std::string library;

    std::string alignment_filename;
    std::string reference_genome("GRCh37");
    std::string autosomal_reference_filename;
    std::string mitochondrial_reference_name;
    std::string peak_filename;
    std::vector<std::string> excluded_region_filenames;

    std::string metrics_filename;
    std::string problematic_reads_filename;
    boost::shared_ptr<boost::iostreams::filtering_ostream> metrics_file;

    static struct option long_options[] = {
        {"help", no_argument, nullptr, OPT_HELP},
        {"verbose", no_argument, nullptr, OPT_VERBOSE},
        {"version", no_argument, nullptr, OPT_VERSION},
        {"problematic-reads-file", required_argument, nullptr, OPT_PROBLEMATIC_READS_FILE},
        {"name", required_argument, nullptr, OPT_NAME},
        {"description", required_argument, nullptr, OPT_DESCRIPTION},
        {"url", required_argument, nullptr, OPT_URL},
        {"organism", required_argument, nullptr, OPT_ORGANISM},
        {"library", required_argument, nullptr, OPT_LIBRARY},
        {"metrics-file", required_argument, nullptr, OPT_METRICS_FILE},
        {"excluded-region-file", required_argument, nullptr, OPT_EXCLUDED_REGION_FILE},
        {"peak-file", required_argument, nullptr, OPT_PEAK_FILE},
        {"reference-genome", required_argument, nullptr, OPT_REFERENCE_GENOME},
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
        case OPT_PROBLEMATIC_READS_FILE:
            problematic_reads_filename = optarg;
            break;
        case OPT_NAME:
            name = optarg;
            break;
        case OPT_DESCRIPTION:
            description = optarg;
            break;
        case OPT_URL:
            url = optarg;
            break;
        case OPT_ORGANISM:
            organism = optarg;
            break;
        case OPT_LIBRARY:
            library = optarg;
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
        case OPT_REFERENCE_GENOME:
            reference_genome = std::string(optarg);
            break;
        case OPT_AUTOSOMAL_REFERENCE_FILE:
            autosomal_reference_filename = optarg;
            break;
        case OPT_MITOCHONDRIAL_REFERENCE_NAME:
            mitochondrial_reference_name = optarg;
            break;
        case 0:
            if (long_options[option_index].flag != 0) {
                break;
            }
        default:
            print_usage();
            exit(1);
        }
    }

    // Make sure the BAM file was specified
    if ((optind + 1) > argc) {
        print_usage();
        exit(1);
    }

    alignment_filename = argv[optind];

    Metrics metrics;

    if (name.empty()) {
        metrics.name = basename(alignment_filename);
    } else {
        metrics.name = name;
    }

    if (!description.empty()) {
        metrics.description = description;
    }

    if (!url.empty()) {
        metrics.url = url;
    }

    if (!reference_genome.empty()) {
        metrics.reference_genome = reference_genome;
    }

    if (!organism.empty()) {
        metrics.organism = organism;
    }

    if (!library.empty()) {
        metrics.library = library;
    }

    if (!excluded_region_filenames.empty()) {
        metrics.excluded_region_filenames = excluded_region_filenames;
    }

    if (!alignment_filename.empty()) {
        metrics.alignment_filename = alignment_filename;
    }

    if (!peak_filename.empty()) {
        metrics.peak_filename = peak_filename;
    }

    if (!autosomal_reference_filename.empty()) {
        metrics.autosomal_reference_filename = autosomal_reference_filename;
    }

    if (!mitochondrial_reference_name.empty()) {
        metrics.mitochondrial_reference_name = mitochondrial_reference_name;
    }

    if (!problematic_reads_filename.empty()) {
        metrics.problematic_reads_filename = problematic_reads_filename;
    }

    metrics.verbose = verbose;

    // if the filename for the metrics output wasn't specified,
    // construct it from the source BAM filename
    if (metrics_filename.empty()) {
        metrics_filename = basename(alignment_filename);
        metrics_filename += ".ataqc.json";
    }

    try {
        metrics_file = mostream(metrics_filename);
    } catch (FileException& e) {
        std::cerr << "Could not open metrics file \"" << metrics_filename << "\" for writing: " << e.what() << std::endl;
        exit(1);
    }

    // Make sure the reference genome is valid
    if (!(metrics.genome_autosomal_references.count(reference_genome))) {
        std::cerr << "Sorry, \"" << reference_genome << "\"" << " is not a recognized reference genome." << std::endl << std::endl
                  << "If that's really the name of your reference genome and not an error," << std::endl
                  << "you can supply its autosomal references with the --autosomal-reference-file option." << std::endl;
        exit(1);
    }

    try {
        if (!autosomal_reference_filename.empty()) {
            metrics.load_autosomal_reference();
        }
        if (!excluded_region_filenames.empty()) {
            metrics.load_excluded_regions();
        }
        metrics.load_alignments();
        metrics.load_peaks();
    } catch (FileException& e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }

    std::cout << metrics << std::endl;  // Print the stats

    std::cout << "Making JSON metrics..." << std::endl;

    json metrics_json = metrics.to_json();

    std::cout << "Writing JSON metrics to " << metrics_filename << std::endl;

    *metrics_file << std::setw(2) << metrics_json << std::endl;

    std::cout << "Metrics written to \"" << metrics_filename << "\"" << std::endl;

    std::cout << "Finished." << std::endl;
}
