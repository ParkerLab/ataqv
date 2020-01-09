#include <cstdio>

#include "catch.hpp"

#include "Metrics.hpp"


TEST_CASE("MetricsCollector basics", "[metrics/collector]") {
    MetricsCollector collector("Test collector", "human", "a collector for unit tests", "a library of brutal tests?", "https://theparkerlab.org", "test.bam");

    SECTION("MetricsCollector::is_autosomal") {
        REQUIRE(collector.is_autosomal("chr1"));
        REQUIRE(collector.is_autosomal("1"));
        REQUIRE_FALSE(collector.is_autosomal("chrX"));
        REQUIRE_FALSE(collector.is_autosomal("foo"));
    }

    SECTION("MetricsCollector::is_mitochondrial") {
        REQUIRE(collector.is_mitochondrial("chrM"));
        REQUIRE_FALSE(collector.is_mitochondrial("chr1"));
        REQUIRE_FALSE(collector.is_mitochondrial("foo"));
    }

    SECTION("MetricsCollector::configuration_string") {
        std::string expected = "ataqv " + version_string() + "\n\n" +
            "Operating parameters\n" +
            "====================\n" +
            "Thread limit: 1\n" +
            "Ignoring read groups: no\n\n" +
            "Experiment information\n" +
            "======================\n" +
            "Organism: human\n" +
            "Description: a collector for unit tests\n" +
            "URL: https://theparkerlab.org\n\n" +
            "Reference genome configuration\n" +
            "==============================\n" +
            "Mitochondrial reference: chrM\n" +
            "Autosomal references: \n" +
            "  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,\n" +
            "  20, 21, 22, chr1, chr2, chr3, chr4, chr5, chr6, chr7, chr8, chr9,\n" +
            "  chr10, chr11, chr12, chr13, chr14, chr15, chr16, chr17, chr18,\n" +
            "  chr19, chr20, chr21, chr22\n\n\n";
        REQUIRE(expected == collector.configuration_string());
    }

    SECTION("MetricsCollector::to_json") {
        nlohmann::json collector_json = collector.to_json();
        REQUIRE("null" == collector_json.dump());  // that's all you get with no metrics
    }

    SECTION("MetricsCollector::autosomal_reference_string") {
        collector.organism = "martian hvac louse";
        REQUIRE("" == collector.autosomal_reference_string());
    }

}


TEST_CASE("MetricsCollector::test_supplied_references", "[metrics/test_supplied_references]") {
    std::string autosomal_reference_file = "autosomal_references.gz";
    {
        auto out = mostream(autosomal_reference_file);
        *out << "I\nII\nIII\n";
    }

    MetricsCollector collector("Test collector", "human", "a collector for unit tests", "a library of brutal tests?", "https://theparkerlab.org", "test.bam", autosomal_reference_file, "M", "", "", 1000, true);

    std::remove(autosomal_reference_file.c_str());

    SECTION("MetricsCollector::is_autosomal") {
        REQUIRE(collector.is_autosomal("I"));
        REQUIRE(collector.is_autosomal("II"));
        REQUIRE(collector.is_autosomal("III"));
        REQUIRE_FALSE(collector.is_autosomal("IV"));
        REQUIRE_FALSE(collector.is_autosomal("foo"));
    }

    SECTION("MetricsCollector::is_mitochondrial") {
        REQUIRE(collector.is_mitochondrial("M"));
        REQUIRE_FALSE(collector.is_mitochondrial("chrM"));
        REQUIRE_FALSE(collector.is_mitochondrial("I"));
    }

    SECTION("Bad autosomal reference file") {
        REQUIRE_THROWS(MetricsCollector badcollector("Test collector", "human", "a collector with a bad autosomal reference file", "a library of brutal tests?", "https://theparkerlab.org", "test.bam", "bad_autosomal_reference_file.txt"));
    }
}


TEST_CASE("Metrics::load_alignments with no excluded regions", "[metrics/load_alignments_with_no_excluded_regions]") {
    std::string name("Test collector");
    std::string alignment_file_name("SRR891275.bam");
    std::string peak_file_name("SRR891275.peaks.gz");

    MetricsCollector collector(name, "human", "a collector for unit tests", "a library of brutal tests?", "https://theparkerlab.org", alignment_file_name, "", "chrM", peak_file_name, "", true);

    collector.load_alignments();

    Metrics* metrics = collector.metrics.cbegin()->second;

    REQUIRE(metrics->peaks.size() == 16499);
}


