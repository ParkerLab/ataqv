#include "catch.hpp"

#include "Features.hpp"


TEST_CASE("Features on different references don't overlap", "[features]" ) {
    Feature f1 = Feature("chr1", 1, 100, "feature1");
    Feature f2 = Feature("chr2", 1, 100, "feature2");
    REQUIRE_FALSE(f1.overlaps(f2));
}


TEST_CASE("Features with the same coordinates overlap", "[features]" ) {
    Feature f1 = Feature("chr1", 1, 100, "feature1");
    Feature f2 = Feature("chr1", 1, 100, "feature2");
    REQUIRE(f1.overlaps(f2));
}


TEST_CASE("First feature lies to the left of the second", "[features]" ) {
    Feature f1 = Feature("chr1", 1, 100, "feature1");
    Feature f2 = Feature("chr1", 200, 300, "feature2");
    REQUIRE_FALSE(f1.overlaps(f2));
}


TEST_CASE("First feature lies to the right of the second", "[features]" ) {
    Feature f1 = Feature("chr1", 200, 300, "feature1");
    Feature f2 = Feature("chr1", 1, 100, "feature2");
    REQUIRE_FALSE(f1.overlaps(f2));
}


TEST_CASE("First feature's end lies within the second", "[features]" ) {
    Feature f1 = Feature("chr1", 1, 100, "feature1");
    Feature f2 = Feature("chr1", 50, 150, "feature2");
    REQUIRE(f1.overlaps(f2));
}


TEST_CASE("First feature's start lies within the second", "[features]" ) {
    Feature f1 = Feature("chr1", 50, 150, "feature1");
    Feature f2 = Feature("chr1", 1, 100, "feature2");
    REQUIRE(f1.overlaps(f2));
}


TEST_CASE("Second feature's start lies within the first", "[features]" ) {
    Feature f1 = Feature("chr1", 1, 100, "feature1");
    Feature f2 = Feature("chr1", 50, 150, "feature2");
    REQUIRE(f1.overlaps(f2));
}


TEST_CASE("Second feature's end lies within the first", "[features]" ) {
    Feature f1 = Feature("chr1", 50, 150, "feature1");
    Feature f2 = Feature("chr1", 1, 100, "feature2");
    REQUIRE(f1.overlaps(f2));
}


TEST_CASE("Feature size is correct", "[features]" ) {
    Feature f1 = Feature("chr1", 50, 150, "feature1");
    REQUIRE(f1.size() == 100);
}


TEST_CASE("Feature constructors", "features/constructors") {
    SECTION("Default constructor") {
        Feature f;
        REQUIRE(f.reference == "");
        REQUIRE(f.name == "");
        REQUIRE(f.start == 0);
        REQUIRE(f.end == 0);
    }

    SECTION("Explicit constructor") {
        Feature f("chr1", 1, 1000, "peak_1");
        REQUIRE(f.reference == "chr1");
        REQUIRE(f.name == "peak_1");
        REQUIRE(f.start == 1);
        REQUIRE(f.end == 1000);
    }

    SECTION("Test Feature construction with HTS record") {
        std::string sam("data:@SQ\tSN:chr20\tLN:63025520\nSRR891268.122333488\t83\tchr20\t60087\t60\t50M\t=\t60058\t-79\tAGGAAGGAGAGAGTGAAGGAACTGCCAGGTGACACACTCCCACCATGGAC\tJJJJJJJJJJJJJJJJJIHHGIGIIIJJJJJIIGGIJHHHHHFFFFFCBB\tMD:Z:50\tPG:Z:MarkDuplicates\tNM:i:0\tAS:i:50\tXS:i:23\n");

        samFile *in = sam_open(sam.c_str(), "r");
        bam_hdr_t *header = sam_hdr_read(in);
        bam1_t *record = bam_init1();

        REQUIRE(sam_read1(in, header, record) >= 0);

        Feature f(header, record);
        REQUIRE(f.reference == "chr20");
        REQUIRE(f.name == "SRR891268.122333488");
        REQUIRE(f.start == 60086);
        REQUIRE(f.end == 60136);
    }
}


TEST_CASE("Feature operator<", "features/operator<") {
    SECTION("Start is less") {
        Feature f1 = Feature("chr1", 1, 100, "feature1");
        Feature f2 = Feature("chr1", 50, 150, "feature2");
        REQUIRE(f1 < f2);
        REQUIRE_FALSE(f2 < f1);
    }

    SECTION("End is less") {
        Feature f1 = Feature("chr1", 1, 100, "feature1");
        Feature f2 = Feature("chr1", 1, 150, "feature2");
        REQUIRE(f1 < f2);
        REQUIRE_FALSE(f2 < f1);
    }
}


TEST_CASE("Feature operator>>", "features/operator>>") {
    std::stringstream s1("chr1\t1\t100\tpeak_1");
    Feature f;
    s1 >> f;
    REQUIRE(f.reference == "chr1");
    REQUIRE(f.name == "peak_1");
    REQUIRE(f.start == 1);
    REQUIRE(f.end == 100);
}


TEST_CASE("Feature operator<<", "features/operator<<") {
    Feature f("chr1", 1, 100, "peak_1");
    std::stringstream ss;
    ss << f;
    REQUIRE("chr1\t1\t100\tpeak_1" == ss.str());
}
