//
// Copyright 2015 The Parker Lab at the University of Michigan
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

#include "HTS.hpp"
#include "Metrics.hpp"
#include "Peaks.hpp"
#include "Types.hpp"
#include "Utils.hpp"

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 2

std::string version_string() {
    std::stringstream ss;
    ss << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH;
    return ss.str();
}

void print_version() {
    std::cout << version_string() << std::endl;
}

void print_usage() {
    Metrics metrics = Metrics();
    std::cerr << "ataqc " << version_string() << ": calculate QC metrics for ATAC-seq data" << std::endl << std::endl

              << "Usage:" << std::endl << std::endl << "ataqc [options] <BAM-file-with-duplicates-marked> <sample> <group>" << std::endl << std::endl
              << "where:" << std::endl
              << "    <BAM-file-with-duplicates-marked> is a BAM file with flags set to" << std::endl
              << "                                      indicate duplicate reads" << std::endl

              << "    <sample> is the ID of the sample in the assay, e.g. \"EndoC_50\"" << std::endl

              << "    <group> is the ID of the group in the assay, e.g. \"1\"" << std::endl << std::endl

              << "    The sample and group IDs are only used to put additional" << std::endl
              << "    columns in the output files, to make it easier to combine" << std::endl
              << "    output files in plots." << std::endl << std::endl

              << "Options may include:" << std::endl << std::endl

              << "-h|--help: show this usage message." << std::endl << std::endl
              << "-v|--verbose: show more details and progress updates." << std::endl << std::endl
              << "--version: print the version of the program." << std::endl << std::endl

              << "-s|--summary-file" << std::endl
              << "    The file to which summary statistics will be written. The default" << std::endl
              << "    filename will be the BAM file with \".summary\" appended." << std::endl << std::endl

              << "-t|--template-metrics-file" << std::endl
              << "    The file to which to write observed template lengths. The default" << std::endl
              << "    filename will be the BAM file with \".template_lengths\" appended." << std::endl << std::endl

              << "-p|--peak-file" << std::endl
              << "    The optional BED file containing peaks called for the nonduplicate, " << std::endl
              << "    properly paired and mapped reads from the BAM file; " << std::endl
              << "    Required if --peak-overlap-file is given." << std::endl << std::endl

              << "-o|--peak-overlap-file" << std::endl
              << "    The optional BED file produced by bedtools intersect containing" << std::endl
              << "    peaks called for the BAM file from nonduplicate, properly paired and" << std::endl
              << "    mapped reads overlapping them. Required if --peak-file is given." << std::endl << std::endl

              << "-m|--peak-metrics-file" << std::endl
              << "    The file to which to write peak metrics, if a peak file is supplied." << std::endl
              << "    The default filename will be the BAM file with  \".peak_metrics\" " << std::endl
              << "    appended." << std::endl << std::endl

              << "-g|--reference-genome" << std::endl
              << "    The reference genome for the dataset, which determines which reads" << std::endl
              << "    will be considered autosomal. Built-in references are:" << std::endl << std::endl;

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

    std::cerr << std::endl << "    The default is GRCh37. If you're studying another organism and want" << std::endl
              << "    to change autosomal chromosome lists, use this option with" << std::endl
              << "    --autosomal-reference-file." << std::endl << std::endl

              << "-n|--autosomal-reference-file" << std::endl
              << "    A file containing a list of chromosome names, one per line, that will" << std::endl
              << "    be considered autosomal for the organism specified with --reference-genome" << std::endl
              << "    and determine which templates' sizes are counted." << std::endl << std::endl;
}