TEST_CASE("Metrics::load_alignments", "[metrics/load_alignments]") {
    std::string name("Test collector");
    std::string alignment_file_name("test.bam");
    std::string peak_file_name("test.peaks.gz");
    std::string tss_file_name("hg19.tss.refseq.bed.gz");

    MetricsCollector collector(name, "human", "a collector for unit tests", "a library of brutal tests?", "https://theparkerlab.org", alignment_file_name, "", "chrM", peak_file_name, tss_file_name, 1000, true, 1, false, true, {"exclude.dac.bed.gz", "exclude.duke.bed.gz"});

    collector.load_alignments();

    std::cout << collector << std::endl;

    REQUIRE(collector.metrics.size() == 2);

    Metrics* metrics = collector.metrics.cbegin()->second;
    REQUIRE(metrics->total_reads == 520);
    REQUIRE(metrics->properly_paired_and_mapped_reads == 416);
    REQUIRE(metrics->secondary_reads == 10);
    REQUIRE(metrics->supplementary_reads == 0);
    REQUIRE(metrics->duplicate_reads == 155);
    REQUIRE(metrics->hqaa == 200);
    REQUIRE(metrics->hqaa_short_count == 41);
    REQUIRE(metrics->hqaa_mononucleosomal_count == 32);
    REQUIRE((metrics->hqaa_short_count / (double)metrics->hqaa_mononucleosomal_count) == Approx(1.28125));

    REQUIRE(metrics->paired_reads == 520);
    REQUIRE(metrics->paired_and_mapped_reads == 424);
    REQUIRE(metrics->fr_reads == 416);
    REQUIRE(metrics->first_reads == 260);
    REQUIRE(metrics->second_reads == 260);
    REQUIRE(metrics->forward_reads == 268);
    REQUIRE(metrics->reverse_reads == 252);
    REQUIRE(metrics->forward_mate_reads == 261);
    REQUIRE(metrics->reverse_mate_reads == 259);

    REQUIRE(metrics->unmapped_reads == 9);
    REQUIRE(metrics->unmapped_mate_reads == 2);
    REQUIRE(metrics->qcfailed_reads == 0);
    REQUIRE(metrics->reads_mapped_with_zero_quality == 62);

    REQUIRE(metrics->rf_reads == 9);
    REQUIRE(metrics->ff_reads == 6);
    REQUIRE(metrics->rr_reads == 8);
    REQUIRE(metrics->reads_with_mate_mapped_to_different_reference == 6);
    REQUIRE(metrics->reads_with_mate_too_distant == 2);
    REQUIRE(metrics->reads_mapped_and_paired_but_improperly == 0);

    REQUIRE(metrics->total_autosomal_reads == 219);
    REQUIRE(metrics->total_mitochondrial_reads == 177);
    REQUIRE(metrics->duplicate_autosomal_reads == 18);
    REQUIRE(metrics->duplicate_mitochondrial_reads == 93);

    REQUIRE(metrics->peaks.size() == 37276);

    REQUIRE(metrics->tss_enrichment == Approx(6.0));

    nlohmann::json j = collector.to_json();
    unsigned long long int total_reads = j[0]["metrics"]["total_reads"];
    unsigned long long int hqaa = j[0]["metrics"]["hqaa"];
    REQUIRE(total_reads == metrics->total_reads);
    REQUIRE(hqaa == metrics->hqaa);
    REQUIRE(1.28125 == j[0]["metrics"]["short_mononucleosomal_ratio"].get<long double>());
}

TEST_CASE("Metrics::load_alignments errors", "[metrics/load_alignments_errors]") {
    SECTION("MetricsCollector::load_alignments fails without alignment file name") {
        MetricsCollector collector("Broken collector", "human", "a collector without an alignment file", "a library of brutal tests?", "https://theparkerlab.org", "", "", "", "");
        REQUIRE_THROWS_AS(collector.load_alignments(), FileException);
    }

    SECTION("MetricsCollector::load_alignments fails with bad alignment file name") {
        MetricsCollector collector("Broken collector", "human", "a collector with a non-existent alignment file", "a library of brutal tests?", "https://theparkerlab.org", "missing_alignment_file.bam", "", "chrM", "");
        REQUIRE_THROWS_AS(collector.load_alignments(), FileException);
    }
}

