#include "catch.hpp"

#include "Peaks.hpp"


TEST_CASE("Peak operator<", "peaks/operator<") {
    SECTION("Start is less") {
        Peak p1("chr1", 1, 100, "peak1");
        Peak p2("chr1", 50, 150, "peak2");
        REQUIRE(p1 < p2);
        REQUIRE_FALSE(p2 < p1);
    }

    SECTION("End is less") {
        Peak p1 = Peak("chr1", 1, 100, "peak1");
        Peak p2 = Peak("chr1", 1, 150, "peak2");
        REQUIRE(p1 < p2);
        REQUIRE_FALSE(p2 < p1);
    }
}


TEST_CASE("Peak operator>>", "peaks/operator>>") {
    std::stringstream ss("chr1\t1\t100\tpeak_1");
    Peak p;
    ss >> p;
    REQUIRE(p.reference == "chr1");
    REQUIRE(p.name == "peak_1");
    REQUIRE(p.start == 1);
    REQUIRE(p.end == 100);
}


TEST_CASE("Peak operator<<", "peaks/operator<<") {
    Peak p("chr1", 1, 100, "peak_1");
    std::stringstream ss;
    ss << p;
    REQUIRE("chr1\t1\t100\tpeak_1" == ss.str());
}


TEST_CASE("Peak empty", "peaks/empty") {
    PeakTree tree;

    Peak peak1("chr1", 100, 200, "peak1");
    Peak peak2("chr1", 150, 250, "peak2");

    tree.add(peak1);
    tree.add(peak2);

    REQUIRE_FALSE(tree.empty());
    REQUIRE(2 == tree.size());
}


TEST_CASE("Peak sorting", "peaks/sorting") {
    PeakTree tree;

    Peak peak1("chr1", 100, 200, "peak1");
    peak1.overlapping_hqaa = 100;

    Peak peak2("chr1", 150, 250, "peak2");
    peak2.overlapping_hqaa = 200;

    Peak peak3("chr2", 100, 200, "peak3");
    peak3.overlapping_hqaa = 300;

    Peak peak4("chr10", 100, 200, "peak4");
    peak4.overlapping_hqaa = 400;

    tree.add(peak4);
    tree.add(peak2);
    tree.add(peak1);
    tree.add(peak3);

    REQUIRE(peak1 < peak2);
    REQUIRE(peak2 < peak3);
    REQUIRE(peak3 < peak4);

    SECTION("Default ordering") {
        std::vector<Peak> default_order = tree.list_peaks();

        REQUIRE(default_order[0] == peak1);
        REQUIRE(default_order[1] == peak2);
        REQUIRE(default_order[2] == peak3);
        REQUIRE(default_order[3] == peak4);
    }

    SECTION("Ordered by overlapping HQAA") {
        std::vector<Peak> default_order = tree.list_peaks_by_overlapping_hqaa_descending();

        REQUIRE(default_order[0] == peak4);
        REQUIRE(default_order[1] == peak3);
        REQUIRE(default_order[2] == peak2);
        REQUIRE(default_order[3] == peak1);
    }
}


TEST_CASE("Peak HQAA counting", "peaks/hqaa") {
    PeakTree tree;

    Peak peak1("chr1", 100, 200, "peak1");
    peak1.overlapping_hqaa = 100;

    Peak peak2("chr1", 150, 250, "peak2");
    peak2.overlapping_hqaa = 200;

    Peak peak3("chr1", 200, 300, "peak3");
    peak3.overlapping_hqaa = 300;

    Peak peak4("chr10", 100, 200, "peak4");
    peak4.overlapping_hqaa = 400;

    tree.add(peak2);
    tree.add(peak1);
    tree.add(peak4);
    tree.add(peak3);

    REQUIRE_FALSE(tree.empty());

    Feature hqaa1("chr1", 125, 175, "hqaa1");
    tree.record_alignment(hqaa1, true, false);

    ReferencePeakCollection chr1 = *tree.get_reference_peaks("chr1");
    chr1.sort();
    REQUIRE(chr1.peaks[0].overlapping_hqaa == 101);
    REQUIRE(chr1.peaks[1].overlapping_hqaa == 201);
    REQUIRE(chr1.peaks[2].overlapping_hqaa == 300);

    ReferencePeakCollection chr10 = *tree.get_reference_peaks("chr10");
    REQUIRE(chr10.peaks[0].overlapping_hqaa == 400);
}


TEST_CASE("PeakTree reference peak counts", "[peaks/referencepeakcounts]") {
    PeakTree tree;

    Peak peak1("chr1", 100, 200, "peak1");
    Peak peak2("chr1", 150, 250, "peak2");
    Peak peak3("chr2", 150, 250, "peak3");

    tree.add(peak1);
    tree.add(peak2);
    tree.add(peak3);

    REQUIRE_FALSE(tree.empty());
    REQUIRE(3 == tree.size());
    std::stringstream ss;
    tree.print_reference_peak_counts(&ss);
    REQUIRE("chr1 peak count: 2\nchr2 peak count: 1\n" == ss.str());
}


TEST_CASE("ReferencePeakCollection overlap", "[peaks/rpcoverlap]") {
    PeakTree tree;

    Peak peak1("chr1", 100, 200, "peak1");
    Peak peak2("chr1", 150, 250, "peak2");
    Peak peak3("chr2", 150, 250, "peak3");

    tree.add(peak1);
    tree.add(peak2);
    tree.add(peak3);

    ReferencePeakCollection chr1 = *tree.get_reference_peaks("chr1");
    REQUIRE(chr1.overlaps(peak1));
    REQUIRE_FALSE(chr1.overlaps(peak3));
}


TEST_CASE("ReferencePeakCollection only accepts peaks on the same reference", "[peaks/samereference]") {
    ReferencePeakCollection rpc;

    Peak peak1("chr1", 100, 200, "peak1");
    Peak peak2("chr1", 150, 250, "peak2");
    Peak peak3("chr2", 150, 250, "peak3");

    REQUIRE_NOTHROW(rpc.add(peak1));
    REQUIRE_NOTHROW(rpc.add(peak2));
    REQUIRE_THROWS_AS(rpc.add(peak3), std::out_of_range);
}