int main(int argc, char **argv) {

    int c, option_index = 0;
    bool verbose = 0;
    Metrics metrics = Metrics();

    std::string source;

    std::string reference_genome("GRCh37");
    std::string autosomal_reference_filename;

    std::string summary_filename;
    std::ofstream summary_file;

    std::string template_metrics_filename;
    std::ofstream template_metrics_file;

    std::string peak_filename;
    std::string peak_overlap_filename;
    std::string peak_metrics_filename;
    std::ofstream peak_metrics_file;

    static struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"verbose", no_argument, NULL, 'v'},
        {"version", no_argument, NULL, 0},
        {"summary-file", required_argument, NULL, 's'},
        {"template-metrics-file", required_argument, NULL, 'f'},
        {"peak-file", required_argument, NULL, 'p'},
        {"peak-overlap-file", required_argument, NULL, 'o'},
        {"peak-metrics-file", required_argument, NULL, 'm'},
        {"reference-genome", required_argument, NULL, 'g'},
        {"autosomal-reference-file", required_argument, NULL, 'a'},
        {0, 0, 0, 0}
    };

    // parse the command line arguments
    while ((c = getopt_long(argc, argv, "0a:f:g:m:p:o:s:t:u:vh?", long_options, &option_index)) != -1) {
        switch (c) {
        case 'h':
        case '?':
            print_usage();
            exit(1);
        case 'v':
            verbose = true;
            break;
        case 'f':
            template_metrics_filename = optarg;
            break;
        case 'm':
            peak_metrics_filename = optarg;
            break;
        case 'p':
            peak_filename = optarg;
            break;
        case 'o':
            peak_overlap_filename = optarg;
            break;
        case 'g':
            reference_genome = std::string(optarg);
            metrics.reference_genome = reference_genome;
            break;
        case 's':
            summary_filename = optarg;
            break;
        case 'a':
            autosomal_reference_filename = optarg;
            break;
        case 0:
            if (long_options[option_index].flag != 0){
                break;
            }

            if (long_options[option_index].name == std::string("version")) {
                print_version();
                exit(1);
            }
        default:
            print_usage();
            exit(1);
        }
    }

    if ((!peak_filename.empty() && peak_overlap_filename.empty()) || (peak_filename.empty() && !peak_overlap_filename.empty())) {
        std::cerr << "Error: if either of --peak-file or --peak-overlap-file are specified," << std::endl
                  << "you must also specify its companion." << std::endl;
        print_usage();
        exit(1);
    }

    if (!autosomal_reference_filename.empty()) {
        metrics.load_autosomal_reference(metrics.reference_genome, autosomal_reference_filename, verbose);
    }

    // Make sure the BAM file was specified
    if ((optind + 3) > argc) {
        print_usage();
        exit(1);
    }

    source = argv[optind];
    metrics.sample = argv[optind + 1];
    metrics.group = argv[optind + 2];

    // if the filename for the template size counts wasn't specified,
    // construct it from the source BAM filename
    if (summary_filename.empty()) {
        summary_filename = source;
        summary_filename += ".summary";
    }

    // Open the template size file here to make sure we can, before
    // processing the entire BAM file
    summary_file.open(summary_filename.c_str());
    if (summary_file.fail() || !summary_file.is_open()) {
        std::cerr << "Could not open summary table file \"" << summary_filename << "\": " << strerror(errno) << std::endl;
        exit(1);
    }

    // if the filename for the template size counts wasn't specified,
    // construct it from the source BAM filename
    if (template_metrics_filename.empty()) {
        template_metrics_filename = source;
        template_metrics_filename += ".template_lengths";
    }

    // Open the template size file here to make sure we can, before
    // processing the entire BAM file
    template_metrics_file.open(template_metrics_filename.c_str());
    if (template_metrics_file.fail() || !template_metrics_file.is_open()) {
        std::cerr << "Could not open template size table file \"" << template_metrics_filename << "\": " << strerror(errno) << std::endl;
        exit(1);
    }

    // if the filename for the peak read counts wasn't specified,
    // construct it from the source BAM filename
    if (peak_metrics_filename.empty()) {
        peak_metrics_filename = source;
        peak_metrics_filename += ".peak_metrics";
    }

    // open peak read count output file here, to make sure we can
    // before processing the entire BAM file
    if (!peak_filename.empty()) {
        peak_metrics_file.open(peak_metrics_filename.c_str());
        if (peak_metrics_file.fail() || !peak_metrics_file.is_open()) {
            std::cerr << "Could not open peak read count file \"" << peak_metrics_filename << "\": " << strerror(errno) << std::endl;
            exit(1);
        }
    }

    // Make sure the reference genome is valid
    if (!(metrics.genome_autosomal_references.count(reference_genome))) {
        std::cerr << "Sorry, \"" << reference_genome << "\"" << " is not a recognized reference genome." << std::endl << std::endl
                  << "If that's really the name of your reference genome and not an error," << std::endl
                  << "you can supply its autosomal references with the --autosomal-reference-file option." << std::endl;
        exit(1);
    }

    try {
        metrics.load_bam_file(source, verbose);  // Read the BAM file
        std::cout << metrics << std::endl;  // Print the stats
    } catch (FileException& e) {
        std::cerr << "Error loading BAM file: " << e.what() << std::endl;
        exit(1);
    }

    PeakMetrics peak_metrics = PeakMetrics(metrics);

    // if peak statistics were requested, report them
    if (!peak_filename.empty()) {
        peak_metrics.load_peaks(peak_filename, peak_overlap_filename, verbose);
        if (peak_metrics.peaks.empty()) {
            std::cout << "No peaks were found in " << peak_filename << std::endl;
        } else {
            peak_metrics.write_peak_metrics(peak_metrics_file);
            std::cout << peak_metrics;
        }
    }
    peak_metrics_file.close();
    std::cout << std::endl << "Peak metrics written to \"" << peak_metrics_filename << "\"" << std::endl;

    metrics.write_template_lengths(template_metrics_file);
    template_metrics_file.close();
    std::cout << "Template size metrics written to \"" << template_metrics_filename << "\"" << std::endl;

    metrics.write_table_column_headers(summary_file);
    if (!peak_filename.empty()) {
        summary_file << "\t";
        peak_metrics.write_table_column_headers(summary_file);
    }

    summary_file << std::endl;

    metrics.write_table_columns(summary_file);
    if (!peak_filename.empty()) {
        summary_file << "\t";
        peak_metrics.write_table_columns(summary_file);
    }

    summary_file << std::endl;
    summary_file.close();
    std::cout << "Summary metrics written to \"" << summary_filename << "\"" << std::endl;

    std::cout << "Finished." << std::endl;
}