TEST_CASE("Metrics::ignore_read_groups", "[metrics/ignore_read_groups]") {
    std::string name("Test collector");
    std::string alignment_file_name("test.bam");
    std::string peak_file_name("test.peaks.gz");

    MetricsCollector collector(name, "human", "a collector for unit tests", "a library of brutal tests?", "https://theparkerlab.org", alignment_file_name, "", "chrM", peak_file_name, "", 1000, true, 1, true, true, {"exclude.dac.bed.gz", "exclude.duke.bed.gz"});

    collector.load_alignments();

    std::cout << collector << std::endl;

    REQUIRE(collector.metrics.size() == 1);

    Metrics* metrics = collector.metrics.cbegin()->second;
    REQUIRE(metrics->total_reads == 1040);
    REQUIRE(metrics->properly_paired_and_mapped_reads == 846);
    REQUIRE(metrics->secondary_reads == 20);
    REQUIRE(metrics->supplementary_reads == 0);
    REQUIRE(metrics->duplicate_reads == 313);
    REQUIRE(metrics->hqaa == 400);
    REQUIRE(metrics->hqaa_short_count == 107);
    REQUIRE(metrics->hqaa_mononucleosomal_count == 60);
    REQUIRE((metrics->hqaa_short_count / (double)metrics->hqaa_mononucleosomal_count) == Approx(1.78333));

    REQUIRE(metrics->paired_reads == 1040);
    REQUIRE(metrics->paired_and_mapped_reads == 864);
    REQUIRE(metrics->fr_reads == 846);
    REQUIRE(metrics->first_reads == 519);
    REQUIRE(metrics->second_reads == 521);
    REQUIRE(metrics->forward_reads == 529);
    REQUIRE(metrics->reverse_reads == 511);
    REQUIRE(metrics->forward_mate_reads == 521);
    REQUIRE(metrics->reverse_mate_reads == 519);

    REQUIRE(metrics->unmapped_reads == 17);
    REQUIRE(metrics->unmapped_mate_reads == 6);
    REQUIRE(metrics->qcfailed_reads == 0);
    REQUIRE(metrics->reads_mapped_with_zero_quality == 111);

    REQUIRE(metrics->rf_reads == 15);
    REQUIRE(metrics->ff_reads == 10);
    REQUIRE(metrics->rr_reads == 17);
    REQUIRE(metrics->reads_with_mate_mapped_to_different_reference == 14);
    REQUIRE(metrics->reads_with_mate_too_distant == 4);
    REQUIRE(metrics->reads_mapped_and_paired_but_improperly == 0);

    REQUIRE(metrics->total_autosomal_reads == 437);
    REQUIRE(metrics->total_mitochondrial_reads == 369);
    REQUIRE(metrics->duplicate_autosomal_reads == 35);
    REQUIRE(metrics->duplicate_mitochondrial_reads == 185);

    REQUIRE(metrics->peaks.size() == 37276);

    nlohmann::json j = collector.to_json();
    unsigned long long int total_reads = j[0]["metrics"]["total_reads"];
    unsigned long long int hqaa = j[0]["metrics"]["hqaa"];
    REQUIRE(total_reads == metrics->total_reads);
    REQUIRE(hqaa == metrics->hqaa);
    REQUIRE(Approx(1.78333) == j[0]["metrics"]["short_mononucleosomal_ratio"].get<long double>());
}

TEST_CASE("Metrics::missing_peak_file", "[metrics/missing_peak_file]") {
    std::string name("Test collector");
    std::string alignment_file_name("test.bam");
    std::string peak_file_name("notthere.peaks.gz");

    MetricsCollector collector(name, "human", "a collector for unit tests", "a library of brutal tests?", "https://theparkerlab.org", alignment_file_name, "", "chrM", peak_file_name, "", 1000, true, 1, true, true, {"exclude.dac.bed.gz", "exclude.duke.bed.gz"});
    REQUIRE_THROWS_AS(collector.load_alignments(), FileException);
}

TEST_CASE("Metrics::missing_tss_file", "[metrics/missing_ss_file]") {
    std::string name("Test collector");
    std::string alignment_file_name("test.bam");
    std::string peak_file_name("test.peaks.gz");
    std::string tss_file_name("notthere.bed.gz");

    MetricsCollector collector(name, "human", "a collector for unit tests", "a library of brutal tests?", "https://theparkerlab.org", alignment_file_name, "", "chrM", peak_file_name, tss_file_name, 1000, true, 1, true, true, {"exclude.dac.bed.gz", "exclude.duke.bed.gz"});
    REQUIRE_THROWS_AS(collector.load_alignments(), FileException);
}
